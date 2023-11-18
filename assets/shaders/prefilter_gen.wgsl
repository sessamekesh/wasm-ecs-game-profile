// Adapted from https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/2.2.2.ibl_specular_textured/2.2.2.prefilter.fs

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

// Bind Group 2 - PBR parameters
struct PbrParams {
  roughness: f32,
  inputTextureResolution: f32,
}
@group(2) @binding(0) var<uniform> pbrParams: PbrParams;

const PI = 3.14159265359;

fn RadicalInverse_VdC(bits_in: u32) -> f32 {
  var bits: u32 = bits_in;
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return f32(bits) * 2.3283064365386963e-10; // / 0x100000000
}

fn Hammersley(i: u32, n: u32) -> vec2f {
  return vec2f(f32(i)/f32(n), RadicalInverse_VdC(i));
}

fn ImportanceSampleGGX(Xi: vec2f, N: vec3f, roughness: f32) -> vec3f {
  let a = roughness * roughness;
  let phi = 2. * PI * Xi.x;
  let cosTheta = sqrt((1. - Xi.y) / (1. + (a * a - 1.) * Xi.y));
  let sinTheta = sqrt(1. - cosTheta * cosTheta);

  // From spherical coordinates to cartesian coordinates - halfway vector
  var H: vec3f;
  H.x = cos(phi) * sinTheta;
  H.y = sin(phi) * sinTheta;
  H.z = cosTheta;

  // From tangent-space H vector to world-space sample vector
  let up = select(vec3f(1., 0., 0.), vec3f(0., 0., 1.), abs(N.z) < 0.999);
  let tangent = normalize(cross(up, N));
  let bitangent = cross(N, tangent);

  let sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
  return normalize(sampleVec);
}

fn DistributionGGX(N: vec3f, H: vec3f, roughness: f32) -> f32 {
  let a = roughness * roughness;
  let a2 = a * a;
  let NdotH = max(dot(N, H), 0.);
  let NdotH2 = NdotH * NdotH;

  var denom = (NdotH2 * (a2 - 1.) + 1.);
  denom = PI * denom * denom;

  return a2 / denom;
}

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
  let roughness = pbrParams.roughness;

  // Make the simplifying assumption that V equals R equals the normal
  let R = N;
  let V = R;

  const SAMPLE_COUNT: u32 = 1024u;
  var prefilteredColor = vec3f(0.);
  var totalWeight: f32 = 0.;

  for (var i = 0u; i < SAMPLE_COUNT; i += 1) {
    // Generates a sample vector that's biased towards the
    //  preferred alighment direction (importance sampling)
    let Xi = Hammersley(i, SAMPLE_COUNT);
    let H = ImportanceSampleGGX(Xi, N, roughness);
    let L = normalize(2. * dot(V, H) * H - V);

    let NdotL = max(dot(N, L), 0.);

    //let sampleValue = textureSampleLevel(cubeTexture, cubeSampler, L, mipLevel);
    let sampleValue = textureSample(cubeTexture, cubeSampler, L);

    if (NdotL > 0.) {
      // Sample from the environment's mip level based on roughness/pdf
      //let D = DistributionGGX(N, H, roughness);
      //let NdotH = max(dot(N, H), 0.);
      //let HdotV = max(dot(H, V), 0.);
      //let pdf = D * NdotH / (4. * HdotV) + 0.0001;

      //let resolution = pbrParams.inputTextureResolution; // Resolution of source cubemap (per face)
      //let saTexel = 4. * PI / (6. * resolution * resolution);
      //let saSample = 1. / (f32(SAMPLE_COUNT) * pdf + 0.0001);

      //let mipLevel = select(0.5 * log2(saSample / saTexel), 0., roughness == 0.);

      prefilteredColor += sampleValue.rgb * NdotL;
      totalWeight += NdotL;
    }
  }

  out.hdr_fragment = vec4f(prefilteredColor / totalWeight, 1.);

  return out;
}
