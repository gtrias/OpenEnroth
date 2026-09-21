#include "precision.glsl"
#include "attribmask.glsl"
#include "fog.glsl"
#include "lighting.glsl"

#ifdef OE_GLSL_LEGACY
varying vec4 vertexColour;
varying vec2 texuv;
varying float olayer;
varying vec3 vsPos;
varying vec3 vsNorm;
varying float vsAttrib;
varying vec4 viewspace;
#define FragColour gl_FragColor
#else
in vec4 vertexColour;
in vec2 texuv;
flat in float olayer;
in vec3 vsPos;
in vec3 vsNorm;
flat in int vsAttrib;
in vec4 viewspace;

out vec4 FragColour;
#endif

uniform int waterframe;
uniform Sunlight sun;
uniform vec3 CameraPos;
uniform float gamma;

#define num_point_lights 20
uniform PointLight fspointlights[num_point_lights];

#ifdef OE_GLSL_LEGACY
uniform sampler2D textureArray0;
uniform float waterLayersScale;
uniform sampler2D textureArray1;
uniform float tileLayersScale;
#else
uniform sampler2DArray textureArray0;
uniform sampler2DArray textureArray1;
#endif
uniform FogParam fog;

void main() {
    vec3 fragnorm = normalize(vsNorm);
    vec3 fragviewdir = normalize(CameraPos - vsPos);

    // get water textures at point
#ifdef OE_GLSL_LEGACY
    vec4 watercol = texture2D(textureArray0, vec2(texuv.x, (fract(texuv.y) + float(waterframe)) * waterLayersScale));
#else
    vec4 watercol = texture(textureArray0, vec3(texuv.x,texuv.y,waterframe));
#endif

    vec4 fragcol = vec4(0);

    // get normal texture at point
#ifdef OE_GLSL_LEGACY
    fragcol = texture2D(textureArray1, vec2(texuv.x, (fract(texuv.y) + olayer) * tileLayersScale));
#else
    fragcol = texture(textureArray1, vec3(texuv.x,texuv.y,olayer));
#endif

    // replace texture with water if alpha or a water tile (bit 0x1 in attribs)
#ifdef OE_GLSL_LEGACY
    if (fragcol.a == 0.0 || attribMaskSet(vsAttrib, 1.0, 2.0)){
#else
    if (fragcol.a == 0.0 || (vsAttrib & 0x1) > 0){
#endif
        fragcol = watercol;
    }

    // apply sun
    vec3 result = CalcSunLight(sun, fragnorm, fragviewdir, vec3(1));
    result = clamp(result, 0.0, 0.85);

    // stack torchlight if any
    result += CalcPointLight(fspointlights[0], fragnorm, vsPos, fragviewdir);

    // stack stationary lights
    for(int i = 1; i < num_point_lights; i++) {
        if (fspointlights[i].type == 1.0)
            result += CalcPointLight(fspointlights[i], fragnorm, vsPos, fragviewdir);
    }

    result *= fragcol.rgb;

    // stack mobile lights
    for(int i = 1; i < num_point_lights; i++) {
        if (fspointlights[i].type == 2.0)
            result += CalcPointLight(fspointlights[i], fragnorm, vsPos, fragviewdir);
    }

    vec3 clamps = result;

    float dist = length(viewspace);
    float alpha = getFogAlpha(fog, dist);

    float fograt = getFogRatio(fog, dist);
    FragColour = mix(vec4(clamps, vertexColour.a), vec4(fog.color, alpha), fograt);
    FragColour.rgb = pow(FragColour.rgb, vec3(1.0/gamma));
}
