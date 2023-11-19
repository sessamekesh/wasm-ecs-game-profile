// Adapted from https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/2.2.2.ibl_specular_textured/2.2.2.background.vs

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

struct CameraParamsUbo {
  mat_view: mat4x4f,
  mat_proj: mat4x4f,
}

// Bind Group 0 - camera params
@group(0) @binding(0) var<uniform> cameraParams: CameraParamsUbo;

// Bind Group 1 - skybox sampler
@group(1) @binding(0) var cubeSampler: sampler;
@group(1) @binding(1) var cubeTexture: texture_cube<f32>;

@vertex
fn vs(vin: VertexInput) -> FragmentInput {
  var out: FragmentInput;

  out.world_pos = vin.vertex_position;
  out.world_pos.y *= -1.;

  // set w=0 to only apply rotation (no camera position offset)
  let view_pos = cameraParams.mat_view * vec4(vin.vertex_position, 0.0);
  out.frag_coord = cameraParams.mat_proj * vec4(view_pos.xyz, 1.0);

  return out;
}

@fragment
fn fs(vin: FragmentInput) -> FragmentOutput {
  var out: FragmentOutput;

  out.hdr_fragment = textureSample(cubeTexture, cubeSampler, vin.world_pos);

  return out;
}
