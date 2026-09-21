#ifdef OE_GLSL_LEGACY
attribute vec3 vaPos;
attribute vec2 vaTexUV;
attribute float vaTexLayer;
attribute vec3 vaNormal;
attribute float vaAttrib;

varying vec4 vertexColour;
varying vec2 texuv;
varying float olayer;
varying vec3 vsPos;
varying vec3 vsNorm;
varying float vsAttrib;
#else
layout (location = 0) in vec3 vaPos;
layout (location = 1) in vec2 vaTexUV;
layout (location = 2) in float vaTexLayer;
layout (location = 3) in vec3 vaNormal;
layout (location = 4) in float vaAttrib;

out vec4 vertexColour;
out vec2 texuv;
flat out float olayer;
out vec3 vsPos;
out vec3 vsNorm;
flat out int vsAttrib;
#endif

uniform mat4 view;
uniform mat4 projection;


void main() {
    gl_Position = projection * view * vec4(vaPos, 1.0);

    //unused
    vertexColour = vec4(0.0005 * vaPos.y, 0.30, 0.30, 1.0);

    texuv = vaTexUV;
    olayer = vaTexLayer;
    vsPos = vaPos;
    vsNorm = vaNormal;
#ifdef OE_GLSL_LEGACY
    vsAttrib = vaAttrib;
#else
    vsAttrib = int(vaAttrib);
#endif
}
