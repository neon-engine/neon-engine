#version 330 core

out vec4 FragColor;

in vec3 frag_coord;
in vec3 normal_coord;

uniform vec3 color;
uniform vec3 light_color;

uniform vec3 light_position;

void main() {
    float ambient_strength = 0.1;
    vec3 ambient = ambient_strength * light_color;
    vec3 result = ambient * color;
    FragColor = vec4(result, 1.0);
//    vec3 norm = normalize(normal_coord);
//    vec3 light_direction = normalize(light_position - frag_coord);
//    float diff = max(dot(norm, light_direction), 0.0);
}