//
// engine.h: This file contains the types and functions relative to the engine.
//

#pragma once

#include "platform.h"
#include "Geometry.h"
#include "Mesh.h"
#include "assimp_model_loading.h"
#include <glad/glad.h>

#define BINDING(b) b

typedef glm::vec2  vec2;
typedef glm::vec3  vec3;
typedef glm::vec4  vec4;
typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;

enum LightType
{
    LightType_Directional,
    LightType_Point
};

struct Light
{
    LightType type;
    vec3 color;
    vec3 direction;
    vec3 position;
};

struct Image
{
    void* pixels;
    ivec2 size;
    i32   nchannels;
    i32   stride;
};

struct Texture
{
    GLuint      handle;
    std::string filepath;
};

struct Program
{
    GLuint             handle;
    std::string        filepath;
    std::string        programName;
    u64                lastWriteTimestamp; // What is this for?
    VertexShaderLayout vertexInputLayout;
};

struct OpenGLInfo
{
    std::string version;
    std::string renderer;
    std::string vendor;
    std::string shadingLanguageVersion;

    std::vector<std::string> extensions;
};

struct Entity
{
    Entity(glm::mat4 world, u32 modelIdx, u32 localParamsOff, u32 localParamsSiz)
    {
        worldMatrix = world;
        modelIndex = modelIdx;
        localParamsOffset = localParamsOff;
        localParamsSize = localParamsSiz;
    }

    glm::mat4 worldMatrix;
    u32 modelIndex;
    u32 localParamsOffset;
    u32 localParamsSize;
};

enum Mode
{
    Mode_TexturedQuad,
    Mode_Model,
    Mode_Count
};

struct App
{
    // Loop
    f32  deltaTime;
    bool isRunning;

    // Input
    Input input;

    // Graphics
    char gpuName[64];
    char openGlVersion[64];
    GLint maxUniformBufferSize;
    GLint uniformBlockAlignment;
    GLuint uniformbufferHandle;
    ivec2 displaySize;

    std::vector<Texture>  textures;
    std::vector<Program>  programs;
    std::vector<Mesh> meshes;
    std::vector<Model> models;
    std::vector<Material> materials;
    std::vector<Entity> entities;


    // program indices
    u32 texturedGeometryProgramIdx;
    u32 texturedMeshProgramIdx;

    // texture indices
    u32 diceTexIdx;
    u32 whiteTexIdx;
    u32 blackTexIdx;
    u32 normalTexIdx;
    u32 magentaTexIdx;

    u32 model;
    u32 texturedMeshProgram_uTexture;

    // Mode
    Mode mode;

    // OpenGL info
    OpenGLInfo glInfo;
    bool showInfo = false;

    // Embedded geometry (in-editor simple meshes such as
    // a screen filling quad, a cube, a sphere...)
    GLuint embeddedVertices;
    GLuint embeddedElements;

    // Location of the texture uniform in the textured quad shader
    GLuint programUniformTexture;

    // VAO object to link our screen filling quad with our textured quad shader
    GLuint vao;
};

u32 LoadTexture2D(App* app, const char* filepath);

void Init(App* app);

void Gui(App* app);

void Update(App* app);

void Render(App* app);

