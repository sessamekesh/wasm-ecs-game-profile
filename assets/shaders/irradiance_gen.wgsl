// Adapted from https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/2.2.2.ibl_specular_textured/2.2.2.background.vs

struct VertexInput {
  @location(0) pos: vec3f,
}

struct FragmentInput {
  @builtin(position) frag_coord: vec4f,
  @location(0) world_pos: vec3f,
}

struct FragmentOutput {
  @location(0) hdr_fragment: vec4f,
}

// Bind Group 0 - camera params
struct CameraParamsUbo {
  mat_view: mat4x4f,
  mat_proj: mat4x4f,
}
@group(0) @binding(0) var<uniform> cameraParams: CameraParamsUbo;

// Bind Group 1 - HDR environment cubemap
@group(1) @binding(0) var cubeSampler: sampler;
@group(1) @binding(1) var cubeTexture: texture_cube<f32>;

const PI = 3.14159265359;

@vertex
fn vs(vin: VertexInput) -> FragmentInput {
  var out: FragmentInput;

  out.world_pos = vin.pos;
  out.frag_coord = cameraParams.mat_proj * cameraParams.mat_view * vec4(vin.pos, 1.0);

  return out;
}

@fragment
fn fs(fin: FragmentInput) -> FragmentOutput {
  var out: FragmentOutput;

  let N = normalize(fin.world_pos);

  var irradiance = vec3f(0.);

  // Tangent space calculation from origin point
  var up = vec3(0., 1., 0.);
  let right = normalize(cross(up, N));
  up = normalize(cross(N, right));

  let sampleDelta: f32 = 0.025;
  var nrSamples: f32 = 0.;
  for (var phi: f32 = 0.; phi < 2. * PI; phi += sampleDelta) {
    for (var theta: f32 = 0.; theta < 0.5 * PI; theta += sampleDelta) {
      let tangentSample = vec3f(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      let sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;

      irradiance += textureSample(cubeTexture, cubeSampler, sampleVec).rgb * cos(theta) * sin(theta);
      nrSamples += 1.;
    }
  }

  out.hdr_fragment = vec4f(PI * irradiance * (1. / nrSamples), 1.);

  return out;
}
