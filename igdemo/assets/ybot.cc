#include <igasset/igpack_decoder.h>
#include <igasync/promise_combiner.h>
#include <igdemo/assets/ybot.h>
#include <igdemo/render/animated-pbr.h>
#include <igdemo/render/ctx-components.h>
#include <igdemo/render/skeletal-animation.h>
#include <igdemo/render/world-transform-component.h>

namespace {

struct CtxYbotResources {
  igasset::OzzAnimationWithNames DefeatedAnimation;
  igasset::OzzAnimationWithNames WalkAnimation;
  igasset::OzzAnimationWithNames RunAnimation;
  igasset::OzzAnimationWithNames IdleAnimation;
  ozz::animation::Skeleton Skeleton;

  igdemo::AnimatedPbrGeometry Geometry;

  igdemo::AnimatedPbrMaterial redMaterial;
  igdemo::AnimatedPbrMaterial blueMaterial;
  igdemo::AnimatedPbrMaterial greenMaterial;
};

}  // namespace

namespace igdemo {

std::shared_ptr<igasync::Promise<std::vector<std::string>>> load_ybot_resources(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks,
    std::shared_ptr<igasync::Promise<void>> shaderLoadedPromise) {
  auto decoder_promise =
      procs.loadFileCb(asset_root_path + "ybot.igpack")
          ->then_consuming(
              [](std::variant<std::string, FileReadError> rsl) {
                if (std::holds_alternative<FileReadError>(rsl)) {
                  return std::string("");
                }

                return std::get<std::string>(rsl);
              },
              compute_tasks)
          ->then_consuming(igasset::IgpackDecoder::Create, compute_tasks);

  auto ybot_mesh_promise = decoder_promise->then(
      [](std::shared_ptr<igasset::IgpackDecoder> decoder)
          -> std::optional<igasset::DracoDecoder> {
        if (!decoder) {
          return {};
        }

        auto rsl = decoder->extract_draco_decoder("geo");
        if (std::holds_alternative<igasset::IgpackExtractError>(rsl)) {
          return {};
        }

        return std::get<igasset::DracoDecoder>(std::move(rsl));
      },
      compute_tasks);

  auto combiner = igasync::PromiseCombiner::Create();

  auto pos_norm_key = combiner->add_consuming(
      ybot_mesh_promise->then(
          [](const std::optional<igasset::DracoDecoder>& decoder)
              -> std::optional<std::vector<igasset::PosNormalVertexData3D>> {
            if (!decoder) return {};

            auto dat = decoder->get_pos_norm_data();
            if (std::holds_alternative<igasset::DracoDecoderError>(dat)) {
              return {};
            }

            return std::get<std::vector<igasset::PosNormalVertexData3D>>(
                std::move(dat));
          },
          compute_tasks),
      compute_tasks);

  auto indices_key = combiner->add_consuming(
      ybot_mesh_promise->then(
          [](const std::optional<igasset::DracoDecoder>& decoder)
              -> std::optional<std::vector<std::uint16_t>> {
            if (!decoder) return {};

            auto dat = decoder->get_u16_indices();
            if (std::holds_alternative<igasset::DracoDecoderError>(dat)) {
              return {};
            }

            return std::get<std::vector<std::uint16_t>>(std::move(dat));
          },
          compute_tasks),
      compute_tasks);

  auto bone_weights_key = combiner->add_consuming(
      ybot_mesh_promise->then(
          [](const std::optional<igasset::DracoDecoder>& decoder)
              -> std::optional<std::vector<igasset::BoneWeightsVertexData>> {
            if (!decoder) return {};

            auto dat = decoder->get_bone_data();
            if (std::holds_alternative<igasset::DracoDecoderError>(dat)) {
              return {};
            }

            return std::get<std::vector<igasset::BoneWeightsVertexData>>(
                std::move(dat));
          },
          compute_tasks),
      compute_tasks);

  auto bone_names_key = combiner->add_consuming(
      ybot_mesh_promise->then(
          [](const std::optional<igasset::DracoDecoder>& decoder)
              -> std::optional<std::vector<std::string>> {
            if (!decoder) return {};

            auto dat = decoder->get_bone_names();
            if (std::holds_alternative<igasset::DracoDecoderError>(dat)) {
              return {};
            }

            return std::get<std::vector<std::string>>(std::move(dat));
          },
          compute_tasks),
      compute_tasks);

  auto bone_inv_bind_poses_key = combiner->add_consuming(
      ybot_mesh_promise->then(
          [](const std::optional<igasset::DracoDecoder>& decoder)
              -> std::optional<std::vector<glm::mat4>> {
            if (!decoder) return {};

            auto dat = decoder->get_bone_inv_bind_poses();
            if (std::holds_alternative<igasset::DracoDecoderError>(dat)) {
              return {};
            }

            return std::get<std::vector<glm::mat4>>(std::move(dat));
          },
          compute_tasks),
      compute_tasks);

  auto skeleton_key = combiner->add_consuming(
      decoder_promise->then(
          [](const std::shared_ptr<igasset::IgpackDecoder>& decoder)
              -> std::optional<ozz::animation::Skeleton> {
            if (!decoder) {
              return {};
            }

            auto rsl = decoder->extract_ozz_skeleton("skeleton");
            if (std::holds_alternative<igasset::IgpackExtractError>(rsl)) {
              return {};
            }

            return std::get<ozz::animation::Skeleton>(std::move(rsl));
          },
          compute_tasks),
      compute_tasks);

  auto idle_key = combiner->add_consuming(
      decoder_promise->then(
          [](const std::shared_ptr<igasset::IgpackDecoder>& decoder)
              -> std::optional<igasset::OzzAnimationWithNames> {
            if (!decoder) {
              return {};
            }

            auto rsl = decoder->extract_ozz_animation("idle");
            if (std::holds_alternative<igasset::IgpackExtractError>(rsl)) {
              return {};
            }

            return std::get<igasset::OzzAnimationWithNames>(std::move(rsl));
          },
          compute_tasks),
      compute_tasks);

  auto walk_key = combiner->add_consuming(
      decoder_promise->then(
          [](const std::shared_ptr<igasset::IgpackDecoder>& decoder)
              -> std::optional<igasset::OzzAnimationWithNames> {
            if (!decoder) {
              return {};
            }

            auto rsl = decoder->extract_ozz_animation("walk");
            if (std::holds_alternative<igasset::IgpackExtractError>(rsl)) {
              return {};
            }

            return std::get<igasset::OzzAnimationWithNames>(std::move(rsl));
          },
          compute_tasks),
      compute_tasks);

  auto run_key = combiner->add_consuming(
      decoder_promise->then(
          [](const std::shared_ptr<igasset::IgpackDecoder>& decoder)
              -> std::optional<igasset::OzzAnimationWithNames> {
            if (!decoder) {
              return {};
            }

            auto rsl = decoder->extract_ozz_animation("run");
            if (std::holds_alternative<igasset::IgpackExtractError>(rsl)) {
              return {};
            }

            return std::get<igasset::OzzAnimationWithNames>(std::move(rsl));
          },
          compute_tasks),
      compute_tasks);

  auto defeated_key = combiner->add_consuming(
      decoder_promise->then(
          [](const std::shared_ptr<igasset::IgpackDecoder>& decoder)
              -> std::optional<igasset::OzzAnimationWithNames> {
            if (!decoder) {
              return {};
            }

            auto rsl = decoder->extract_ozz_animation("defeated");
            if (std::holds_alternative<igasset::IgpackExtractError>(rsl)) {
              return {};
            }

            return std::get<igasset::OzzAnimationWithNames>(std::move(rsl));
          },
          compute_tasks),
      compute_tasks);

  combiner->add(shaderLoadedPromise, compute_tasks);

  return combiner->combine(
      [r, pos_norm_key, indices_key, bone_weights_key, bone_names_key,
       bone_inv_bind_poses_key, skeleton_key, idle_key, walk_key, run_key,
       defeated_key, device, queue](
          igasync::PromiseCombiner::Result rsl) -> std::vector<std::string> {
        std::vector<std::string> missing_elements = {};

        auto pos_norm_data = rsl.move(pos_norm_key);
        auto indices = rsl.move(indices_key);
        auto bone_weights = rsl.move(bone_weights_key);
        auto bone_names = rsl.move(bone_names_key);
        auto inv_bind_poses = rsl.move(bone_inv_bind_poses_key);
        auto skeleton = rsl.move(skeleton_key);
        auto idle = rsl.move(idle_key);
        auto walk = rsl.move(walk_key);
        auto run = rsl.move(run_key);
        auto defeated = rsl.move(defeated_key);

        if (!pos_norm_data) {
          missing_elements.push_back("ybot::pos_norm_data");
        }
        if (!indices) {
          missing_elements.push_back("ybot::indices");
        }
        if (!bone_weights) {
          missing_elements.push_back("ybot::bone_weights");
        }
        if (!bone_names) {
          missing_elements.push_back("ybot::bone_names");
        }
        if (!inv_bind_poses) {
          missing_elements.push_back("ybot::inv_bind_poses");
        }
        if (!skeleton) {
          missing_elements.push_back("ybot::skeleton");
        }
        if (!idle) {
          missing_elements.push_back("ybot::idle");
        }
        if (!walk) {
          missing_elements.push_back("ybot::walk");
        }
        if (!run) {
          missing_elements.push_back("ybot::run");
        }
        if (!defeated) {
          missing_elements.push_back("ybot::defeated");
        }

        if (missing_elements.size() > 0) {
          return missing_elements;
        }

        AnimatedPbrGeometry ybot_geometry{device,
                                          queue,
                                          *pos_norm_data,
                                          *bone_weights,
                                          *indices,
                                          *std::move(bone_names),
                                          *std::move(inv_bind_poses)};

        auto wv = igecs::WorldView::Thin(r);

        const auto& shader = wv.ctx<CtxAnimatedPbrPipeline>();

        CtxAnimatedPbrPipeline::GPUPbrColorParams rParams{};
        rParams.albedo = glm::vec3(1.f, 0.f, 0.2f);
        rParams.metallic = 0.85f;
        rParams.roughness = 0.35f;

        CtxAnimatedPbrPipeline::GPUPbrColorParams gParams{};
        gParams.albedo = glm::vec3(0.f, 1.f, 0.2f);
        gParams.metallic = 0.85f;
        gParams.roughness = 0.95f;

        CtxAnimatedPbrPipeline::GPUPbrColorParams bParams{};
        bParams.albedo = glm::vec3(0.f, 0.12f, 1.f);
        bParams.metallic = 0.05f;
        bParams.roughness = 0.75f;

        wv.attach_ctx<CtxYbotResources>(CtxYbotResources{
            std::move(*defeated), std::move(*walk), std::move(*run),
            std::move(*idle), std::move(*skeleton), std::move(ybot_geometry),
            AnimatedPbrMaterial(device, queue, shader.obj_bgl, rParams),
            AnimatedPbrMaterial(device, queue, shader.obj_bgl, gParams),
            AnimatedPbrMaterial(device, queue, shader.obj_bgl, bParams)});

        return {};
      },
      main_thread_tasks);
}

igecs::WorldView::Decl YbotRenderResources::decl() {
  return igecs::WorldView::Decl()
      .ctx_reads<CtxYbotResources>()
      .ctx_reads<CtxWgpuDevice>()
      .ctx_reads<CtxAnimatedPbrPipeline>()
      .writes<AnimationStateComponent>()
      .writes<AnimatedPbrInstance>()
      .writes<AnimatedPbrSkinBindGroup>()
      .writes<WorldTransformComponent>()
      .writes<SkinComponent>();
}

bool YbotRenderResources::has_render_resources(igecs::WorldView* wv,
                                               entt::entity e) {
  return wv->has<AnimatedPbrInstance>(e) &&
         wv->has<AnimatedPbrSkinBindGroup>(e) &&
         wv->has<AnimationStateComponent>(e) &&
         wv->has<WorldTransformComponent>(e) && wv->has<SkinComponent>(e);
}

void YbotRenderResources::attach(igecs::WorldView* wv, entt::entity e,
                                 MaterialType materialType) {
  const auto& ybotResources = wv->ctx<CtxYbotResources>();
  const auto& ctxDevice = wv->ctx<CtxWgpuDevice>();
  const auto& shader = wv->ctx<CtxAnimatedPbrPipeline>();

  const igdemo::AnimatedPbrMaterial* material = nullptr;
  switch (materialType) {
    case MaterialType::GREEN:
      material = &ybotResources.greenMaterial;
      break;
    case MaterialType::BLUE:
      material = &ybotResources.blueMaterial;
      break;
    case MaterialType::RED:
    default:
      material = &ybotResources.redMaterial;
      break;
  }

  wv->attach<AnimatedPbrInstance>(
      e, AnimatedPbrInstance{material, &ybotResources.Geometry});
  wv->attach<AnimatedPbrSkinBindGroup>(e, ctxDevice.device, ctxDevice.queue,
                                       shader.skin_bgl,
                                       ybotResources.Geometry.boneNames.size());
  wv->attach<AnimationStateComponent>(
      e, AnimationStateComponent{&ybotResources.IdleAnimation, 0.f, true});
  wv->attach<WorldTransformComponent>(e);
  wv->attach<SkinComponent>(
      e, SkinComponent{
             &ybotResources.Geometry.boneNames,
             &ybotResources.Geometry.invBindPoses, &ybotResources.Skeleton,
             std::vector<glm::mat4>(ybotResources.Geometry.boneNames.size())});
}

igecs::WorldView::Decl YbotAnimationResources::decl() {
  return igecs::WorldView::Decl()
      .ctx_reads<CtxYbotResources>()
      .writes<AnimationStateComponent>();
}

void YbotAnimationResources::update_animation_state(
    igecs::WorldView* wv, entt::entity e, AnimationType animation_type) {
  const auto& ybotResources = wv->ctx<CtxYbotResources>();
  auto& animation_state = wv->write<AnimationStateComponent>(e);

  switch (animation_type) {
    case AnimationType::WALK:
      if (animation_state.animation != &ybotResources.WalkAnimation) {
        animation_state.animation = &ybotResources.WalkAnimation;
        animation_state.loop = true;
        animation_state.sample_time = 0.f;
      }
      break;
    case AnimationType::RUN:
      if (animation_state.animation != &ybotResources.RunAnimation) {
        animation_state.animation = &ybotResources.RunAnimation;
        animation_state.loop = true;
        animation_state.sample_time = 0.f;
      }
      break;
    case AnimationType::DEFEATED:
      if (animation_state.animation != &ybotResources.DefeatedAnimation) {
        animation_state.animation = &ybotResources.DefeatedAnimation;
        animation_state.loop = true;
        animation_state.sample_time = 0.f;
      }
      break;
    default:
    case AnimationType::IDLE:
      if (animation_state.animation != &ybotResources.IdleAnimation) {
        animation_state.animation = &ybotResources.IdleAnimation;
        animation_state.loop = true;
        animation_state.sample_time = 0.f;
      }
      break;
  }
}

}  // namespace igdemo
