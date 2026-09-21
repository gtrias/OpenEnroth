#ifdef OE_GLSL_LEGACY
attribute vec3 vaPos;
attribute vec2 vaTexUV;
// vitaGL gives a mat4 attribute/varying a single varying slot, which leaves the generated program inconsistent, so
// the per-channel colours are passed as four separate vec4s instead. They use the same attribute slots (2..5) as the
// mat4 does on the platforms that support it.
attribute vec4 vaColour0;
attribute vec4 vaColour1;
attribute vec4 vaColour2;
attribute vec4 vaColour3;

varying vec4 colours0;
varying vec4 colours1;
varying vec4 colours2;
varying vec4 colours3;
varying vec2 texuv;
#else
layout (location = 0) in vec3 vaPos;
layout (location = 1) in vec2 vaTexUV;
layout (location = 2) in mat4 vaColours; // One color per texture channel, in columns.

out mat4 colours;
out vec2 texuv;
#endif

uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * vec4(vaPos, 1.0);
#ifdef OE_GLSL_LEGACY
    colours0 = vaColour0;
    colours1 = vaColour1;
    colours2 = vaColour2;
    colours3 = vaColour3;
#else
    colours = vaColours;
#endif
    texuv = vaTexUV;
}
