// Common precision declarations for GL ES.

#ifdef GL_ES
    precision highp int;
    precision highp float;
    precision highp sampler2D;
#ifndef OE_GLSL_NO_ARRAY_TEXTURES
    precision highp sampler2DArray;
#endif
#endif
