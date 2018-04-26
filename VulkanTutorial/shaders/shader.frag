#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texture_sampler;



layout(location = 0) in vec3 fragment_color;
layout(location = 1) in vec2 fragment_texture_coordinate;
layout(location = 2) in vec3 vertex_normal;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(-vertex_normal, 1.0f); //texture(texture_sampler, fragment_texture_coordinate);
}