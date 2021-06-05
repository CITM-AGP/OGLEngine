//
// engine.cpp : Put all your graphics stuff in this file. This is kind of the graphics module.
// In here, you should type all your OpenGL commands, and you can also type code to handle
// input platform events (e.g to move the camera or react to certain shortcuts), writing some
// graphics related GUI options, and so on.
//

#define MIPMAP_BASE_LEVEL 0
#define MIPMAP_MAX_LEVEL 4

#include "engine.h"
#include <imgui.h>
#include <stb_image.h>
#include <stb_image_write.h>

GLuint CreateProgramFromSource(String programSource, const char* shaderName)
{
    GLchar  infoLogBuffer[1024] = {};
    GLsizei infoLogBufferSize = sizeof(infoLogBuffer);
    GLsizei infoLogSize;
    GLint   success;

    char versionString[] = "#version 430\n";
    char shaderNameDefine[128];
    sprintf(shaderNameDefine, "#define %s\n", shaderName);
    char vertexShaderDefine[] = "#define VERTEX\n";
    char fragmentShaderDefine[] = "#define FRAGMENT\n";

    const GLchar* vertexShaderSource[] = {
        versionString,
        shaderNameDefine,
        vertexShaderDefine,
        programSource.str
    };
    const GLint vertexShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(vertexShaderDefine),
        (GLint) programSource.len
    };
    const GLchar* fragmentShaderSource[] = {
        versionString,
        shaderNameDefine,
        fragmentShaderDefine,
        programSource.str
    };
    const GLint fragmentShaderLengths[] = {
        (GLint) strlen(versionString),
        (GLint) strlen(shaderNameDefine),
        (GLint) strlen(fragmentShaderDefine),
        (GLint) programSource.len
    };

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, ARRAY_COUNT(vertexShaderSource), vertexShaderSource, vertexShaderLengths);
    glCompileShader(vshader);
    glGetShaderiv(vshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with vertex shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, ARRAY_COUNT(fragmentShaderSource), fragmentShaderSource, fragmentShaderLengths);
    glCompileShader(fshader);
    glGetShaderiv(fshader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fshader, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glCompileShader() failed with fragment shader %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    GLuint programHandle = glCreateProgram();
    glAttachShader(programHandle, vshader);
    glAttachShader(programHandle, fshader);
    glLinkProgram(programHandle);
    glGetProgramiv(programHandle, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(programHandle, infoLogBufferSize, &infoLogSize, infoLogBuffer);
        ELOG("glLinkProgram() failed with program %s\nReported message:\n%s\n", shaderName, infoLogBuffer);
    }

    glUseProgram(0);

    glDetachShader(programHandle, vshader);
    glDetachShader(programHandle, fshader);
    glDeleteShader(vshader);
    glDeleteShader(fshader);

    return programHandle;
}

u32 LoadProgram(App* app, const char* filepath, const char* programName)
{
    String programSource = ReadTextFile(filepath);

    Program program = {};
    program.handle = CreateProgramFromSource(programSource, programName);
    program.filepath = filepath;
    program.programName = programName;
    program.lastWriteTimestamp = GetFileLastWriteTimestamp(filepath);

    int attributeCount;
    glGetProgramiv(program.handle, GL_ACTIVE_ATTRIBUTES, &attributeCount);

    GLchar attributeName[64];
    GLsizei attributeNameLength;
    GLint attributeSize;
    GLenum attributeType;

    for (int i = 0; i < attributeCount; ++i)
    {
        glGetActiveAttrib(program.handle,
            i,
            64/*ARRAY_COUNT(attributeName)*/,
            &attributeNameLength,
            &attributeSize,
            &attributeType,
            attributeName
        );

        program.vertexInputLayout.attributes.push_back(
            { (u8)glGetAttribLocation(program.handle, attributeName), (u8)attributeSize }); // position
    }

    app->programs.push_back(program);

    return app->programs.size() - 1;
}

Image LoadImage(const char* filename)
{
    Image img = {};
    stbi_set_flip_vertically_on_load(true);
    img.pixels = stbi_load(filename, &img.size.x, &img.size.y, &img.nchannels, 0);
    if (img.pixels)
    {
        img.stride = img.size.x * img.nchannels;
    }
    else
    {
        ELOG("Could not open file %s", filename);
    }
    return img;
}

void FreeImage(Image image)
{
    stbi_image_free(image.pixels);
}

GLuint CreateTexture2DFromImage(Image image)
{
    GLenum internalFormat = GL_RGB8;
    GLenum dataFormat     = GL_RGB;
    GLenum dataType       = GL_UNSIGNED_BYTE;

    switch (image.nchannels)
    {
        case 3: dataFormat = GL_RGB; internalFormat = GL_RGB8; break;
        case 4: dataFormat = GL_RGBA; internalFormat = GL_RGBA8; break;
        default: ELOG("LoadTexture2D() - Unsupported number of channels");
    }

    GLuint texHandle;
    glGenTextures(1, &texHandle);
    glBindTexture(GL_TEXTURE_2D, texHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, image.size.x, image.size.y, 0, dataFormat, dataType, image.pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    return texHandle;
}

u32 LoadTexture2D(App* app, const char* filepath)
{
    for (u32 texIdx = 0; texIdx < app->textures.size(); ++texIdx)
        if (app->textures[texIdx].filepath == filepath)
            return texIdx;

    Image image = LoadImage(filepath);

    if (image.pixels)
    {
        Texture tex = {};
        tex.handle = CreateTexture2DFromImage(image);
        tex.filepath = filepath;

        u32 texIdx = app->textures.size();
        app->textures.push_back(tex);

        FreeImage(image);
        return texIdx;
    }
    else
    {
        return UINT32_MAX;
    }
}

void OnGLError(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION)
        return;

    ELOG("OpenGL debug message: %s", message);

    switch (source)
    {
        case GL_DEBUG_SOURCE_API: ELOG(" - source: GL_DEBUG_SOURCE_API"); break;                            // Calls to OpenGL API
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM: ELOG(" - source: GL_DEBUG_SOURCE_WINDOW_SYSTEM"); break;        // Calls to a window-system API
        case GL_DEBUG_SOURCE_SHADER_COMPILER: ELOG(" - source: GL_DEBUG_SOURCE_SHADER_COMPILER"); break;    // A compiler for a shading language
        case GL_DEBUG_SOURCE_THIRD_PARTY: ELOG(" - source: GL_DEBUG_SOURCE_THIRD_PARTY"); break;            // An application associated to OpenGL
        case GL_DEBUG_SOURCE_APPLICATION: ELOG(" - source: GL_DEBUG_SOURCE_APPLICATION"); break;            // Generated by the user of this application
        case GL_DEBUG_SOURCE_OTHER: ELOG(" - source: GL_DEBUG_SOURCE_OTHER"); break;                        // Some source that is not any of these
    }


    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR: ELOG(" - type: GL_DEBUG_TYPE_ERROR"); break;                              // An error, typically from the API
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ELOG(" - type: GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"); break;  // Some behaviour marked deprecated
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: ELOG(" - type: GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"); break;    // Something has invoked undefined behaviour
        case GL_DEBUG_TYPE_PORTABILITY: ELOG(" - type: GL_DEBUG_TYPE_PORTABILITY"); break;                  // Some functionality the user relies upon
        case GL_DEBUG_TYPE_PERFORMANCE: ELOG(" - type: GL_DEBUG_TYPE_PERFORMANCE"); break;                  // Code has triggered possible performance problems
        case GL_DEBUG_TYPE_MARKER: ELOG(" - type: GL_DEBUG_TYPE_MARKER"); break;                            // Command stream annotation
        case GL_DEBUG_TYPE_PUSH_GROUP: ELOG(" - type: GL_DEBUG_TYPE_PUSH_GROUP"); break;                    // Group pushing
        case GL_DEBUG_TYPE_POP_GROUP: ELOG(" - type: GL_DEBUG_TYPE_POP_GROUP"); break;                      // Group pop
        case GL_DEBUG_TYPE_OTHER: ELOG(" - type: GL_DEBUG_TYPE_OTHER"); break;                              // Some type that is not any of these
    }

    switch (source)
    {
        case GL_DEBUG_SEVERITY_HIGH: ELOG(" - severity: GL_DEBUG_SEVERITY_HIGH"); break;                    // All OpenGL errors, shader cimpilation/linking
        case GL_DEBUG_SEVERITY_MEDIUM: ELOG(" - severity: GL_DEBUG_SEVERITY_MEDIUM"); break;                // major performance warnings, shader compilation
        case GL_DEBUG_SEVERITY_LOW: ELOG(" - severity: GL_DEBUG_SEVERITY_LOW"); break;                      //Redundant state change performance warning
    }
}

void GenerateFramebufferTexture(GLuint& textureHandle, ivec2 displaySize, GLint type)
{
    glGenTextures(1, &textureHandle);
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, displaySize.x, displaySize.y, 0, GL_RGBA, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void CheckFBOStatus()
{
    GLenum frameBufferStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (frameBufferStatus != GL_FRAMEBUFFER_COMPLETE)
    {
        switch (frameBufferStatus)
        {
        case GL_FRAMEBUFFER_UNDEFINED:                      ELOG("GL_FRAMEBUFFER_UNDEFINED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:          ELOG("GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:  ELOG("GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"); break;
        case GL_FRAMEBUFFER_UNSUPPORTED:                    ELOG("GL_FRAMEBUFFER_UNSUPPORTED"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:         ELOG("GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE"); break;
        case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:       ELOG("GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS"); break;
        default: ELOG("Unknown framebuffer status error");
        }
    }
}

void Init(App* app)
{
    // TODO: Initialize your resources here!
    // - vertex buffers
    // - element/index buffers
    // - vaos
    // - programs (and retrieve uniform indices)
    // - textures

    // --- Open GL info ---
    app->glInfo.version = (const char*)glGetString(GL_VERSION);
    app->glInfo.renderer = (const char*)glGetString(GL_RENDERER);
    app->glInfo.vendor = (const char*)glGetString(GL_VENDOR);
    app->glInfo.shadingLanguageVersion = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);

    // --- Framebuffer ---   
    GenerateFramebufferTexture(app->modelTextureAttachment, app->displaySize, GL_UNSIGNED_BYTE);
    GenerateFramebufferTexture(app->normalsTextureAttachment, app->displaySize, GL_UNSIGNED_BYTE);
    GenerateFramebufferTexture(app->albedoTextureAttachment, app->displaySize, GL_UNSIGNED_BYTE);
    GenerateFramebufferTexture(app->depthTextureAttachment, app->displaySize, GL_UNSIGNED_BYTE);
    GenerateFramebufferTexture(app->positionTextureAttachment, app->displaySize, GL_UNSIGNED_BYTE);

    // Depth
    glGenTextures(1, &app->depthAttachmentHandle);
    glBindTexture(GL_TEXTURE_2D, app->depthAttachmentHandle);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, app->displaySize.x, app->displaySize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Attach it to bound framebuffer object
    glGenFramebuffers(1, &app->framebufferHandle);
    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->modelTextureAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->normalsTextureAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, app->albedoTextureAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, app->depthTextureAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, app->positionTextureAttachment, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, app->depthAttachmentHandle, 0);

    CheckFBOStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // --- Uniform buffers ---
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &app->maxUniformBufferSize);
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &app->uniformBlockAlignment);
    app->cbuffer = CreateConstantBuffer(app->maxUniformBufferSize);

    GLint num_extensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);

    for (int i = 0; i < num_extensions; ++i)
    {
        app->glInfo.extensions.push_back((const char*)glGetStringi(GL_EXTENSIONS, GLuint(i)));
    }

    if (GL_MAJOR_VERSION > 4 || (GL_MAJOR_VERSION == 4 && GL_MINOR_VERSION >= 3))
    {
        glDebugMessageCallback(OnGLError, app);
    }

    // --- Geometry ---
    glGenBuffers(1, &app->embeddedVertices);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenBuffers(1, &app->embeddedElements);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Attribute state
    glGenVertexArrays(1, &app->vao);
    glBindVertexArray(app->vao);
    glBindBuffer(GL_ARRAY_BUFFER, app->embeddedVertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexV3V2), (void*)12);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, app->embeddedElements);
    glBindVertexArray(0);

    // --- Program ---
    app->texturedGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "TEXTURED_GEOMETRY");
    Program& texturedGeometryProgram = app->programs[app->texturedGeometryProgramIdx];
    app->programUniformTexture = glGetUniformLocation(texturedGeometryProgram.handle, "uTexture");

    app->deferredGeometryProgramIdx = LoadProgram(app, "shaders.glsl", "GEOMETRY_PASS");
    app->deferredLightingProgramIdx = LoadProgram(app, "shaders.glsl", "LIGHTING_PASS");
    app->blitBrightestPixelsProgram = LoadProgram(app, "shaders.glsl", "BLOOM_BRIGHTEST");
    app->blur = LoadProgram(app, "shaders.glsl", "BLOOM_BLUR");
    app->bloomProgram = LoadProgram(app, "shaders.glsl", "BLOOM");
    app->texturedMeshProgramIdx = LoadProgram(app, "shaders.glsl", "SHOW_TEXTURED_MESH");  

    // --- Textures ---
    app->diceTexIdx = LoadTexture2D(app, "dice.png");
    app->whiteTexIdx = LoadTexture2D(app, "color_white.png");
    app->blackTexIdx = LoadTexture2D(app, "color_black.png");
    app->normalTexIdx = LoadTexture2D(app, "color_normal.png");
    app->magentaTexIdx = LoadTexture2D(app, "color_magenta.png");
    app->barrelAlbedoMap = LoadTexture2D(app, "Bandit/Bandit1_AlbedoMap.png");
    app->barrelNormalMap = LoadTexture2D(app, "models/Barrel_NormalMap.png");

    app->model = LoadModel(app, "Patrick/Patrick.obj");
    app->barrel = LoadModel(app, "models/Barrel_Prop.fbx");
    app->sphere = LoadModel(app, "models/Sphere.fbx");
    app->plane = LoadModel(app, "models/Plane.fbx");


    // --- Create entities ---
    Entity ent = Entity(glm::mat4(1.0), app->model, 0, 0);
    ent.worldMatrix = glm::translate(ent.worldMatrix, vec3(-5.0, 1.0, 5.0));
    app->entities.push_back(ent);

    Entity ent2 = Entity(glm::mat4(1.0), app->model, 0, 0);
    ent2.worldMatrix = glm::translate(ent2.worldMatrix, vec3(2.5f, 1.0, 2.0));
    app->entities.push_back(ent2);

    Entity ent3 = Entity(glm::mat4(1.0), app->model, 0, 0);
    ent3.worldMatrix = glm::translate(ent3.worldMatrix, vec3(2.0, 2.0, -2.0));
    app->entities.push_back(ent3);

    Entity ent4 = Entity(glm::mat4(1.0), app->plane, 0, 0);
    ent4.worldMatrix = glm::translate(ent4.worldMatrix, vec3(0.0, -2.5, 0.0));
    app->entities.push_back(ent4);

    Entity ent5 = Entity(glm::mat4(1.0), app->barrel, 0, 0);
    ent5.worldMatrix = glm::translate(ent5.worldMatrix, vec3(0.0, 5.0, 0.0));
    app->entities.push_back(ent5);

    vec3 lightPos1 = vec3(-1.0, 1.0, -5.0);
    vec3 lightPos2 = vec3(6.0, 1.0, 0.0);
    vec3 lightPos3 = vec3(0.0, 1.0, 7.0);

    Entity entlight1 = Entity(glm::mat4(1.0), app->sphere, 0, 0);
    entlight1.worldMatrix = glm::translate(entlight1.worldMatrix, lightPos1);
    app->entities.push_back(entlight1);

    Entity entlight2 = Entity(glm::mat4(1.0), app->sphere, 0, 0);
    entlight2.worldMatrix = glm::translate(entlight2.worldMatrix, lightPos2);
    app->entities.push_back(entlight2);

    Entity entlight3 = Entity(glm::mat4(1.0), app->sphere, 0, 0);
    entlight3.worldMatrix = glm::translate(entlight3.worldMatrix, lightPos3);
    app->entities.push_back(entlight3);

    // --- Create lights
    Light light0 = Light(LightType::LightType_Directional, vec3(1.0, 1.0, 1.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 0.0)); 
    app->lights.push_back(light0);

    Light light02 = Light(LightType::LightType_Directional, vec3(1.0, 1.0, 1.0), vec3(-1.0, 0.0, 1.0), vec3(0.0, 0.0, 0.0));
    app->lights.push_back(light02);

    Light light1 = Light(LightType::LightType_Point, vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, 0.0), lightPos1);
    app->lights.push_back(light1);

    Light light2 = Light(LightType::LightType_Point, vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, 0.0), lightPos2);
    app->lights.push_back(light2);

    Light light3 = Light(LightType::LightType_Point, vec3(1.0, 1.0, 1.0), vec3(0.0, 0.0, 0.0), lightPos3);
    app->lights.push_back(light3);

    // --- Camera ---
    app->cameraReference = vec3(0.0f);
    app->cameraPosition = vec3(0.0f, 4.0f, 15.0f);

    app->cameraMatrix = glm::lookAt
    (
        app->cameraPosition,        // the position of your camera, in world space
        app->cameraReference,       // where you want to look at, in world space
        glm::vec3(0, 1, 0)          // probably glm::vec3(0,1,0), but (0,-1,0) would make you looking upside-down, which can be great too
    );

    // Generates a really hard-to-read matrix, but a normal, standard 4x4 matrix nonetheless
    app->projectionMatrix = glm::perspective(
        glm::radians(60.0f),        // The vertical Field of View, in radians: the amount of "zoom". Think "camera lens". Usually between 90° (extra wide) and 30° (quite zoomed in)
        4.0f / 3.0f,                // Aspect Ratio. Depends on the size of your window. Notice that 4/3 == 800/600 == 1280/960, sounds familiar ?
        0.1f,                       // Near clipping plane. Keep as big as possible, or you'll get precision issues.
        1000.0f                      // Far clipping plane. Keep as little as possible.
    );

    // --- BLOOM ---

    // Bloom mipmap

    if (app->rtBright != 0) 
        glDeleteTextures(1, &app->rtBright);

    glGenTextures(1, &app->rtBright);
    glBindTexture(GL_TEXTURE_2D, app->rtBright);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);

    int w = app->displaySize.x;
    int h = app->displaySize.y;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w / 2, h / 2, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, w / 4, h / 4, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, w / 8, h / 8, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, w / 16, h / 16, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, w / 32, h / 32, 0, GL_RGBA, GL_FLOAT, nullptr);
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glBindTexture(GL_TEXTURE_2D, 0);

    // Bloom mipmap
    if (app->rtBloomH != 0)
        glDeleteTextures(1, &app->rtBloomH);

    glGenTextures(1, &app->rtBloomH);
    glBindTexture(GL_TEXTURE_2D, app->rtBloomH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, MIPMAP_BASE_LEVEL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MIPMAP_MAX_LEVEL);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w / 2, h / 2, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA16F, w / 4, h / 4, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA16F, w / 8, h / 8, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 3, GL_RGBA16F, w / 16, h / 16, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexImage2D(GL_TEXTURE_2D, 4, GL_RGBA16F, w / 32, h / 32, 0, GL_RGBA, GL_FLOAT, nullptr);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);

    // Bloom fbos 
    glGenFramebuffers(1, &app->fboBloom1);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom1);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->rtBright, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->rtBloomH, 0);
    CheckFBOStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->fboBloom2);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom2);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->rtBright, 1);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->rtBloomH, 1);
    CheckFBOStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->fboBloom3);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom3);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->rtBright, 2);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->rtBloomH, 2);
    CheckFBOStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->fboBloom4);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom4);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->rtBright, 3);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->rtBloomH, 3);
    CheckFBOStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glGenFramebuffers(1, &app->fboBloom5);
    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom5);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, app->rtBright, 4);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, app->rtBloomH, 4);
    CheckFBOStatus();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // --- Mode ---
    app->mode = Mode::Mode_Model;
}

void Gui(App* app)
{
    ImGui::BeginMainMenuBar();     
    if (ImGui::BeginMenu("Render mode"))
    {
        if (ImGui::MenuItem("ForwardRender"))
            app->mode = Mode::Mode_ForwardRender;

        if (ImGui::MenuItem("Model"))
            app->mode = Mode::Mode_Model;

        if (ImGui::MenuItem("Normals"))
            app->mode = Mode::Mode_Normals;

        if (ImGui::MenuItem("Albedo"))
            app->mode = Mode::Mode_Albedo;

        if (ImGui::MenuItem("Depth"))
            app->mode = Mode::Mode_Depth;

        if (ImGui::MenuItem("Position"))
            app->mode = Mode::Mode_Position;

        //if (ImGui::MenuItem("BloomBrightest"))
        //    app->mode = Mode::Mode_Bloom_Brightest;

        //if (ImGui::MenuItem("BloomBlur"))
        //    app->mode = Mode::Mode_Bloom_Blur;

        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Debug"))
    {
        if (ImGui::MenuItem("Debug window"))
            app->showInfo = !app->showInfo;

        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();

    if (app->showInfo)
    {
        ImGui::Begin("Debug window", &app->showInfo);
        static vec3 entity1 = vec3(-5.0f, 1.0f, 5.0f);
        static vec3 entity2 = vec3(2.5f, 1.0f, 2.0f);
        static vec3 entity3 = vec3(2.0f, 2.0f, -2.0f);

        ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "NORMAL MAP");
        ImGui::NewLine();

        ImGui::Checkbox("Normal Map", &app->normalMap);
        ImGui::NewLine();

        ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "BLOOM");
        ImGui::NewLine();

        ImGui::Checkbox("Render Bloom", &app->renderBloom);
        ImGui::NewLine();

        ImGui::InputInt("Kernel Radius", &app->kernelRadius);
        ImGui::NewLine();
        ImGui::InputFloat("Bloom Threshold", &app->bloomThreshold);
        ImGui::NewLine();
        ImGui::InputInt("LOD 0 Intensity", &app->lodIntensity0);
        ImGui::NewLine();
        ImGui::InputInt("LOD 1 Intensity", &app->lodIntensity1);
        ImGui::NewLine();
        ImGui::InputInt("LOD 2 Intensity", &app->lodIntensity2);
        ImGui::NewLine();
        ImGui::InputInt("LOD 3 Intensity", &app->lodIntensity3);
        ImGui::NewLine();
        ImGui::InputInt("LOD 4 Intensity", &app->lodIntensity4);        
        ImGui::NewLine();


        ImGui::NewLine();

        ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "Entity positions [X / Y / Z]");
        ImGui::NewLine();

        ImGui::Text("Entity 1");
        if (ImGui::DragFloat3("##e", (float*)&entity1, 0.1f))
        {
            app->entities[0] = Entity(glm::mat4(1.0), app->model, 0, 0);
            app->entities[0].worldMatrix = glm::translate(app->entities[0].worldMatrix, vec3(entity1.x, entity1.y, entity1.z));
        }

        ImGui::Text("Entity 2");
        if (ImGui::DragFloat3("##e2", (float*)&entity2, 0.1f))
        {
            app->entities[1] = Entity(glm::mat4(1.0), app->model, 0, 0);
            app->entities[1].worldMatrix = glm::translate(app->entities[1].worldMatrix, vec3(entity2.x, entity2.y, entity2.z));
        }

        ImGui::Text("Entity 3");
        if (ImGui::DragFloat3("##e3", (float*)&entity3, 0.1f))
        {
            app->entities[2] = Entity(glm::mat4(1.0), app->model, 0, 0);
            app->entities[2].worldMatrix = glm::translate(app->entities[2].worldMatrix, vec3(entity3.x, entity3.y, entity3.z));
        }

        ImGui::NewLine();
        ImGui::Separator();
        ImGui::NewLine();

        ImGui::TextColored(ImVec4(1.0, 1.0, 0.0, 1.0), "Info");
        ImGui::NewLine();
        ImGui::Text("FPS: %f", 1.0f / app->deltaTime);

        // --- Open GL info ---
        ImGui::Text("OpenGL version: %s", app->glInfo.version.c_str());
        ImGui::Text("OpenGL renderer: %s", app->glInfo.renderer.c_str());
        ImGui::Text("GPU vendor: %s", app->glInfo.vendor.c_str());
        ImGui::Text("GLSL version: %s", app->glInfo.shadingLanguageVersion.c_str());

        ImGui::NewLine();
        if (ImGui::CollapsingHeader("Extensions"))
        {
            for (int i = 0; i < app->glInfo.extensions.size(); ++i)
                ImGui::Text("Extension %i: %s", i, app->glInfo.extensions[i].c_str());
        }

        ImGui::End();
    }
}

void HandleUserInput(App* app)
{
    vec3 newPos(0.0f);
    float speed = 10.0f * app->deltaTime; 

    vec3 right_vector(1, 0, 0);
    vec3 up_vector(0, 1, 0);
    vec3 rotation_center(0, 0, -3.5);
    
    if (app->input.keys[K_LEFT_SHIFT] == BUTTON_PRESSED) speed = 20.0f * app->deltaTime;

    if (app->input.mouseButtons[LEFT])
    {
        // Rotation based on mouse axis
        glm::mat4x4 rotation_matrixX = glm::rotate(-app->input.mouseDelta.x / 2 * app->deltaTime, glm::normalize(up_vector));
        glm::mat4x4 rotation_matrixY = glm::rotate(-app->input.mouseDelta.y / 2 * app->deltaTime, glm::normalize(right_vector));
        glm::mat4x4 transform = glm::translate(rotation_center) * rotation_matrixX * rotation_matrixY * glm::translate(-rotation_center);

        // Apply the transformations
        app->cameraPosition = glm::vec3(transform * glm::vec4(app->cameraPosition, 1));
        app->cameraReference = glm::vec3(transform * glm::vec4(app->cameraReference, 1));
    }

    if (app->input.mouseButtons[RIGHT])
    {
        vec3 direction_vector = glm::normalize(app->cameraReference - app->cameraPosition);

        if (app->input.keys[K_W] == BUTTON_PRESSED) newPos += speed * direction_vector;
        if (app->input.keys[K_S] == BUTTON_PRESSED) newPos -= speed * direction_vector;

        if (app->input.keys[K_D] == BUTTON_PRESSED) newPos += speed * vec3(-direction_vector.z, 0, direction_vector.x);
        if (app->input.keys[K_A] == BUTTON_PRESSED) newPos -= speed * vec3(-direction_vector.z, 0, direction_vector.x);

        if (app->input.keys[K_T] == BUTTON_PRESSED) newPos.y += speed;
        if (app->input.keys[K_G] == BUTTON_PRESSED) newPos.y -= speed;
    
        app->cameraReference += newPos;
        app->cameraPosition += newPos;
    }

    // Generate view matrix
    app->cameraMatrix = glm::lookAt(app->cameraPosition, app->cameraReference, up_vector);
}

void Update(App* app)
{
    for (u64 i = 0; i < app->programs.size(); ++i)
    {
        Program& program = app->programs[i];
        u64 currentTimestamp = GetFileLastWriteTimestamp(program.filepath.c_str());

        if (currentTimestamp > program.lastWriteTimestamp)
        {
            glDeleteProgram(program.handle);
            String programSource = ReadTextFile(program.filepath.c_str());
            const char* programName = program.programName.c_str();
            program.handle = CreateProgramFromSource(programSource, programName);
            program.lastWriteTimestamp = currentTimestamp;
        }
    }

    HandleUserInput(app);
    
    // --- Global params ---
    MapBuffer(app->cbuffer, GL_WRITE_ONLY);
    app->globalParamsOffset = app->cbuffer.head;

    PushVec3(app->cbuffer, app->cameraPosition);
    PushUInt(app->cbuffer, app->lights.size());

    for (u32 i = 0; i < app->lights.size(); ++i)
    {
        AlignHead(app->cbuffer, sizeof(vec4));

        Light& light = app->lights[i];
        PushUInt(app->cbuffer, light.type);
        PushVec3(app->cbuffer, light.color);
        PushVec3(app->cbuffer, light.direction);
        PushVec3(app->cbuffer, light.position);
    }

    app->globalParamsSize = app->cbuffer.head - app->globalParamsOffset;

    // --- Local params ---
    for (Entity& ent : app->entities)
    {
        glm::mat4 worldViewProjectionMatrix = app->projectionMatrix * app->cameraMatrix * ent.worldMatrix;

        AlignHead(app->cbuffer, app->uniformBlockAlignment);
        ent.localParamsOffset = app->cbuffer.head;

        PushMat4(app->cbuffer, ent.worldMatrix);
        PushMat4(app->cbuffer, worldViewProjectionMatrix);

        ent.localParamsSize = app->cbuffer.head - ent.localParamsOffset;
    }

    UnmapBuffer(app->cbuffer);
}

GLuint FindVAO(Mesh& mesh, u32 submeshIndex, const Program& program)
{
    Submesh& submesh = mesh.submeshes[submeshIndex];

    // Try finding a vao for this submesh/program
    for (u32 i = 0; i < (u32)submesh.vaos.size(); ++i)
    {
        if (submesh.vaos[i].programHandle == program.handle)
            return submesh.vaos[i].handle;
    }

    GLuint vaoHandle = 0;

    // Create a new vao for this submesh/program
    glGenVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vertexBufferHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.indexBufferHandle);

    // We have to link all vertex inputs attributes to attributes in the vertex buffer

    for (u32 i = 0; i < program.vertexInputLayout.attributes.size(); ++i)
    {
        bool attributeWasLinked = false;

        for (u32 j = 0; j < submesh.vertexBufferLayout.attributes.size(); ++j)
        {
            if (program.vertexInputLayout.attributes[i].location == submesh.vertexBufferLayout.attributes[j].location)
            {
                const u32 index = submesh.vertexBufferLayout.attributes[j].location;
                const u32 ncomp = submesh.vertexBufferLayout.attributes[j].componentCount;
                const u32 offset = submesh.vertexBufferLayout.attributes[j].offset + submesh.vertexOffset; // attribute offset + vertex offset
                const u32 stride = submesh.vertexBufferLayout.stride;
                glVertexAttribPointer(index, ncomp, GL_FLOAT, GL_FALSE, stride, (void*)(u64)offset);
                glEnableVertexAttribArray(index);

                attributeWasLinked = true;
                break;
            }
        }

        assert(attributeWasLinked); // The submesh should provide an attribute for each vertex inputs
    }

    glBindVertexArray(0);

    // Store it in the list of vaos for this submesh
    Vao vao = { vaoHandle, program.handle };
    submesh.vaos.push_back(vao);

    return vaoHandle;
}

// renderQuad() renders a 1x1 XY quad in NDC
// -----------------------------------------
unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad(App* app)
{
    if (quadVAO == 0)
    {
        float quadVertices[] = 
        {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void passBloom(App* app, GLuint fbo, GLenum colorAttachment, GLuint inputTexture, int maxLod)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(colorAttachment);
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    Program& program = app->programs[app->bloomProgram];
    glUseProgram(program.handle);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(program.handle, "colorMap"), 0);
    glUniform1i(glGetUniformLocation(program.handle, "maxLod"), maxLod);
    glUniform1i(glGetUniformLocation(program.handle, "lodI0"), app->lodIntensity0);
    glUniform1i(glGetUniformLocation(program.handle, "lodI1"), app->lodIntensity1);
    glUniform1i(glGetUniformLocation(program.handle, "lodI2"), app->lodIntensity2);
    glUniform1i(glGetUniformLocation(program.handle, "lodI3"), app->lodIntensity3);
    glUniform1i(glGetUniformLocation(program.handle, "lodI4"), app->lodIntensity4);

    renderQuad(app);

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glBlendFunc(GL_ONE, GL_ZERO);
    glEnable(GL_DEPTH_TEST);
}

void passBlur(App* app, GLuint pfbo, const glm::uvec2& viewportSize, GLenum colorAttachment, GLuint inputTexture, GLint inputLod, const glm::uvec2& direction)
{
    glBindFramebuffer(GL_FRAMEBUFFER, pfbo);
    glDrawBuffer(colorAttachment);
    glViewport(0, 0, viewportSize.x, viewportSize.y);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    Program& program = app->programs[app->blur];
    glUseProgram(program.handle);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glUniform1i(glGetUniformLocation(program.handle, "colorMap"), 0);
    glUniform1i(glGetUniformLocation(program.handle, "inputLod"), inputLod);
    glUniform1i(glGetUniformLocation(program.handle, "kernelRadius"), app->kernelRadius);
    glUniform2i(glGetUniformLocation(program.handle, "direction"), direction.x, direction.y);

    renderQuad(app);

    glUseProgram(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_BLEND);
}

void passBlitBrightPixels(App* app, GLuint fbo, const glm::uvec2& viewportSize, GLenum colorAttachment, GLuint inputTexture)
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glDrawBuffer(colorAttachment);
    glViewport(0, 0, viewportSize.x, viewportSize.y);

    Program& program = app->programs[app->blitBrightestPixelsProgram];
    glUseProgram(program.handle);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glUniform1i(glGetUniformLocation(program.handle, "colorTexture"), 0);
    glUniform1f(glGetUniformLocation(program.handle, "threshold"), app->bloomThreshold);

    renderQuad(app);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glUseProgram(0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ResetFBOS(App* app)
{
    GLuint drawBuffers[] =
    {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
    };

    glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom1);
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom2);
    //glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);
    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom3);
    //glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);
    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom4);
    //glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);
    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glBindFramebuffer(GL_FRAMEBUFFER, app->fboBloom5);
    //glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);
    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void RenderBloom(App* app)
{
#define LOD(x) x

    const glm::uvec2 horizontal(1.0, 0.0);
    const glm::uvec2 vertical(0.0, 1.0);

    const float w = app->displaySize.x;
    const float h = app->displaySize.y;

    //ResetFBOS(app);

    // horizontal blur
    float threshold = 1.0;
    passBlitBrightPixels(app, app->fboBloom1, glm::uvec2(w / 2, h / 2), GL_COLOR_ATTACHMENT0, app->modelTextureAttachment);
    glBindTexture(GL_TEXTURE_2D, app->rtBright);
    glGenerateMipmap(GL_TEXTURE_2D);

    //// horizontal blur
    passBlur(app, app->fboBloom1, glm::uvec2(w / 2, h / 2), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(0), horizontal);
    passBlur(app, app->fboBloom2, glm::uvec2(w / 4, h / 4), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(1), horizontal);
    passBlur(app, app->fboBloom3, glm::uvec2(w / 8, h / 8), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(2), horizontal);
    passBlur(app, app->fboBloom4, glm::uvec2(w / 16, h / 16), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(3), horizontal);
    passBlur(app, app->fboBloom5, glm::uvec2(w / 32, h / 32), GL_COLOR_ATTACHMENT1, app->rtBright, LOD(4), horizontal);

    // vertical blur
    passBlur(app, app->fboBloom1, glm::uvec2(w / 2, h / 2), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(0), vertical);
    passBlur(app, app->fboBloom2, glm::uvec2(w / 4, h / 4), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(1), vertical);
    passBlur(app, app->fboBloom3, glm::uvec2(w / 8, h / 8), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(2), vertical);
    passBlur(app, app->fboBloom4, glm::uvec2(w / 16, h / 16), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(3), vertical);
    passBlur(app, app->fboBloom5, glm::uvec2(w / 32, h / 32), GL_COLOR_ATTACHMENT0, app->rtBloomH, LOD(4), vertical);

    passBloom(app, app->framebufferHandle, GL_COLOR_ATTACHMENT0, app->rtBloomH, 5);

    glViewport(0, 0, app->displaySize.x, app->displaySize.y);
    glBindTexture(GL_TEXTURE_2D, 0);

#undef LOD
}

void Render(App* app)
{
    glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "Render");

    // --- Framebuffer ---
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ResetFBOS(app);

    glBindFramebuffer(GL_FRAMEBUFFER, app->framebufferHandle);

    GLuint drawBuffers[] =
    {
        GL_COLOR_ATTACHMENT0,
        GL_COLOR_ATTACHMENT1,
        GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3,
        GL_COLOR_ATTACHMENT4
    };
    glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glViewport(0, 0, app->displaySize.x, app->displaySize.y);

    // --- Draw entities --

   /* Program& texturedMeshProgram = app->programs[app->texturedMeshProgramIdx];
    glUseProgram(texturedMeshProgram.handle);*/

    // Deferred geometry pass

    Program& renderProgram = app->programs[app->deferredGeometryProgramIdx];

    // forward shading
    if (app->mode == Mode::Mode_ForwardRender)
        renderProgram = app->programs[app->texturedMeshProgramIdx];

    glUseProgram(renderProgram.handle);

    glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);

    for (Entity& ent : app->entities)
    {
        Model& model = app->models[ent.modelIndex];
        Mesh& mesh = app->meshes[model.meshIdx];

        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(1), app->cbuffer.handle, ent.localParamsOffset, ent.localParamsSize);
        glEnable(GL_DEPTH_TEST);

        for (u32 i = 0; i < mesh.submeshes.size(); ++i)
        {
            GLuint vao = FindVAO(mesh, i, renderProgram);
            glBindVertexArray(vao);

            u32 submeshMaterialIdx = model.materialIdx[i];
            Material& submeshMaterial = app->materials[submeshMaterialIdx];

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, app->textures[submeshMaterial.albedoTextureIdx].handle);

            //if (ent.modelIndex > 1)
           //     glUniform1i(glGetUniformLocation(renderProgram.handle, "noTexture"), 1);
            //else
                glUniform1i(glGetUniformLocation(renderProgram.handle, "noTexture"), 0);
            
            glUniform1i(glGetUniformLocation(renderProgram.handle, "uTexture"), 0);

            if (app->normalMap)
            {
                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, app->textures[app->barrelNormalMap].handle);
                glUniform1i(glGetUniformLocation(renderProgram.handle, "uNormalMap"), 1);

                if (ent.modelIndex == 1)
                    glUniform1i(glGetUniformLocation(renderProgram.handle, "noNormal"), 0);
                else
                    glUniform1i(glGetUniformLocation(renderProgram.handle, "noNormal"), 1);
            }

            Submesh& submesh = mesh.submeshes[i];
            glDrawElements(GL_TRIANGLES, submesh.indices.size(), GL_UNSIGNED_INT, (void*)(u64)submesh.indexOffset);
        }
    }

    // deferred lighting pass
    if (app->mode != Mode::Mode_ForwardRender)
    {
        Program& deferredLighting = app->programs[app->deferredLightingProgramIdx];
        glUseProgram(deferredLighting.handle);

        glUniform1i(glGetUniformLocation(deferredLighting.handle, "oNormals"), 0);
        glUniform1i(glGetUniformLocation(deferredLighting.handle, "oAlbedo"), 1);
        glUniform1i(glGetUniformLocation(deferredLighting.handle, "oDepth"), 2);
        glUniform1i(glGetUniformLocation(deferredLighting.handle, "oPosition"), 3);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, app->normalsTextureAttachment);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, app->albedoTextureAttachment);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, app->depthTextureAttachment);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, app->positionTextureAttachment);

        GLuint drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(ARRAY_COUNT(drawBuffers), drawBuffers);

        glDepthMask(false);
        glBindBufferRange(GL_UNIFORM_BUFFER, BINDING(0), app->cbuffer.handle, app->globalParamsOffset, app->globalParamsSize);
        renderQuad(app);
        glDepthMask(true);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0); 

    if (app->renderBloom && app->mode != Mode::Mode_ForwardRender)
    {
        RenderBloom(app);
    }

    // --- Draw framebuffer texture ---
    Program& programTexturedGeometry = app->programs[app->texturedGeometryProgramIdx];
    glUseProgram(programTexturedGeometry.handle);
    //glBindVertexArray(app->vao);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUniform1i(app->programUniformTexture, 0);
    glActiveTexture(GL_TEXTURE0);

    switch (app->mode)
    {
        case Mode::Mode_ForwardRender:
            glBindTexture(GL_TEXTURE_2D, app->modelTextureAttachment);
            break;

        case Mode::Mode_Model:
            glBindTexture(GL_TEXTURE_2D, app->modelTextureAttachment);
            break;

        case Mode::Mode_Normals:
            glBindTexture(GL_TEXTURE_2D, app->normalsTextureAttachment);
            break;

        case Mode::Mode_Albedo:
            glBindTexture(GL_TEXTURE_2D, app->albedoTextureAttachment);
            break;

        case Mode::Mode_Depth:
            glBindTexture(GL_TEXTURE_2D, app->depthTextureAttachment);
            break;

        case Mode::Mode_Position:
            glBindTexture(GL_TEXTURE_2D, app->positionTextureAttachment);
            break;

        //case Mode::Mode_Bloom_Brightest:
        //    glBindTexture(GL_TEXTURE_2D, app->rtBright);
        //    break;

        //case Mode::Mode_Bloom_Blur:
        //    glBindTexture(GL_TEXTURE_2D, app->rtBloomH);
        //    break;
    }
    
    renderQuad(app);

    //glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, 0);
    //glBindVertexArray(0);
    glUseProgram(0);
    glPopDebugGroup();
}