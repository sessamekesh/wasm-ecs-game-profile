#include <igasync/promise_combiner.h>
#include <igdemo/logic/framecommon.h>
#include <igdemo/render/skeletal-animation.h>
#include <igdemo/render/world-transform-component.h>
#include <igdemo/systems/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_transform.h>

namespace {

struct CtxOzzSamplingConcurrencyParams {
  std::uint32_t samplingChunkSize;
  std::uint32_t transformationChunkSize;
};

struct OzzSamplingBuffer {
  ozz::animation::SamplingJob::Context samplingContext;
  std::vector<ozz::math::SoaTransform> animLocals;

  OzzSamplingBuffer(const ozz::animation::Animation& animation)
      : samplingContext(animation.num_tracks()),
        animLocals(animation.num_soa_tracks()) {}

  void maybe_resize(const ozz::animation::Animation& animation) {
    if (samplingContext.max_tracks() != animation.num_tracks()) {
      samplingContext.Resize(animation.num_tracks());
    }

    if (animLocals.size() != animation.num_soa_tracks()) {
      animLocals.resize(animation.num_soa_tracks());
    }
  }
};

struct OzzTransformationsBuffers {
  OzzTransformationsBuffers(const ozz::animation::Skeleton& skeleton)
      : skeletonLocals(skeleton.num_soa_joints()),
        skeletonModels(skeleton.num_joints()) {}

  std::vector<ozz::math::SoaTransform> skeletonLocals;
  std::vector<ozz::math::Float4x4> skeletonModels;
};

struct CtxOzzJobRemappers {
  using RasKeyT = std::tuple<const igasset::OzzAnimationWithNames*,
                             const ozz::animation::Skeleton*>;
  using PgsKeyT = std::tuple<const ozz::animation::Skeleton*,
                             const std::vector<std::string>*>;

  std::unordered_map<RasKeyT,
                     igasset::RemapAnimationToSkeletonIndicesJob::Context>
      rasMap;
  std::unordered_map<PgsKeyT, igasset::PrepareGpuSkinningDataJob::Context>
      pgsMap;

  igasset::RemapAnimationToSkeletonIndicesJob::Context& ras_context(
      const igasset::OzzAnimationWithNames* anim,
      const ozz::animation::Skeleton* s) {
    auto it = rasMap.find({anim, s});
    if (it == rasMap.end()) {
      rasMap[{anim, s}] =
          igasset::RemapAnimationToSkeletonIndicesJob::Context{};
      rasMap[{anim, s}].check_or_init(s, anim);
      return rasMap[{anim, s}];
    }

    return it->second;
  }

  igasset::PrepareGpuSkinningDataJob::Context& pgs_context(
      const ozz::animation::Skeleton* s,
      const std::vector<std::string>* model_bones) {
    auto it = pgsMap.find({s, model_bones});
    if (it == pgsMap.end()) {
      pgsMap[{s, model_bones}] = igasset::PrepareGpuSkinningDataJob::Context{};
      pgsMap[{s, model_bones}].check_or_init(s, model_bones);
      return pgsMap[{s, model_bones}];
    }

    return it->second;
  }
};

}  // namespace

namespace igdemo {

void init_animation_systems(igecs::WorldView* wv) {
  wv->attach_ctx<CtxOzzSamplingConcurrencyParams>(20, 20);
  wv->attach_ctx<CtxOzzJobRemappers>();
}

//
// AdvanceAnimationTimeSystem
//
const igecs::WorldView::Decl& AdvanceAnimationTimeSystem::decl() {
  static igecs::WorldView::Decl decl = igecs::WorldView::Decl()
                                           // Declared outside of system
                                           .ctx_reads<CtxFrameTime>()

                                           // Iterators
                                           .writes<AnimationStateComponent>();

  return decl;
}

void AdvanceAnimationTimeSystem::run(igecs::WorldView* wv) {
  const auto& dt = wv->ctx<CtxFrameTime>().secondsSinceLastFrame;

  auto view = wv->view<AnimationStateComponent>();

  for (auto [e, animationState] : view.each()) {
    animationState.sample_time += dt;
    while (animationState.sample_time >
               animationState.animation->animation.duration() &&
           animationState.loop) {
      animationState.sample_time -=
          animationState.animation->animation.duration();
    }
  }
}

//
// SampleOzzAnimationSystem
//
void SampleOzzAnimationSystem::set_entities_per_task(igecs::WorldView* wv,
                                                     std::uint32_t chunk_size) {
  if (chunk_size > 0) {
    wv->mut_ctx<CtxOzzSamplingConcurrencyParams>().samplingChunkSize =
        chunk_size;
  }
}

const igecs::WorldView::Decl& SampleOzzAnimationSystem::decl() {
  static igecs::WorldView::Decl decl =
      igecs::WorldView::Decl()
          // Defined by calling init_animation_systems
          .ctx_reads<CtxOzzSamplingConcurrencyParams>()
          // Iterators (external)
          .reads<AnimationStateComponent>()
          // Iterators (internal)
          .writes<OzzSamplingBuffer>();

  return decl;
}

std::shared_ptr<igasync::Promise<void>> SampleOzzAnimationSystem::run(
    igecs::WorldView* wv, std::shared_ptr<igasync::TaskList> main_thread,
    std::shared_ptr<igasync::TaskList> any_thread,
    std::function<void(igasync::TaskProfile profile)> profile_cb) {
  auto process_list = std::make_shared<std::vector<entt::entity>>();

  const auto& chunk_size =
      wv->ctx<CtxOzzSamplingConcurrencyParams>().samplingChunkSize;

  // Pass 1: Attach sampling buffers to any components that don't have them,
  //  and collect references to the entities that should be processed.
  {
    auto view = wv->view<const AnimationStateComponent>();
    for (auto [e, animation_state] : view.each()) {
      if (!wv->has<OzzSamplingBuffer>(e)) {
        wv->attach<OzzSamplingBuffer>(e, animation_state.animation->animation);
      }
      process_list->push_back(e);
    }
  }

  // Schedule tasks in the appropriate chunk sizes to run
  auto combiner = igasync::PromiseCombiner::Create();

  for (std::uint32_t startChunk = 0; startChunk < process_list->size();
       startChunk += chunk_size) {
    std::uint32_t ct =
        std::min(chunk_size,
                 static_cast<std::uint32_t>(process_list->size()) - startChunk);

    combiner->add(any_thread->run([startChunk, ct, process_list, wv]() {
      for (int i = startChunk; i < startChunk + ct; i++) {
        entt::entity e = (*process_list)[i];

        const auto& animationState = wv->read<AnimationStateComponent>(e);
        auto& ozzSamplingBuffer = wv->write<OzzSamplingBuffer>(e);

        ozzSamplingBuffer.maybe_resize(animationState.animation->animation);

        ozz::animation::SamplingJob sampling_job;
        sampling_job.animation = &animationState.animation->animation;
        sampling_job.context = &ozzSamplingBuffer.samplingContext;
        sampling_job.output =
            ozz::span(&ozzSamplingBuffer.animLocals[startChunk], ct);
        sampling_job.ratio = (animationState.sample_time /
                              animationState.animation->animation.duration());
        sampling_job.Run();
      }
    }),
                  any_thread);
  }

  return combiner->combine([](auto) {}, any_thread);
}

//
// TransformOzzAnimationToModelSpaceSystem
//
void TransformOzzAnimationToModelSpaceSystem::set_entities_per_task(
    igecs::WorldView* wv, std::uint32_t chunk_size) {
  if (chunk_size > 0) {
    wv->mut_ctx<CtxOzzSamplingConcurrencyParams>().transformationChunkSize =
        chunk_size;
  }
}

const igecs::WorldView::Decl& TransformOzzAnimationToModelSpaceSystem::decl() {
  static igecs::WorldView::Decl decl =
      igecs::WorldView::Decl()
          // Defined by calling init_animation_systems
          .ctx_reads<CtxOzzSamplingConcurrencyParams>()
          .ctx_reads<CtxOzzJobRemappers>()

          // Iterators (external)
          .writes<SkinComponent>()

          // Iterators (internal)
          .reads<OzzSamplingBuffer>()
          .writes<OzzTransformationsBuffers>();

  return decl;
}

std::shared_ptr<igasync::Promise<void>>
TransformOzzAnimationToModelSpaceSystem::run(
    igecs::WorldView* wv, std::shared_ptr<igasync::TaskList> main_thread,
    std::shared_ptr<igasync::TaskList> any_thread,
    std::function<void(igasync::TaskProfile profile)> profile_cb) {
  auto process_list = std::make_shared<std::vector<entt::entity>>();

  const auto& chunk_size =
      wv->ctx<CtxOzzSamplingConcurrencyParams>().transformationChunkSize;

  // Pass 1: Attach sampling buffers to any components that don't have them,
  //  and collect references to the entities that should be processed.
  {
    auto& transform_contexts = wv->mut_ctx<CtxOzzJobRemappers>();
    auto view = wv->view<const AnimationStateComponent, const OzzSamplingBuffer,
                         SkinComponent>();
    for (auto [e, animation_state, sampling_buffer, skin] : view.each()) {
      // Make sure (derived) OzzTransformationsBuffers is attached
      if (!wv->has<OzzTransformationsBuffers>(e)) {
        wv->attach<OzzTransformationsBuffers>(e, *skin.skeleton);
      }

      // Make sure appropriate sampling contexts exist
      transform_contexts.ras_context(animation_state.animation, skin.skeleton);
      transform_contexts.pgs_context(skin.skeleton, skin.boneNames);

      process_list->push_back(e);
    }
  }

  // Schedule tasks in the appropriate chunk sizes to run
  auto combiner = igasync::PromiseCombiner::Create();

  for (std::uint32_t startChunk = 0; startChunk < process_list->size();
       startChunk += chunk_size) {
    std::uint32_t ct =
        std::min(chunk_size,
                 static_cast<std::uint32_t>(process_list->size()) - startChunk);

    combiner->add(any_thread->run([startChunk, ct, process_list, wv]() {
      auto& transform_contexts = wv->mut_ctx<CtxOzzJobRemappers>();
      for (int i = startChunk; i < startChunk + ct; i++) {
        entt::entity e = (*process_list)[i];

        const auto* animation = wv->read<AnimationStateComponent>(e).animation;
        const auto& ozzSamplingBuffer = wv->read<OzzSamplingBuffer>(e);
        auto& skinComponent = wv->write<SkinComponent>(e);
        auto& ozzTransformationsBuffers =
            wv->write<OzzTransformationsBuffers>(e);

        // Job 1 - remap animation to skeleton indices
        igasset::RemapAnimationToSkeletonIndicesJob ras_job;
        ras_job.animation = animation;
        ras_job.skeleton = skinComponent.skeleton;
        ras_job.context =
            &transform_contexts.ras_context(animation, skinComponent.skeleton);
        ras_job.input = ozz::span(&ozzSamplingBuffer.animLocals[0],
                                  ozzSamplingBuffer.animLocals.size());
        ras_job.output =
            ozz::span(&ozzTransformationsBuffers.skeletonLocals[0],
                      ozzTransformationsBuffers.skeletonLocals.size());
        ras_job.Run();

        // Job 2 - Local to model transformation (skeleton space)
        ozz::animation::LocalToModelJob ltm_job;
        ltm_job.skeleton = skinComponent.skeleton;
        ltm_job.input =
            ozz::span(&ozzTransformationsBuffers.skeletonLocals[0],
                      ozzTransformationsBuffers.skeletonLocals.size());
        ltm_job.output =
            ozz::span(&ozzTransformationsBuffers.skeletonModels[0],
                      ozzTransformationsBuffers.skeletonModels.size());
        ltm_job.Run();

        // Job 3 - remap skeleton indices to geometry indices, and apply inverse
        // bind poses
        igasset::PrepareGpuSkinningDataJob pgs_job;
        pgs_job.skeleton = skinComponent.skeleton;
        pgs_job.context = &transform_contexts.pgs_context(
            skinComponent.skeleton, skinComponent.boneNames);
        pgs_job.inv_bind_poses = skinComponent.invBindPoses;
        pgs_job.model_bones = skinComponent.boneNames;
        pgs_job.model_space_input =
            ozz::span(&ozzTransformationsBuffers.skeletonModels[0],
                      ozzTransformationsBuffers.skeletonModels.size());
        pgs_job.output =
            ozz::span(&skinComponent.skin[0], skinComponent.skin.size());
        pgs_job.Run();
      }
    }),
                  any_thread);
  }
}

}  // namespace igdemo
