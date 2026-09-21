#ifdef OE_GLSL_LEGACY
attribute vec3 vaPos;
attribute vec2 vaTexUV;
attribute vec4 vaCol;

attribute float palid;
varying vec4 colour;
varying vec2 texuv;
varying float paletteid;
#else
layout (location = 0) in vec3 vaPos;
layout (location = 1) in vec2 vaTexUV;
layout (location = 2) in vec4 vaCol;

layout (location = 4) in float palid;


out vec4 colour;
out vec2 texuv;
flat out int paletteid;
#endif

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(vaPos, 1.0);
    colour = vaCol;
    texuv = vaTexUV;
#ifdef OE_GLSL_LEGACY
    paletteid = palid;
#else
    paletteid = int(palid);
#endif
}
