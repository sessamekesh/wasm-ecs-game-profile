#include <igasset/igpack_decoder.h>
#include <igasync/promise_combiner.h>
#include <igdemo/assets/core-shaders.h>
#include <igdemo/render/animated-pbr.h>
#include <igdemo/render/processing/equirect-to-cubemap.h>
#include <igdemo/systems/pbr-geo-pass.h>
#include <igdemo/systems/tonemap-pass.h>

namespace igdemo {

std::shared_ptr<igasync::Promise<std::vector<std::string>>> load_core_shaders(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue, wgpu::TextureFormat output_format,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks) {
  auto decoder_promise =
      procs.loadFileCb(asset_root_path + "shaders.igpack")
          ->then_consuming(
              [](std::variant<std::string, FileReadError> rsl) {
                if (std::holds_alternative<FileReadError>(rsl)) {
                  return std::string("");
                }
                return std::get<std::string>(rsl);
              },
              compute_tasks)
          ->then_consuming(igasset::IgpackDecoder::Create, compute_tasks)
          // Hack for WASM - move the igpack decoder object to the main thread
          //  to ensure that queueing with the device reference happens from the
          //  main thread
          ->then_consuming([](auto r) { return r; }, main_thread_tasks);

  auto pbr_animated_setup_promise = decoder_promise->then(
      [device, r](std::shared_ptr<igasset::IgpackDecoder> decoder) {
        if (!decoder) {
          return false;
        }

        auto wv = igecs::WorldView::Thin(r);
        return PbrGeoPassSystem::setup_animated(device, &wv, *decoder,
                                                "pbrAnimatedWgsl");
      },
      main_thread_tasks);

  auto tonemap_system_setup_promise = decoder_promise->then(
      [device, queue, output_format,
       r](std::shared_ptr<igasset::IgpackDecoder> decoder) {
        if (!decoder) {
          return false;
        }

        auto rsl = decoder->extract_wgsl_shader("acesTonemappingWgsl");
        if (std::holds_alternative<igasset::IgpackExtractError>(rsl)) {
          return false;
        }

        auto wv = igecs::WorldView::Thin(r);
        const auto* src = std::get<const IgAsset::WgslSource*>(rsl);
        return TonemapPass::setup(&wv, device, queue, src, output_format);
      },
      main_thread_tasks);

  auto combiner = igasync::PromiseCombiner::Create();

  auto pbr_animated_setup_rsl_key =
      combiner->add_consuming(pbr_animated_setup_promise, main_thread_tasks);
  auto tonemap_setup_rsl_key =
      combiner->add_consuming(tonemap_system_setup_promise, main_thread_tasks);

  return combiner->combine(
      [pbr_animated_setup_rsl_key, tonemap_setup_rsl_key,
       r](igasync::PromiseCombiner::Result rsl) -> std::vector<std::string> {
        std::vector<std::string> errors;

        auto pbr_animated_rsl = rsl.move(pbr_animated_setup_rsl_key);
        auto tonemap_setup_rsl = rsl.move(tonemap_setup_rsl_key);

        if (!pbr_animated_rsl) {
          errors.push_back("pbrAnimatedWgsl");
        }

        if (!tonemap_setup_rsl) {
          errors.push_back("acesTonemappingSystem");
        }

        if (errors.size() > 0) {
          return errors;
        }

        return errors;
      },
      main_thread_tasks);
}

}  // namespace igdemo
