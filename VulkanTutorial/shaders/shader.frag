#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(binding = 1) uniform sampler2D texture_sampler;

layout(location = 0) in vec3 fragment_color;
layout(location = 1) in vec2 fragment_texture_coordinate;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec3 light_direction;
layout(location = 4) in vec3 light_reflection;
layout(location = 5) in vec3 vert_position;

layout(location = 0) out vec4 outColor;

//general luminosity
vec4 ambient_light_color = vec4(0.3f,0.3f,0.3f,1.0f);
//ocean color
vec4 opaque_color = vec4(0.0f, 0.75f, 0.65f, 1.0f);
//warm sunlight color
vec4 diffuse_color = vec4(0.406f, 0.368f, 0.225f, 1.0f);
//reflection color
vec4 specular_color = vec4(1.0f,1.0f,1.0f,1.0f);

void main() {

    float lambertian = max(dot(light_direction, -normal), 0.0f);
    float specular = 0.0f;

    if(lambertian>0.0f){
        float specAngle = max(dot(light_reflection, normalize(-vert_position)), 0.0f);
        specular = pow(specAngle, 4.0f);
    }
    //add all light sources together
    vec4 color = ambient_light_color + lambertian * diffuse_color + specular * specular_color;
    //multiply with target color
    outColor = color * opaque_color;
}