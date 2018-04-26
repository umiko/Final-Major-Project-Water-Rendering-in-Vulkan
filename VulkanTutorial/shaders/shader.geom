#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(triangles) in;
layout(triangle_strip, max_vertices=3) out;

layout(location = 0) in vec3[3] in_color;
layout(location = 1) in vec2[3] in_texture_coord;

layout(location = 0) out vec3 out_color;
layout(location = 1) out vec2 out_texture_coord;
layout(location = 2) out vec3 out_vertex_normal;

void main(void){
	vec3 a = (gl_in[1].gl_Position - gl_in[0].gl_Position).xyz;
	vec3 b = (gl_in[2].gl_Position - gl_in[0].gl_Position).xyz;
	vec3 normal = normalize(cross(a,b));

	for(int i = 0; i<gl_in.length(); i++){
		gl_Position = gl_in[i].gl_Position;
		out_vertex_normal = normal;
		out_color = in_color[i];
		out_texture_coord = in_texture_coord[i];
		EmitVertex();
	}
	EndPrimitive();
}