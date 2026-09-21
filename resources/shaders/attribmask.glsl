// The engine encodes per-vertex/face flags in a bitmask attribute, which the shaders decode with bitwise operators.
// GLSL ES 1.00 has no bitwise operators, so on that dialect (vitaGL) the tests are expressed with float math.
//
// `attrib` is an exact integer value well below 2^24, so floor/mod arithmetic is lossless. `lowBit` is the lowest bit
// of the tested mask and `valueCount` is 2 raised to the number of bits in it - e.g. for the single bit 0x400, pass
// 1024.0 and 2.0, and for the four-bit mask 0x3C00 pass 1024.0 and 16.0.

#ifdef OE_GLSL_LEGACY

bool attribMaskSet(float attrib, float lowBit, float valueCount) {
    return mod(floor(attrib / lowBit), valueCount) != 0.0;
}

#endif
