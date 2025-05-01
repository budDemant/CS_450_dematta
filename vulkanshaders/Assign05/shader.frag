#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec4 interPos;
layout(location = 2) in vec3 interNormal;


layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;

struct PointLight {
    vec4 pos;
    vec4 vpos;
    vec4 color;
};

layout(set=0, binding=1) uniform UBOFragment {
    PointLight light;
    float metallic;
    float roughness;
} uboFrag;

// functions
vec3 getFresnelAtAngleZero(vec3 albedo, float metallic) {
    vec3 F0 = vec3(0.04); // Good default value for insulators (0.04)
    
    F0 = mix(F0, albedo, metallic);

    return F0;
}

vec3 getFresnel(vec3 F0, vec3 L, vec3 H) {
    float cosAngle = max(dot(L, H), 0.0);
    // Schlick approximation
    return F0 + (1.0 - F0) * pow(1.0 - cosAngle, 5.0);
}

float getNDF(vec3 H, vec3 N, float roughness) {
    float alpha = roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (alpha * alpha - 1.0) + 1.0); // GGX/Trowbirdge-Reitz NDF
    denom = PI * denom * denom;

    return (alpha * alpha) / denom;
}

float getSchlickGeo(vec3 B, vec3 N, float roughness) {
    float NdotB = max(dot(N, B), 0.0);
    float k = (roughness + 1.0);
    k = (k * k) / 8.0; // Calculate k as (roughness + 1)^2 / 8

    return NdotB / (NdotB * (1.0 - k) + k);
}

float getGF(vec3 L, vec3 V, vec3 N, float roughness) {
    float GL = getSchlickGeo(L, N, roughness);
    float GV = getSchlickGeo(V, N, roughness);
    return GL * GV;
}







void main() {

    vec3 N = normalize(interNormal); // Normalize interNormal → N
    vec3 L = normalize(vec3(uboFrag.light.vpos) - vec3(interPos)); // Calculate light vector L
    vec3 baseColor = vec3(fragColor); 
    vec3 V = normalize(-vec3(interPos)); // Calculate the normalized view vector V
    vec3 F0 = getFresnelAtAngleZero(baseColor, uboFrag.metallic); // Calculate F0
    vec3 H = normalize(L + V); // Calculate normalized half-vector H
    vec3 F = getFresnel(F0, L, H); // Calculate Fresnel reflectance F
    vec3 kS = F; // specular color kS



    // Gamma-correct (linear to sRGB)
    vec4 finalColor = fragColor;
    //finalColor.rgb = pow(finalColor.rgb, vec3(2.2));

    // Output final color
    outColor = finalColor;
} 
