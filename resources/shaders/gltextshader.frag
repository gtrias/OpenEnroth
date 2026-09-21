#include "precision.glsl"

#ifdef OE_GLSL_LEGACY
varying vec4 colours0;
varying vec4 colours1;
varying vec4 colours2;
varying vec4 colours3;
varying vec2 texuv;
#define FragColour gl_FragColor
#else
in mat4 colours;
in vec2 texuv;
out vec4 FragColour;
#endif

uniform sampler2D texture0;

void main() {
    // Texture channels hold per-channel color weights summing to 1, so this multiplication blends the four
    // per-channel colors passed in the columns of the colours matrix.
#ifdef OE_GLSL_LEGACY
    vec4 texel = texture2D(texture0, texuv);
    FragColour = colours0 * texel.x + colours1 * texel.y + colours2 * texel.z + colours3 * texel.w;
#else
    FragColour = colours * texture(texture0, texuv);
#endif
}
