#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragment_color;
layout(location = 1) in vec2 fragment_tex_coord;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D tex_sampler;

void main() {
    outColor = vec4(fragment_color * texture(tex_sampler, fragment_tex_coord).rgb, 1.0);
}