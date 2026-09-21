#include "precision.glsl"
#include "fog.glsl"

#ifdef OE_GLSL_LEGACY
varying vec4 colour;
varying vec4 texuv;
varying float screenspace;
#define FragColour gl_FragColor
#else
in vec4 colour;
in vec4 texuv;
in float screenspace;

out vec4 FragColour;
#endif

uniform sampler2D texture0;
uniform FogParam fog;

void main() {
#ifdef OE_GLSL_LEGACY
    vec4 fragcol = texture2DProj(texture0, texuv) * colour;
#else
    vec4 fragcol = textureProj(texture0, texuv) * colour;
#endif

    float fograt = getFogRatio(fog, screenspace);
    if (fragcol.a < 0.004) fograt = 0.0;

    float alpha = 1.0; // Not getFogAlpha(fog, screenspace) b/c we need foggy sky.

    FragColour = mix(fragcol, vec4(fog.color, alpha), fograt);
}
