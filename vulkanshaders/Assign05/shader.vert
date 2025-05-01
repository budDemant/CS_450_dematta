#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec4 interPos;
layout(location = 2) out vec3 interNormal;


layout(push_constant) uniform PushConstants {
    mat4 modelMat;
    mat4 normMat;
} pc;


// Add the appropriate UBO struct
layout(set = 0, binding = 0) uniform UBOVertex {
    mat4 viewMat;
    mat4 projMat;
} ubo;




void main() {
    vec4 pos = vec4(inPosition, 1.0);
    vec4 vpos = ubo.viewMat * pc.modelMat * pos;
    pos = ubo.projMat * vpos;


    gl_Position = pos;
    fragColor = inColor;

    interPos = vpos;
    interNormal = mat3(pc.normMat) * inNormal;
} 


