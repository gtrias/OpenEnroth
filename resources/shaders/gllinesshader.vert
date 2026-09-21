#ifdef OE_GLSL_LEGACY
attribute vec3 vaPos;
attribute vec4 vaCol;

varying vec4 colour;
#else
layout (location = 0) in vec3 vaPos;
layout (location = 1) in vec4 vaCol;

out vec4 colour;
#endif

uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 adjpos = view * vec4(vaPos, 1.0);
    adjpos.x += 0.5;
    adjpos.y += 0.5;
    gl_Position = projection * adjpos; 
    colour = vaCol;
}
