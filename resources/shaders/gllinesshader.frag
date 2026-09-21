#include "precision.glsl"

#ifdef OE_GLSL_LEGACY
varying vec4 colour;
#define FragColour gl_FragColor
#else
in vec4 colour;

out vec4 FragColour;
#endif

void main() {
    FragColour = colour;
}
