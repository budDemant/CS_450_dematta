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

vec3 getFresnelAtAngleZero(vec3 albedo, float metallic) {
    vec3 F0 = vec3(0.04); // Good default value for insulators (0.04)
    
    F0 = mix(F0, albedo, metallic);

    return F0;
}



void main() {

    // Gamma-correct (linear to sRGB)
    vec4 finalColor = fragColor;
    //finalColor.rgb = pow(finalColor.rgb, vec3(2.2));

    // Output final color
    outColor = finalColor;
} 
