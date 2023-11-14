// ---------------- Standard PBR functions ----------------

const PI = 3.14159265359;

// f(CookTorrance) = (D * F * G) / (4(omega_0 * n)(omega_i * n))

// Normal Distribution function (denoted as D above) - approximates the
//  percentage of the surface's microfacets that are aligned with the
//  halfway vector. "Smooth" objects will have many/most of their facets
//  aligned with the halfway vector - e.g. a mirror will tend to reflect
//  in a very focused way, as opposed to rough fogged glass.
fn DistributionGGX(N: vec3f, H: vec3f, roughness: f32) -> f32 {
  let a = roughness * roughness;
  let a2 = a * a;
  let NdotH = max(dot(N, H), 0.);
  let NdotH2 = NdotH * NdotH;

  var denom = (NdotH2 * (a2 - 1.) + 1.);
  denom = PI * denom * denom;

  return a2 / denom;
}

// Geometry function (denoted G above) - what percentage of facets are
//  occluded from any light source because of other "bumpy" facets in the
//  nearby area? E.g., sandpaper will have some subtle occlusion that
//  needs to be respected.
fn GeometrySchlickGGX(NdotV: f32, roughness: f32) -> f32 {
  let r = (roughness + 1.);
  let k = (r * r) / 8.;

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

// Fresnel function (denoted F above) - provides the proportion of
//  light that is reflected vs. refracted. Metallic objects will
//  reflect, while dielectrics will have variation.
fn fresnelSchlick(cosTheta: f32, F0: vec3f) -> vec3f {
  return F0 + (1. - F0) * pow(clamp(1. - cosTheta, 0., 1.), 5.);
}

fn fresnelSchlickRoughness(cosTheta: f32, F0: vec3f, roughness: f32) -> vec3f {
  return F0 + (max(vec3f(1. - roughness), F0) - F0) * pow(clamp(1. - cosTheta, 0., 1.), 5.);
}

// ---------------- End PBR common code ----------------

struct VertexInput {
  @location(0) position: vec3f,
  @location(1) normal: vec3f,
  @location(2) bone_weights: vec4f,
  @location(3) bone_indices: vec4u
}

struct VertexOutput {
  @builtin(position) frag_coord: vec4f,
  @location(0) world_pos: vec3f,
  @location(1) world_normal: vec3f
}

struct CameraParamsUbo {
  mat_view_proj: mat4x4f,
  camera_pos: vec3f
}

struct LightingParams {
  sun_color: vec3f,
  ambient_coefficient: f32,
  sun_direction: vec3f
}

struct PbrColorParams {
  albedo: vec3f,
  metallic: f32,
  roughness: f32
}

struct SkinningParams {
  data: array<mat4x4f, 80>
}

@group(0) @binding(0) var<uniform> cameraParams: CameraParamsUbo;
@group(0) @binding(1) var<uniform> lightingParams: LightingParams;
@group(1) @binding(0) var<uniform> colorParams: PbrColorParams;
@group(2) @binding(0) var<uniform> skinMatrices: SkinningParams;
@group(2) @binding(1) var<uniform> matWorld: mat4x4f;

@vertex
fn vs(vertex: VertexInput) -> VertexOutput {
  var out: VertexOutput;

  let skin_transform = mat4x4f(
    vertex.bone_weights.x * skinMatrices.data[vertex.bone_indices.x] +
    vertex.bone_weights.y * skinMatrices.data[vertex.bone_indices.y] +
    vertex.bone_weights.z * skinMatrices.data[vertex.bone_indices.z] +
    vertex.bone_weights.w * skinMatrices.data[vertex.bone_indices.w]);

  out.world_pos = (matWorld * skin_transform * vec4f(vertex.position, 1.)).xyz;
  out.world_normal = normalize((matWorld * skin_transform * vec4(normalize(vertex.normal), 0.))).xyz;
  out.frag_coord = cameraParams.mat_view_proj * vec4f(out.world_pos, 1.);

  return out;
}

@fragment
fn fs(frag: VertexOutput) -> @location(0) vec4f {
  let camera_position = cameraParams.camera_pos;

  let N = frag.world_normal;
  let V = normalize(camera_position - frag.world_pos);
  let R = reflect(-V, N);

  let base_albedo = colorParams.albedo;
  let base_metal = colorParams.metallic;
  let base_roughness = colorParams.roughness;

  // Calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 of 0.04 and if
  //  it's metal, use the albedo color as F0 (metallic workflow)
  let F0 = mix(vec3f(0.04), base_albedo, base_metal);

  // Lighting considers only the sun for this shader
  let L = normalize(-lightingParams.sun_direction);
  let H = normalize(V + L);
  let radiance = lightingParams.sun_color;

  // Cook-Torrance BRDF
  let NDF = DistributionGGX(N, H, base_roughness);
  let G = GeometrySmith(N, V, L, base_roughness);
  let F = fresnelSchlick(max(dot(H, V), 0.), F0);

  let numerator = NDF * G * F;
  let denominator = 4. * max(dot(N, V), 0.) * max(dot(N, L), 0.) + 0.0001;
  let specular = numerator / denominator;

  // kS is equal to Fresnel
  let kS = F;

  // For energy conservation, the diffuse and specular light can't be above 1.
  // To preserve this relationship, the diffuse component (kD) should be 1. - kS
  var kD = vec3f(1.) - kS;

  // Multiply kD by the inverse metalness - this way only non-metals have
  //  diffuse lighting, and a linear blend from metal to non-metal can be simulated.
  kD = kD * (1. - base_metal);

  // Scale light by NdotL
  let NdotL = max(dot(N, L), 0.);

  let Lo = (kD * base_albedo / PI + specular) * radiance * NdotL;

  let ambient = lightingParams.sun_color * lightingParams.ambient_coefficient * base_albedo;
  let hdr_color = ambient + Lo;

  return vec4f(hdr_color, 1.);
}
