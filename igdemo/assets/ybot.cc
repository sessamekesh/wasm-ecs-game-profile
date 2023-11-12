#include <igasset/igpack_decoder.h>
#include <igasync/promise_combiner.h>
#include <igdemo/assets/ybot.h>
#include <igdemo/render/animated-pbr.h>

namespace {

struct CtxYbotResources {
  igasset::OzzAnimationWithNames DefeatedAnimation;
  igasset::OzzAnimationWithNames WalkAnimation;
  igasset::OzzAnimationWithNames RunAnimation;
  igasset::OzzAnimationWithNames IdleAnimation;
  ozz::animation::Skeleton Skeleton;

  igdemo::AnimatedPbrGeometry Geometry;
};

}  // namespace

namespace igdemo {

std::shared_ptr<igasync::Promise<std::vector<std::string>>> load_ybot_resources(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks) {
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

        wv.attach_ctx<CtxYbotResources>(CtxYbotResources{
            std::move(*defeated), std::move(*walk), std::move(*run),
            std::move(*idle), std::move(*skeleton), std::move(ybot_geometry)});

        return {};
      },
      main_thread_tasks);
}

void attach_ybot_render_resources(igecs::WorldView* wv, entt::entity e) {
  // TODO (sessamekesh): Attach AnimatedPbrInstance, geometry, animations...
  // Also attach an animation switcher, so that animations can be hot-swapped
  //  in/out based on whatever state the thingy has.
}

}  // namespace igdemo
