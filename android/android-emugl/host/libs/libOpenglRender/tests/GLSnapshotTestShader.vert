attribute vec4 position;
uniform mat4 projection;
uniform mat4 transform;
uniform mat4 screenSpace;
varying float linear;

void main(void) {
    vec4 transformedPosition = projection * transform * position;
    gl_Position = transformedPosition;
    linear = (screenSpace * position).x;
}
