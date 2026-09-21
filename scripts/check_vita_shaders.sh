#!/usr/bin/env bash
#
# Validates the vitaGL shader variant (OE_GLSL_LEGACY / OE_GLSL_NO_ARRAY_TEXTURES, GLSL ES 1.00) of every engine
# shader locally with glslangValidator.
#
# vitaGL's runtime translator only understands the legacy ES 1.00 dialect, so the legacy branches are what actually
# runs on the console. Validating them here avoids a hardware round-trip per syntax error.
#
# Note the "100" version: vitaGL strips the #version directive before translating, but glslang needs one to apply the
# ES 1.00 rules we care about.
#
# Usage: scripts/check_vita_shaders.sh

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SHADER_DIR="$REPO_ROOT/resources/shaders"

SHADERS=(
    glterrain gloutbuild glbspshader
    gltextshader gllinesshader gltwodshader
    glbillbshader gldecalshader glforcepershader
)

# Same defines the shader loader injects on vita, see OpenGLShader::loadShader. GL_ES is not defined here: an ES
# shader gets it from the compiler itself.
PREAMBLE='#version 100
#extension GL_GOOGLE_include_directive : enable
#define OE_GLSL_LEGACY 1
#define OE_GLSL_NO_ARRAY_TEXTURES 1'

TEMP_DIR="$(mktemp -d)"
trap 'rm -rf "$TEMP_DIR"' EXIT

failures=0
for name in "${SHADERS[@]}"; do
    for stage in vert frag; do
        source_file="$SHADER_DIR/$name.$stage"
        staged="$TEMP_DIR/$name.$stage"
        printf '%s\n' "$PREAMBLE" > "$staged"
        cat "$source_file" >> "$staged"

        if ! glslangValidator -I"$SHADER_DIR" -E "$staged" > "$TEMP_DIR/pp.$name.$stage" 2>"$TEMP_DIR/err.$name.$stage"; then
            echo "=== $name.$stage: preprocessing failed"
            cat "$TEMP_DIR/err.$name.$stage"
            failures=$((failures + 1))
            continue
        fi

        if ! output="$(glslangValidator "$TEMP_DIR/pp.$name.$stage" 2>&1)"; then
            echo "=== $name.$stage"
            echo "$output"
            failures=$((failures + 1))
        fi
    done
done

if [ "$failures" -eq 0 ]; then
    echo "All vita-class shaders compile as GLSL ES 1.00."
else
    echo "$failures shader(s) failed."
    exit 1
fi
