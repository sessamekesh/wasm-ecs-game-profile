// ACES mapping implementation from https://64.github.io/tonemapping/
//  Which in turn got it from https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
const aces_input_matrix = mat3x3f(
  vec3f(0.59719, 0.35458, 0.04823),
  vec3f(0.07600, 0.90834, 0.01566),
  vec3f(0.02840, 0.13383, 0.83777));

const aces_output_matrix = mat3x3f(
  vec3( 1.60475, -0.53108, -0.07367),
  vec3(-0.10208,  1.10813, -0.00605),
  vec3(-0.00327, -0.07276,  1.07602));

fn rtt_and_odt_fit(v: vec3f) -> vec3f {
  let a = v * (v + 0.0245786) - 0.000090537;
  let b = v * (0.983729 * v + 0.4329510) + 0.238081;
  return a / b;
}

fn aces_fitted(v: vec3f) -> vec3f {
  return aces_output_matrix * rtt_and_odt_fit(aces_input_matrix * v);
}

fn linear_to_srgb(color: vec3f) -> vec3f {
  let x = color * 12.92f;
  let y = 1.055f * pow(saturate(color), vec3f(1. / 2.4)) - vec3f(0.055);

  var clr = color;
  clr.r = select(y.r, x.r, color.r < 0.0031308f);
  clr.g = select(y.g, x.g, color.g < 0.0031308f);
  clr.b = select(y.b, x.b, color.b < 0.0031308f);
  return clr;
}

struct VertexInput {
  @location(0) position: vec2f,
}

struct FragmentInput {
  @builtin(position) frag_coord: vec4f,
  @location(0) uv: vec2f,
}

struct FragmentOutput {
  @location(0) ldr_fragment: vec4f,
}

@group(0) @binding(0) var hdrSampler: sampler;
@group(0) @binding(1) var hdrTexture: texture_2d<f32>;

@vertex
fn vs(vertex: VertexInput) -> FragmentInput {
  var out: FragmentInput;
  out.frag_coord = vec4f(vertex.position, 1.0, 1.0);
  out.uv = (vec2f(vertex.position.x, -vertex.position.y) + 1.) / 2.;
  return out;
}

@fragment
fn fs(frag: FragmentInput) -> FragmentOutput {
  var out: FragmentOutput;

  let uv = frag.uv;
  let hdr_color = textureSample(hdrTexture, hdrSampler, uv);

  // Multiply by 1.8? Dunno, give it a go and see how it looks I guess.
  // https://github.com/TheRealMJP/BakingLab/blob/6677ddbd5f90a0548e74647dd37753b2858f376d/BakingLab/ToneMapping.hlsl#L105

  // Exposure filtiner!
  let exposure_adjusted = hdr_color.xyz * 1. / 6.;

  let ldr_linear = aces_fitted(exposure_adjusted);
  let ldr_srgb = linear_to_srgb(ldr_linear);
  let ldr_gamma_corrected = pow(ldr_srgb, vec3f(1. / 2.2));
  out.ldr_fragment = vec4f(ldr_gamma_corrected, 1.);

  return out;
}
