// Adapted from: https://github.com/JoeyDeVries/LearnOpenGL/blob/master/src/6.pbr/2.2.2.ibl_specular_textured/2.2.2.brdf.vs

struct VertexInput {
  @location(0) position: vec3f,
  @location(1) uv: vec2f,
}

struct FragmentInput {
  @builtin(position) frag_coord: vec4f,
  @location(0) uv: vec2f,
}

struct FragmentOutput {
  @location(0) lut_rsl: vec2f,
}

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

// Geometry function (denoted G above) - what percentage of facets are
//  occluded from any light source because of other "bumpy" facets in the
//  nearby area? E.g., sandpaper will have some subtle occlusion that
//  needs to be respected.
fn GeometrySchlickGGX(NdotV: f32, roughness: f32) -> f32 {
  let a = roughness;
  let k = (a * a) / 2.; // NOTE that k is different for IBL

  let denom = NdotV * (1. - k) + k;
  return NdotV / denom;
}

fn GeometrySmith(N: vec3f, V: vec3f, L: vec3f, roughness: f32) -> f32 {
  let NdotV = max(dot(N, V), 0.);
  let NdotL = max(dot(N, L), 0.);
  let ggx2 = GeometrySchlickGGX(NdotV, roughness);
  let ggx1 = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

fn IntegrateBRDF(NdotV: f32, roughness: f32) -> vec2f {
  var V: vec3f;
  V.x = sqrt(1. - NdotV * NdotV);
  V.y = 0.;
  V.z = NdotV;

  var A: f32 = 0.;
  var B: f32 = 0.;

  let N = vec3f(0., 0., 1.);

  const SAMPLE_COUNT: u32 = 1024u;

  for (var i = 0u; i < SAMPLE_COUNT; i += 1u) {
    // Generates a sample vector that's biased towards the
    //  preferred alighment direction (importance sampling)
    let Xi = Hammersley(i, SAMPLE_COUNT);
    let H = ImportanceSampleGGX(Xi, N, roughness);
    let L = normalize(2. * dot(V, H) * H - V);

    let NdotL = max(L.z, 0.);
    let NdotH = max(H.z, 0.);
    let VdotH = max(dot(V, H), 0.);

    if (NdotL > 0.) {
      let G = GeometrySmith(N, V, L, roughness);
      let G_Vis = (G * VdotH) / (NdotH * NdotV);
      let Fc = pow(1. - VdotH, 5.);

      A += (1. - Fc) * G_Vis;
      B += Fc * G_Vis;
    }
  }

  A /= f32(SAMPLE_COUNT);
  B /= f32(SAMPLE_COUNT);
  return vec2f(A, B);
}

@vertex
fn vs(vin: VertexInput) -> FragmentInput {
  var out: FragmentInput;

  out.frag_coord = vec4f(vin.position, 1.);
  out.uv = vin.uv;

  return out;
}

@fragment
fn fs(fin: FragmentInput) -> FragmentOutput {
  var out: FragmentOutput;

  out.lut_rsl = IntegrateBRDF(fin.uv.x, fin.uv.y);

  return out;
}
