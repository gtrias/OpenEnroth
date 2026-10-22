#include "OpenGLShader.h"

#include <cassert>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <string>
#include <utility>

#include <glad/gl.h> // NOLINT: this is not a C system include.

#include "Engine/Resources/EngineFileSystem.h"

#include "Library/FileSystem/Interface/FileSystem.h"
#include "Library/Logger/Logger.h"
#include "Library/Preprocessor/Preprocessor.h"

// Note: the caller passes the object type explicitly - glIsShader is not available on all GL implementations (it's
// missing from Vita's vitaGL), and calling through a null function pointer takes the whole process down.
static std::string compileErrors(int object, bool isShader) {
    GLint success = 1;
    GLchar infoLog[2048];
    if (isShader) {
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (!success)
            glGetShaderInfoLog(object, 2048, NULL, infoLog);
    } else {
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (!success)
            glGetProgramInfoLog(object, 2048, NULL, infoLog);
    }

    if (!success) {
        if (infoLog[0]) {
            return infoLog;
        } else {
            return "Unknown error";
        }
    }

    return {};
}
#ifdef __vita__
static void bindAttributeLocations(GLuint program, const Blob &vertSource) {
    const std::string_view source(static_cast<const char *>(vertSource.data()), vertSource.size());

    // Scan the modern-dialect declarations ("layout (location = N) in <type> <name>;") and pin the same
    // locations for the legacy variant's attributes, which carry identical names.
    static constexpr std::string_view marker = "layout (location = ";
    size_t pos = 0;
    while ((pos = source.find(marker, pos)) != std::string_view::npos) {
        pos += marker.size();

        int location = 0;
        const char *begin = source.data() + pos;
        const auto [end, error] = std::from_chars(begin, source.data() + source.size(), location);
        const size_t semicolon = source.find(';', pos);
        if (error != std::errc() || end == begin || semicolon == std::string_view::npos || location < 0)
            continue;

        // The attribute name is the identifier right before the semicolon.
        size_t nameEnd = semicolon;
        while (nameEnd > pos && std::isspace(static_cast<unsigned char>(source[nameEnd - 1])))
            nameEnd--;
        size_t nameStart = nameEnd;
        while (nameStart > pos && (std::isalnum(static_cast<unsigned char>(source[nameStart - 1])) || source[nameStart - 1] == '_'))
            nameStart--;
        if (nameStart < nameEnd) {
            const std::string name(source.substr(nameStart, nameEnd - nameStart));
            glBindAttribLocation(program, location, name.c_str());
        }
    }
}
#endif

OpenGLShader::~OpenGLShader() {
    release();
}

#ifdef __vita__
/**
 * @param data                          Bytes to hash.
 * @param hash                          Hash to continue from.
 * @return                              64-bit FNV-1a hash, stable across runs unlike `std::hash`.
 */
static uint64_t fnv1a(std::string_view data, uint64_t hash = 14695981039346656037ull) {
    for (char c : data)
        hash = (hash ^ static_cast<unsigned char>(c)) * 1099511628211ull;
    return hash;
}

static std::string programCachePath(const Blob &vertSource, const Blob &fragSource) {
    uint64_t hash = fnv1a("vitaGL program binary v1");
    hash = fnv1a(vertSource.str(), hash);
    hash = fnv1a(fragSource.str(), hash);
    return fmt::format("shader_cache/{:016x}.bin", hash);
}
#endif

bool OpenGLShader::load(const Blob &vertSource, const Blob &fragSource, bool openGLES, const FileSystem *pwd) {
    Blob vertPreprocessed = preprocessShader(vertSource, openGLES, pwd);
    Blob fragPreprocessed = preprocessShader(fragSource, openGLES, pwd);
    if (!vertPreprocessed || !fragPreprocessed)
        return false;
    _paths = fmt::format("{}+{}", vertSource.displayPath(), fragSource.displayPath());

#ifdef __vita__
    // Compiling the shaders on the console takes most of startup, so linked programs are cached on the memory card.
    std::string cachePath = programCachePath(vertPreprocessed, fragPreprocessed);
    if (ufs->exists(cachePath)) {
        Blob binary = ufs->read(cachePath);
        GLuint program = glCreateProgram();
        glProgramBinary(program, 0, binary.data(), binary.size());
        _id = program;
        MM_INFO("Loaded shader program '{}' from '{}'.", _paths, cachePath);
        return true;
    }
#endif

    GLuint vertex = compileShader(vertPreprocessed, GL_VERTEX_SHADER);
    if (vertex == 0)
        return false;

    GLuint fragment = compileShader(fragPreprocessed, GL_FRAGMENT_SHADER);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return false;
    }

    int result = glCreateProgram();
    glAttachShader(result, vertex);
    glAttachShader(result, fragment);

#ifdef __vita__
    // vitaGL's GLSL ES 1.00 translator has no location convention - attribute locations are whatever its CG
    // semantic resolution produces, which does not match the fixed sequential locations the engine's vertex
    // arrays bind. The modern shader dialect pins the intended locations with layout(location = N), so mirror
    // those pins onto the legacy attribute names explicitly before linking.
    bindAttributeLocations(result, vertSource);
#endif

    glLinkProgram(result);

    // delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    std::string errors = compileErrors(result, /*isShader=*/false);
    if (!errors.empty()) {
        MM_ERROR("Could not link shader program '{}':\n{}", _paths, errors);
        glDeleteProgram(result);
        return false;
    }

#ifdef __vita__
    GLint binarySize = 0;
    glGetProgramiv(result, GL_PROGRAM_BINARY_LENGTH, &binarySize);
    if (binarySize > 0) {
        std::string binary(binarySize, '\0');
        GLsizei written = 0;
        GLenum format = 0;
        glGetProgramBinary(result, binarySize, &written, &format, binary.data());
        binary.resize(written);
        ufs->write(cachePath, Blob::fromString(std::move(binary)));
    }
#endif

    _id = result;
    return true;
}

void OpenGLShader::release() {
    if (_id == 0)
        return;

    glDeleteProgram(_id);
    _id = 0;
}

int OpenGLShader::uniformLocation(const char *name) const {
    assert(isValid());

    int location = glGetUniformLocation(_id, name);
    if (location == -1)
        MM_ERROR("Uniform '{}' not found in shader program '{}'", name, _paths);
    return location;
}

void OpenGLShader::use() {
    assert(isValid());

    glUseProgram(_id);
}

void OpenGLShader::unuse() {
    glUseProgram(0);
}

Blob OpenGLShader::preprocessShader(const Blob &source, bool openGLES, const FileSystem *pwd) {
    assert(pwd);

    // Preprocess source with version and GL_ES define in preamble.
#ifdef __vita__
    // vitaGL's GLSL translator only understands the legacy GLSL ES 1.00 dialect (attribute/varying/texture2D), and
    // it has no texture array support - so shaders compile their Vita-specific variant. Note that vitaGL strips the
    // #version directive anyway.
    (void) openGLES;
    std::string_view preamble = "#define GL_ES\n"
                                "#define OE_GLSL_LEGACY\n"
                                "#define OE_GLSL_NO_ARRAY_TEXTURES\n";
#else
    std::string_view preamble = openGLES ? "#version 320 es\n#define GL_ES\n" : "#version 410 core\n";
#endif
    try {
        static constexpr std::string_view glslDirectives[] = {"version", "extension"};
        return pp::preprocess(source, pwd, preamble, glslDirectives).withDisplayPath(source.displayPath());
    } catch (const std::exception &e) {
        MM_ERROR("Could not preprocess shader '{}': {}", source.displayPath(), e.what());
        return Blob();
    }
}

unsigned OpenGLShader::compileShader(const Blob &preprocessedSource, int type) {
    const char *sources[1] = {static_cast<const char *>(preprocessedSource.data())};
    const GLint lengths[1] = {static_cast<GLint>(preprocessedSource.size())};

    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, sources, lengths);
    glCompileShader(result);

    std::string errors = compileErrors(result, /*isShader=*/true);
    if (!errors.empty()) {
        MM_ERROR("Could not compile shader '{}':\n{}", preprocessedSource.displayPath(), errors);
        glDeleteShader(result);
        return 0;
    }

    MM_INFO("Loaded shader '{}'.", preprocessedSource.displayPath());
    return result;
}
