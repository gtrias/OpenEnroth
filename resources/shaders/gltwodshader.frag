#include "precision.glsl"

#ifdef OE_GLSL_LEGACY
varying vec4 colour;
varying vec2 texuv;
varying float paletteid;
#define FragColour gl_FragColor
#else
in vec4 colour;
in vec2 texuv;
flat in int paletteid;

out vec4 FragColour;
#endif

uniform sampler2D texture0;
uniform sampler2D paltex2D;
#ifdef OE_GLSL_LEGACY
uniform vec2 paletteSize; // texelFetch() is not available, so the palette size is passed in explicitly.
#endif

void main() {
#ifdef OE_GLSL_LEGACY
    vec4 fragcol = texture2D(texture0, texuv);
#else
    vec4 fragcol = texture(texture0, texuv);
#endif
    int index = int(fragcol.r * 255.0);
#ifdef OE_GLSL_LEGACY
    vec4 newcol = vec4(texture2D(paltex2D, (vec2(float(index), float(int(paletteid))) + 0.5) / paletteSize));
#else
    vec4 newcol = vec4(texelFetch(paltex2D, ivec2(index, paletteid), 0));
#endif

#ifdef OE_GLSL_LEGACY
    if (int(paletteid) > 0)
#else
    if (paletteid > 0)
#endif
        if (index > 0)
            fragcol = vec4(newcol.r, newcol.g, newcol.b, 1.0);

    FragColour =  fragcol * colour;
}
