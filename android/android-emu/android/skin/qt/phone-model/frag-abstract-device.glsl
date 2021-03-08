precision mediump float;

varying vec3 varying_pos;
varying vec3 varying_normal;
varying vec2 varying_uv;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform sampler2D gloss_map;
uniform samplerCube env_map;
uniform float no_light;

const vec3 light_pos = vec3(0.0, 0.0, 5.0);

void main() {
    // Make sure normal is unit length.
    vec3 n = normalize(varying_normal);

    // Direction from the point to the light source,
    // unit length.
    vec3 l = normalize(light_pos - varying_pos);

    // Lambertian
    float lambert = max(dot(l, n), 0.0);
    vec4 diffuse_color = texture2D(diffuse_map, varying_uv);
    gl_FragColor = vec4(lambert * diffuse_color.rgb, diffuse_color.a);
}
