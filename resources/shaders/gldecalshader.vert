#ifdef OE_GLSL_LEGACY
attribute vec3 vaPos;
attribute vec2 vaTexUV;
attribute float vaTexLayer;
attribute vec3 vaColours;
attribute float vaAttrib;

varying vec4 vertexColour;
varying vec2 texuv;
varying vec4 viewspace;
#else
layout (location = 0) in vec3 vaPos;
layout (location = 1) in vec2 vaTexUV;
layout (location = 2) in float vaTexLayer;
layout (location = 3) in vec3 vaColours;
layout (location = 4) in float vaAttrib;

out vec4 vertexColour;
out vec2 texuv;
//flat out float olayer;
//out vec3 vsPos;
//out vec3 vsNorm;
//flat out int vsAttrib;
out vec4 viewspace;
#endif

uniform mat4 view;
uniform mat4 projection;

uniform float decalbias;

void main() {
	viewspace = view * vec4(vaPos, 1.0);
	gl_Position = projection * view * vec4(vaPos, 1.0);
	gl_Position -= vec4(0, 0, decalbias, 0);

	vertexColour = vec4(vaColours, 1.0);

	texuv = vaTexUV;
	//olayer = vaTexLayer;
	//vsPos = vaPos;
	//vsNorm = vaNormal;
	//vsAttrib = int(vaAttrib);
} 