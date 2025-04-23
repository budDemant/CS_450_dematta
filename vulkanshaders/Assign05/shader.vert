#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    mat4 modelMat;
} pc;

// Add the appropriate UBO struct
layout(set = 0, binding = 0) uniform UBOVertex {
    mat4 viewMat;
    mat4 projMat;
} ubo;

void main() {
    vec4 pos = vec4(inPosition, 1.0);
    pos = ubo.projMat * ubo.viewMat * pc.modelMat * pos;

    gl_Position = pos;
    fragColor = inColor;
} 


