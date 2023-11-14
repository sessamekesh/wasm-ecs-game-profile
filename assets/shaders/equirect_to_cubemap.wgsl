// Adapted from https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/2.2.2.ibl_specular_textured/2.2.2.equirectangular_to_cubemap.fs

struct VertexInput {
  @location(0) vertex_position: vec3f,
}

struct FragmentInput {
  @builtin(position) frag_coord: vec4f,
  @location(0) world_pos: vec3f,
}

struct FragmentOutput {
  @location(0) hdr_fragment: vec4f,
}

@group(0) @binding(0) var<uniform> matViewProj: mat4x4f;
@group(1) @binding(0) var equirectSampler: sampler;
@group(1) @binding(1) var equirectTexture: texture_2d<f32>;

@vertex
fn vs(vin: VertexInput) -> FragmentInput {
  let vertexPosition = vin.vertex_position;

  var out: FragmentInput;

  out.world_pos = vertexPosition;
  out.frag_coord = matViewProj * vec4(vertexPosition, 1.0);

  return out;
}

const invAtan = vec2f(0.1591, 0.3183);

fn SampleSphericalMap(v: vec3f) -> vec2f {
  var uv = vec2f(atan(v.z, v.x), asin(v.y));
  uv *= invAtan;
  uv += vec2f(0.5);
  return uv;
}

@fragment
fn fs(fragment: FragmentInput) -> FragmentOutput {
  let uv = SampleSphericalMap(normalize(fragment.world_pos));
  let color = textureSample(equirectTexture, equirectSampler, uv);

  var out: FragmentOutput;
  out.hdr_fragment = vec4f(color, 1.);
  return out;
}
