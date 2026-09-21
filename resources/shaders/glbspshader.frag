#include "precision.glsl"
#include "attribmask.glsl"
#include "lighting.glsl"

#ifdef OE_GLSL_LEGACY
varying vec4 vertexColour;
varying vec2 texuv;
varying float olayer;
varying vec3 vsPos;
varying vec3 vsNorm;
varying float vsAttrib;
#define FragColour gl_FragColor
#else
in vec4 vertexColour;
in vec2 texuv;
flat in float olayer;
in vec3 vsPos;
in vec3 vsNorm;
flat in int vsAttrib;

out vec4 FragColour;
#endif

uniform int waterframe;
uniform Sunlight sun;
uniform vec3 CameraPos;
uniform int flowtimer;
uniform int flowtimerms; // TODO(Nik-RE-dev): use a single timer for everything
uniform int watertiles;
uniform float gamma;

#define num_point_lights 40
uniform PointLight fspointlights[num_point_lights];

#ifdef OE_GLSL_LEGACY
uniform sampler2D textureArray0;
uniform float textureLayersScale;
uniform vec2 textureArraySize;
#else
uniform sampler2DArray textureArray0;
#endif

void main() {
    vec3 fragnorm = normalize(vsNorm);
    vec3 fragviewdir = normalize(CameraPos - vsPos);

    vec4 fragcol = vec4(1.0);
    vec2 texcoords = vec2(0.0);
    vec2 texuvmod = vec2(0.0);
    vec2 deltas = vec2(0.0);
#ifdef OE_GLSL_LEGACY
    vec2 texsize = textureArraySize;
#else
    ivec3 texsize = textureSize(textureArray0,0);
#endif

    // texture flow mods
    if (abs(vsNorm.z) >= 0.9) {
#ifdef OE_GLSL_LEGACY
        if (attribMaskSet(vsAttrib, 1024.0, 2.0)) texuvmod.y = 1.0;
#else
        if ((vsAttrib & 0x400) > 0) texuvmod.y = 1.0;
#endif
#ifdef OE_GLSL_LEGACY
        if (attribMaskSet(vsAttrib, 2048.0, 2.0)) texuvmod.y = -1.0;
#else
        if ((vsAttrib & 0x800) > 0) texuvmod.y = -1.0;
#endif
    } else {
#ifdef OE_GLSL_LEGACY
        if (attribMaskSet(vsAttrib, 1024.0, 2.0)) texuvmod.y = -1.0;
#else
        if ((vsAttrib & 0x400) > 0) texuvmod.y = -1.0;
#endif
#ifdef OE_GLSL_LEGACY
        if (attribMaskSet(vsAttrib, 2048.0, 2.0)) texuvmod.y = 1.0;
#else
        if ((vsAttrib & 0x800) > 0) texuvmod.y = 1.0;
#endif
    }

#ifdef OE_GLSL_LEGACY
    if (attribMaskSet(vsAttrib, 4096.0, 2.0)) {
#else
    if ((vsAttrib & 0x1000) > 0) {
#endif
        texuvmod.x = -1.0;
#ifdef OE_GLSL_LEGACY
    } else if (attribMaskSet(vsAttrib, 8192.0, 2.0)) {
#else
    } else if ((vsAttrib & 0x2000) > 0) {
#endif
        texuvmod.x = 1.0;
    }

#ifdef OE_GLSL_LEGACY
    if (attribMaskSet(vsAttrib, 16384.0, 2.0) || attribMaskSet(vsAttrib, 2.0, 2.0)) {
#else
    if ((vsAttrib & 0x4000) > 0 || (vsAttrib & 0x2) > 0) {
#endif
        // Portals & fluids.
        // In-out movement.
        float pongperiod = mod(float(flowtimerms), 8000.0);
        float pongradians = pongperiod * radians(360.0) / 8000.0;
        float pongprogress = sin(pongradians);
        deltas.x = pongprogress * float(texsize.x) * 0.01 * sin(texuv.x / float(texsize.x) * radians(360.0));
        deltas.y = pongprogress * float(texsize.y) * 0.01 * sin(texuv.y / float(texsize.y) * radians(360.0));

        // Swirling movement.
        float flowperiod = mod(float(flowtimerms), 5000.0);
        float flowradians = flowperiod * radians(360.0) / 5000.0;
        deltas.x += float(texsize.x) * 0.01 * sin(flowradians + texuv.y / float(texsize.y) * radians(360.0));
        deltas.y += float(texsize.y) * 0.01 * cos(flowradians + texuv.x / float(texsize.x) * radians(360.0));

        // Small ripples.
        // TODO(captainurist): radians(360 * 32) looks better on portals, radians(360 * 16) looks better on lava,
        //                     but we don't have a way to differentiate, 0x4000 and 0x2 is set on both. Settling on
        //                     radians(360 * 24).
        float rippleperiod = mod(float(flowtimerms), 2000.0);
        float rippleradians = rippleperiod * radians(360.0) / 2000.0;
        deltas.x = deltas.x - float(texsize.x) * 0.005 * cos(rippleradians + (texuv.y + deltas.y) / float(texsize.y) * radians(360.0 * 24.0));
    } else {
        deltas.x = texuvmod.x * mod(float(flowtimer), float(texsize.x));
        deltas.y = texuvmod.y * mod(float(flowtimer), float(texsize.y));
    }

    texcoords.x = (deltas.x + texuv.x) / float(texsize.x);
    texcoords.y = (deltas.y + texuv.y) / float(texsize.y);
#ifdef OE_GLSL_LEGACY
    fragcol = texture2D(textureArray0, vec2(texcoords.x, (fract(texcoords.y) + olayer) * textureLayersScale));
#else
    fragcol = texture(textureArray0, vec3(texcoords.x,texcoords.y,olayer));
#endif

#ifdef OE_GLSL_LEGACY
    vec4 toplayer = texture2D(textureArray0, vec2(texcoords.x, (fract(texcoords.y) + float(0)) * textureLayersScale));
    vec4 watercol = texture2D(textureArray0, vec2(texcoords.x, (fract(texcoords.y) + float(waterframe)) * textureLayersScale));
#else
    vec4 toplayer = texture(textureArray0, vec3(texcoords.x,texcoords.y,0));
    vec4 watercol = texture(textureArray0, vec3(texcoords.x,texcoords.y,waterframe));
#endif

    if ((watertiles == 1) && (olayer == 0.0)){
#ifdef OE_GLSL_LEGACY
        if (attribMaskSet(vsAttrib, 1024.0, 16.0)){ // water anim disabled
#else
        if ((vsAttrib & 0x3C00) != 0){ // water anim disabled
#endif
            fragcol = toplayer;
        } else {
            fragcol = watercol;
        }
    }


    vec3 result = CalcSunLight(sun, fragnorm, fragviewdir, vec3(1)); //fragcol.rgb);
    result = clamp(result, 0.0, 0.85);

    result += CalcPointLight(fspointlights[0], fragnorm, vsPos, fragviewdir);

    // stack stationary
    for(int i = 1; i < num_point_lights; i++) {
        if (fspointlights[i].type == 1.0)
            result += CalcPointLight(fspointlights[i], fragnorm, vsPos, fragviewdir);
    }

    result *= fragcol.rgb;

    // stack mobile

    for(int i = 1; i < num_point_lights; i++) {
        if (fspointlights[i].type == 2.0)
            result += CalcPointLight(fspointlights[i], fragnorm, vsPos, fragviewdir);
    }

    vec3 clamps = result; // fragcol.rgb *  // clamp(result,0,1) * ;

        vec3 dull;

    // percpetion red fade
#ifdef OE_GLSL_LEGACY
    if (attribMaskSet(vsAttrib, 65536.0, 2.0)) {
#else
    if ((vsAttrib & 0x10000) > 0) {
#endif
        float ss = (sin(float(flowtimer) / 30.0) + 1.0) / 2.0;
        dull = vec3(1, ss, ss);
    } else {
        dull = vec3(1,1,1);
    }

    FragColour = vec4(clamps,1)  * vec4(dull,1); // result, 1.0);

    FragColour.rgb = pow(FragColour.rgb, vec3(1.0/gamma));

}
