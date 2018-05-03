#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
	mat4 model;
	mat4 view;
	mat4 projection;
} ubo;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_color;
layout(location = 2) in vec2 in_tex_coord;

layout(location = 3) in vec3 in_displacement;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec2 out_texture_coord;

layout(location = 2) out mat4 out_view;



out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(in_position+in_displacement, 1.0);
    out_color = in_color;
    out_texture_coord = in_tex_coord;
    out_view = ubo.view;
}