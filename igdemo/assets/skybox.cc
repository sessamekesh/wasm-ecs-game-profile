#include <igasset/igpack_decoder.h>
#include <igasync/promise_combiner.h>
#include <igdemo/assets/skybox.h>
#include <igdemo/render/geo/cube.h>
#include <igdemo/render/wgpu-helpers.h>

namespace igdemo {

std::shared_ptr<igasync::Promise<std::vector<std::string>>> load_skybox(
    const IgdemoProcTable& procs, entt::registry* r,
    std::string asset_root_path, const wgpu::Device& device,
    const wgpu::Queue& queue,
    std::shared_ptr<igasync::ExecutionContext> main_thread_tasks,
    std::shared_ptr<igasync::ExecutionContext> compute_tasks) {
  auto wv = igecs::WorldView::Thin(r);
  wv.attach_ctx<CubemapUnitCube>(device, queue);

  auto igpack_decoder_promise =
      procs.loadFileCb(asset_root_path + "skybox.igpack")
          ->then_consuming(
              [](std::variant<std::string, FileReadError> rsl) {
                if (std::holds_alternative<FileReadError>(rsl)) {
                  return std::string("");
                }

                return std::get<std::string>(rsl);
              },
              compute_tasks)
          ->then_consuming(igasset::IgpackDecoder::Create, compute_tasks);

  auto equirect_to_cubemap_promise = igpack_decoder_promise->then(
      [device](std::shared_ptr<igasset::IgpackDecoder> decoder)
          -> std::optional<EquirectangularToCubemapPipeline> {
        if (!decoder) {
          return {};
        }

        auto wgsl_source_rsl =
            decoder->extract_wgsl_shader("equirectToCubemapWgsl");
        if (std::holds_alternative<igasset::IgpackExtractError>(
                wgsl_source_rsl)) {
          return {};
        }

        const auto* wgsl_source =
            std::get<const IgAsset::WgslSource*>(wgsl_source_rsl);

        return EquirectangularToCubemapPipeline::Create(device, wgsl_source);
      },
      main_thread_tasks);

  auto skybox_hdr_promise = igpack_decoder_promise->then(
      [](std::shared_ptr<igasset::IgpackDecoder> decoder)
          -> std::optional<igasset::RawHdr> {
        if (!decoder) return {};
        auto rsl = decoder->extract_raw_hdr("sunsetSkyboxHdr");
        if (std::holds_alternative<igasset::IgpackExtractError>(rsl)) return {};
        return std::get<igasset::RawHdr>(std::move(rsl));
      },
      compute_tasks);

  auto skybox_hdr_texture_promise = skybox_hdr_promise->then_consuming(
      [device, queue](std::optional<igasset::RawHdr> hdr)
          -> std::optional<TextureWithMeta> {
        if (!hdr) return {};

        wgpu::TextureDescriptor td{};
        td.dimension = wgpu::TextureDimension::e2D;
        td.format = wgpu::TextureFormat::RGBA16Float;
        td.label = "skybox-hdr-equirect-texture";
        td.mipLevelCount = 1;
        td.size = wgpu::Extent3D{hdr->width(), hdr->height(), 1};
        td.usage =
            wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding;

        auto texture = device.CreateTexture(&td);

        wgpu::ImageCopyTexture wd{};
        wd.texture = texture;

        wgpu::TextureDataLayout tdl{};
        tdl.bytesPerRow =
            sizeof(numeric::float16_t) * hdr->width() * hdr->num_channels();
        tdl.rowsPerImage = hdr->height();

        wgpu::Extent3D ws = wgpu::Extent3D{hdr->width(), hdr->height(), 1};

        queue.WriteTexture(&wd, hdr->get_data(), hdr->size(), &tdl, &ws);

        TextureWithMeta tr{};
        tr.format = wgpu::TextureFormat::RGBA16Float;
        tr.width = hdr->width();
        tr.height = hdr->height();
        tr.texture = texture;
        return tr;
      },
      main_thread_tasks);

  auto skybox_maps_combiner = igasync::PromiseCombiner::Create();
  auto smc_skybox_hdr_key =
      skybox_maps_combiner->add(skybox_hdr_texture_promise, main_thread_tasks);
  auto smc_equirect_to_cubemap_key =
      skybox_maps_combiner->add(equirect_to_cubemap_promise, main_thread_tasks);
  auto skybox_cubemap_promise = skybox_maps_combiner->combine(
      [device, queue, smc_skybox_hdr_key, smc_equirect_to_cubemap_key,
       r](igasync::PromiseCombiner::Result rsl)
          -> std::optional<TextureWithMeta> {
        auto& skybox_hdr_rsl = rsl.get(smc_skybox_hdr_key);
        auto& equirect_to_cubemap_rsl = rsl.get(smc_equirect_to_cubemap_key);

        if (!skybox_hdr_rsl || !equirect_to_cubemap_rsl) {
          return {};
        }

        auto& skybox_hdr = *skybox_hdr_rsl;
        auto& equirect_to_cubemap = *equirect_to_cubemap_rsl;

        const int kWidth = 512;

        auto wv = igecs::WorldView::Thin(r);

        auto conversion_output = equirect_to_cubemap.convert(
            device, queue, skybox_hdr.texture.CreateView(),
            wv.ctx<CubemapUnitCube>(), kWidth,
            wgpu::TextureUsage::TextureBinding, "hdr-skybox-texture");

        TextureWithMeta t{};
        t.texture = conversion_output.cubemap;
        t.width = kWidth;
        t.height = kWidth;
        t.format = wgpu::TextureFormat::RGBA16Float;
        return t;
      },
      main_thread_tasks);

  auto final_combiner = igasync::PromiseCombiner::Create();
  auto fc_skybox_cubemap_key =
      final_combiner->add(skybox_cubemap_promise, main_thread_tasks);

  return final_combiner->combine(
      [r, fc_skybox_cubemap_key](
          igasync::PromiseCombiner::Result rsl) -> std::vector<std::string> {
        auto& skybox_cubemap_rsl = rsl.get(fc_skybox_cubemap_key);

        std::vector<std::string> errors;

        if (!skybox_cubemap_rsl) {
          errors.push_back("Skybox cubemap");
        }

        if (errors.size() > 0) {
          return errors;
        }

        auto wv = igecs::WorldView::Thin(r);
        wv.attach_ctx<CtxHdrSkybox>(CtxHdrSkybox{
            skybox_cubemap_rsl->texture,
            skybox_cubemap_rsl->texture.CreateView(),
        });

        return errors;
      },
      main_thread_tasks);
}

}  // namespace igdemo
