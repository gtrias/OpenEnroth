#include "precision.glsl"
#include "fog.glsl"

#ifdef OE_GLSL_LEGACY
varying vec4 vertexColour;
varying vec2 texuv;
varying vec4 viewspace;
#define FragColour gl_FragColor
#else
in vec4 vertexColour;
in vec2 texuv;
//flat in float olayer;
//in vec3 vsPos;
//in vec3 vsNorm;
//flat in int vsAttrib;
in vec4 viewspace;
out vec4 FragColour;
#endif

uniform sampler2D texture0;
uniform FogParam fog;

void main() {
#ifdef OE_GLSL_LEGACY
    vec4 fragcol = texture2D(texture0, texuv) * vertexColour;
#else
    vec4 fragcol = texture(texture0, texuv) * vertexColour;
#endif

    float fograt = getFogRatio(fog, abs(viewspace.z/ viewspace.w));
    if (fragcol.a < 0.004) fograt = 0.0;
    
    FragColour = mix(fragcol, vec4(0.0), fograt);
}
