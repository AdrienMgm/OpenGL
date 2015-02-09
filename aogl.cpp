#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>

#include "glew/glew.h"

#include "GLFW/glfw3.h"
#include "stb/stb_image.h"
#include "imgui/imgui.h"
#include "imgui/imguiRenderGL3.h"

#include "glm/glm.hpp"
#include "glm/vec3.hpp" // glm::vec3
#include "glm/vec4.hpp" // glm::vec4, glm::ivec4
#include "glm/mat4x4.hpp" // glm::mat4
#include "glm/gtc/matrix_transform.hpp" // glm::translate, glm::rotate, glm::scale, glm::perspective
#include "glm/gtc/type_ptr.hpp" // glm::value_ptr

#include <vector>

#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1
#endif

#if DEBUG_PRINT == 0
#define debug_print(FORMAT, ...) ((void)0)
#else
#ifdef _MSC_VER
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __FUNCTION__, __FILE__, __LINE__, __VA_ARGS__)
#else
#define debug_print(FORMAT, ...) \
    fprintf(stderr, "%s() in %s, line %i: " FORMAT "\n", \
        __func__, __FILE__, __LINE__, __VA_ARGS__)
#endif
#endif

// Font buffers
extern const unsigned char DroidSans_ttf[];
extern const unsigned int DroidSans_ttf_len;    

// Shader utils
int check_link_error(GLuint program);
int check_compile_error(GLuint shader, const char ** sourceBuffer);
GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize);
GLuint compile_shader_from_file(GLenum shaderType, const char * fileName);

// OpenGL utils
bool checkError(const char* title);

struct Camera
{
    float radius;
    float theta;
    float phi;
    glm::vec3 o;
    glm::vec3 eye;
    glm::vec3 up;
};
void camera_defaults(Camera & c);
void camera_zoom(Camera & c, float factor);
void camera_turn(Camera & c, float phi, float theta);
void camera_pan(Camera & c, float x, float y);

struct PointLight
{
    glm::vec3 position;
    int padding;
    glm::vec3 color;
    float intensity;
};

struct DirectionalLight
{
    glm::vec3 position;
    int padding;
    glm::vec3 color;
    float intensity;
};

struct SpotLight
{
    glm::vec3 position;
    float angle;
    glm::vec3 direction;
    float penumbraAngle;
    glm::vec3 color;
    float intensity;
};

struct GUIStates
{
    bool panLock;
    bool turnLock;
    bool zoomLock;
    int lockPositionX;
    int lockPositionY;
    int camera;
    double time;
    bool playing;
    static const float MOUSE_PAN_SPEED;
    static const float MOUSE_ZOOM_SPEED;
    static const float MOUSE_TURN_SPEED;
};
const float GUIStates::MOUSE_PAN_SPEED = 0.001f;
const float GUIStates::MOUSE_ZOOM_SPEED = 0.05f;
const float GUIStates::MOUSE_TURN_SPEED = 0.005f;

void init_gui_states(GUIStates & guiStates);

int main( int argc, char **argv )
{
    assert(sizeof(glm::vec3) == sizeof(GLfloat) * 3 && "glm::vec3 != 3 * GLfloat");

    int width = 1024, height= 768;
    float widthf = (float) width, heightf = (float) height;
    double t;
    float fps = 0.f;

    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        exit( EXIT_FAILURE );
    }
    glfwInit();
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
    glfwWindowHint(GLFW_DECORATED, GL_TRUE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    int const DPI = 2; // For retina screens only
#else
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_FALSE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    int const DPI = 1;
# endif

    // Open a window and create its OpenGL context
    GLFWwindow * window = glfwCreateWindow(width/DPI, height/DPI, "aogl", 0, 0);
    if( ! window )
    {
        fprintf( stderr, "Failed to open GLFW window\n" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }
    glfwMakeContextCurrent(window);

    // Init glew
    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        exit( EXIT_FAILURE );
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode( window, GLFW_STICKY_KEYS, GL_TRUE );

    // Enable vertical sync (on cards that support it)
    glfwSwapInterval( 1 );
    GLenum glerr = GL_NO_ERROR;
    glerr = glGetError();

    if (!imguiRenderGLInit(DroidSans_ttf, DroidSans_ttf_len))
    {
        fprintf(stderr, "Could not init GUI renderer.\n");
        exit(EXIT_FAILURE);
    }

    ////////////////////////////
    // Init viewer structures //
    ////////////////////////////
    // Camera
    Camera camera;
    camera_defaults(camera);

    // GUI
    GUIStates guiStates;
    init_gui_states(guiStates);

    // Viewport 
    glViewport( 0, 0, width, height);

    /////////////////////////////////////////////
    //  SHADERS
    /////////////////////////////////////////////

    // Try to load and compile shaders
    GLuint vertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "aogl.vert");
    GLuint geomShaderId = compile_shader_from_file(GL_GEOMETRY_SHADER, "aogl.geom");
    GLuint fragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "aogl.frag");
    GLuint blitVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "blit.vert");
    GLuint blitFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "blit.frag");
    GLuint pointLightVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "pointLight.vert");
    GLuint pointLightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "pointLight.frag");
    GLuint spotLightVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "spotLight.vert");
    GLuint spotLightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "spotLight.frag");
    GLuint directionalLightVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "directionalLight.vert");
    GLuint directionalLightFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "directionalLight.frag");
    GLuint shadowMapVertShaderId = compile_shader_from_file(GL_VERTEX_SHADER, "shadowMap.vert");
    GLuint shadowMapFragShaderId = compile_shader_from_file(GL_FRAGMENT_SHADER, "shadowMap.frag");
    
    GLuint programObject = glCreateProgram();
    glAttachShader(programObject, vertShaderId);
    glAttachShader(programObject, geomShaderId);
    glAttachShader(programObject, fragShaderId);
    glLinkProgram(programObject);

    GLuint programBlit = glCreateProgram();
    glAttachShader(programBlit, blitVertShaderId);
    glAttachShader(programBlit, blitFragShaderId);
    glLinkProgram(programBlit);

    GLuint programPointLight = glCreateProgram();
    glAttachShader(programPointLight, pointLightVertShaderId);
    glAttachShader(programPointLight, pointLightFragShaderId);
    glLinkProgram(programPointLight);

    GLuint programDirectionalLight = glCreateProgram();
    glAttachShader(programDirectionalLight, directionalLightVertShaderId);
    glAttachShader(programDirectionalLight, directionalLightFragShaderId);
    glLinkProgram(programDirectionalLight);

    GLuint programSpotLight = glCreateProgram();
    glAttachShader(programSpotLight, spotLightVertShaderId);
    glAttachShader(programSpotLight, spotLightFragShaderId);
    glLinkProgram(programSpotLight);

    GLuint programShadowMap = glCreateProgram();
    glAttachShader(programShadowMap, shadowMapVertShaderId);
    glAttachShader(programShadowMap, shadowMapFragShaderId);
    glLinkProgram(programShadowMap);

    if (check_link_error(programObject) < 0 &&
        check_link_error(programBlit) < 0 &&
        check_link_error(programPointLight) < 0 &&
        check_link_error(programDirectionalLight) < 0 &&
        check_link_error(programSpotLight) < 0)
        exit(1);

    //////////////////////////////////////////////////////
    // Objects & VAO / VBO
    //////////////////////////////////////////////////////

    // Create a Vertex Array Object
    GLuint vao[3];
    glGenVertexArrays(3, vao);

    // Create a VBO for each array
    GLuint vbo[10];
    glGenBuffers(10, vbo);

    //////////
    // CUBE //
    //////////
    // Object
    int cube_triangleCount = 12;
    int cube_triangleList[] = {0, 1, 2, 2, 1, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 10, 9, 11, 12, 13, 14, 14, 13, 15, 16, 17, 18, 19, 17, 20, 21, 22, 23, 24, 25, 26, };
    float cube_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,  1.f, 0.f,  1.f, 1.f,  0.f, 1.f,  1.f, 1.f,  0.f, 0.f, 0.f, 0.f, 1.f, 1.f,  1.f, 0.f,  };
    float cube_vertices[] = {-0.5, -0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, -0.5, 0.5, -0.5, 0.5, 0.5 };
    float cube_normals[] = {0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, -1, 0, 0, }; 

    // Bind the VAO
    glBindVertexArray(vao[0]);

    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_triangleList), cube_triangleList, GL_STATIC_DRAW);

    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[2]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_normals), cube_normals, GL_STATIC_DRAW);

    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[3]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_uvs), cube_uvs, GL_STATIC_DRAW);

    //////////
    // PLAN //
    //////////
    // Object
    int plane_triangleCount = 2;
    int plane_triangleList[] = {0, 1, 2, 2, 1, 3}; 
    float plane_uvs[] = {0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 1.f, 1.f};
    float plane_vertices[] = {-20.0, -2.0, 20.0, 20.0, -2.0, 20.0, -20.0, -2.0, -20.0, 20.0, -2.0, -20.0};
    float plane_normals[] = {0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0};

    // Bind the VAO
    glBindVertexArray(vao[1]);

    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[4]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_triangleList), plane_triangleList, GL_STATIC_DRAW);

    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[5]);
    glEnableVertexAttribArray(0); 
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_vertices), plane_vertices, GL_STATIC_DRAW);

    // Bind normals and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[6]);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*3, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_normals), plane_normals, GL_STATIC_DRAW);

    // Bind uv coords and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[7]);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane_uvs), plane_uvs, GL_STATIC_DRAW);

    //////////
    // Quad //
    //////////
    int quad_triangleCount = 2;
    int quad_triangleList[] = {0, 1, 2, 2, 1, 3};
    float quad_vertices[] = {-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0};
    
    // Bind the VAO
    glBindVertexArray(vao[2]);
    // Bind indices and upload data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[8]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_triangleList), quad_triangleList, GL_STATIC_DRAW);
    // Bind vertices and upload data
    glBindBuffer(GL_ARRAY_BUFFER, vbo[9]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GL_FLOAT)*2, (void*)0);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

    // Generate Shader Storage Objects
    GLuint ssbo[3];
    glGenBuffers(3, ssbo);

    // Nb Lights
    float pointLightCount = 1;
    float directionalLightCount = 1;
    float spotLightCount = 1;

    ///////////////////////////////////////////////////
    //  TEXTURES
    ///////////////////////////////////////////////////
    int x;
    int y;
    int comp;

    GLuint textures[6];
    glGenTextures(6, textures);

    // Texture 1
    unsigned char * diffuse = stbi_load("textures/spnza_bricks_a_diff.tga", &x, &y, &comp, 3);

    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, diffuse);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Texture 2
    unsigned char* specular = stbi_load("textures/spnza_bricks_a_spec.tga", &x, &y, &comp, 3);

    glBindTexture(GL_TEXTURE_2D, textures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, x, y, 0, GL_RGB, GL_UNSIGNED_BYTE, specular);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Texture color
    glBindTexture(GL_TEXTURE_2D, textures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Texture normal
    glBindTexture(GL_TEXTURE_2D, textures[3]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Texture depth
    glBindTexture(GL_TEXTURE_2D, textures[4]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Create shadow texture
    glBindTexture(GL_TEXTURE_2D, textures[5]);
    // Create empty texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 512, 512, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    // Bilinear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Color needs to be 0 outside of texture coordinates
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    ////////////////////
    //  Framebuffers  //
    ////////////////////
    // Deferred
    GLuint gbufferFbo;
    GLuint gbufferDrawBuffers[2];
    
    // Create Framebuffer Object
    glGenFramebuffers(1, &gbufferFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
    // Initialize DrawBuffers
    gbufferDrawBuffers[0] = GL_COLOR_ATTACHMENT0;
    gbufferDrawBuffers[1] = GL_COLOR_ATTACHMENT1;
    glDrawBuffers(2, gbufferDrawBuffers);

    // Attach textures to framebuffer
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 , GL_TEXTURE_2D, textures[2], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1 , GL_TEXTURE_2D, textures[3], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textures[4], 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        fprintf(stderr, "Error on building framebuffer\n");
        exit( EXIT_FAILURE );
    }

    // Create shadow FBO
    GLuint shadowFbo;
    glGenFramebuffers(1, &shadowFbo);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);

    // Create a render buffer since we don't need to read shadow color 
    // in a texture
    GLuint shadowRenderBuffer;
    glGenRenderbuffers(1, &shadowRenderBuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, shadowRenderBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGB, 512, 512);
    // Attach the renderbuffer
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, shadowRenderBuffer);

    // Attach the shadow texture to the depth attachment
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textures[5], 0);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
      fprintf(stderr, "Error on building shadow framebuffer\n");
      exit( EXIT_FAILURE );
    }
    // Back to the default framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ////////////////////////////////////////
    // UNIFORM location
    ////////////////////////////////////////
    // Objects
    GLuint timeLocation = glGetUniformLocation(programObject, "Time");
    GLuint diffuseLocation = glGetUniformLocation(programObject, "Diffuse");
    GLuint specularLocation = glGetUniformLocation(programObject, "Specular");
    GLuint specularPowerLocation = glGetUniformLocation(programObject, "SpecularPower");
    GLuint mvpLocation = glGetUniformLocation(programObject, "MVP");
    // Blit
    GLuint isDepthLocation = glGetUniformLocation(programBlit, "isDepth");
    GLuint textureLocation = glGetUniformLocation(programBlit, "Texture");
    // PointLight
    GLuint pl_colorBufferLocation = glGetUniformLocation(programPointLight, "ColorBuffer");
    GLuint pl_normalBufferLocation = glGetUniformLocation(programPointLight, "NormalBuffer");
    GLuint pl_depthBufferLocation = glGetUniformLocation(programPointLight, "DepthBuffer");
    GLuint pl_cameraPositionLocation = glGetUniformLocation(programPointLight, "CameraPosition");
    GLuint pl_screenToWorldLocation = glGetUniformLocation(programPointLight, "ScreenToWorld");
    // DirectionalLight
    GLuint dl_colorBufferLocation = glGetUniformLocation(programDirectionalLight, "ColorBuffer");
    GLuint dl_normalBufferLocation = glGetUniformLocation(programDirectionalLight, "NormalBuffer");
    GLuint dl_depthBufferLocation = glGetUniformLocation(programDirectionalLight, "DepthBuffer");
    GLuint dl_cameraPositionLocation = glGetUniformLocation(programDirectionalLight, "CameraPosition");
    GLuint dl_screenToWorldLocation = glGetUniformLocation(programDirectionalLight, "ScreenToWorld");
    // SpotLight
    GLuint sl_colorBufferLocation = glGetUniformLocation(programSpotLight, "ColorBuffer");
    GLuint sl_normalBufferLocation = glGetUniformLocation(programSpotLight, "NormalBuffer");
    GLuint sl_depthBufferLocation = glGetUniformLocation(programSpotLight, "DepthBuffer");
    GLuint sl_cameraPositionLocation = glGetUniformLocation(programSpotLight, "CameraPosition");
    GLuint sl_screenToWorldLocation = glGetUniformLocation(programSpotLight, "ScreenToWorld");
    // ShadowMap
    GLuint mvpLightLocation = glGetUniformLocation(programShadowMap, "MVPLight");
    GLuint objectToLightLocation = glGetUniformLocation(programShadowMap, "ObjectToLight");

    // Objects
    glProgramUniform1i(programObject, diffuseLocation, 0);
    glProgramUniform1i(programObject, specularLocation, 1);
    glProgramUniform1f(programObject, specularPowerLocation, 20);
    // Blit
    glProgramUniform1i(programBlit, isDepthLocation, 0);
    glProgramUniform1i(programBlit, textureLocation, 0);
    // PointLight
    glProgramUniform1i(programPointLight, pl_colorBufferLocation, 0);
    glProgramUniform1i(programPointLight, pl_normalBufferLocation, 1);
    glProgramUniform1i(programPointLight, pl_depthBufferLocation, 2);
    // DirectionalLight
    glProgramUniform1i(programDirectionalLight, dl_colorBufferLocation, 0);
    glProgramUniform1i(programDirectionalLight, dl_normalBufferLocation, 1);
    glProgramUniform1i(programDirectionalLight, dl_depthBufferLocation, 2);
    // SpotLight
    glProgramUniform1i(programSpotLight, sl_colorBufferLocation, 0);
    glProgramUniform1i(programSpotLight, sl_normalBufferLocation, 1);
    glProgramUniform1i(programSpotLight, sl_depthBufferLocation, 2);

    if (!checkError("Uniforms"))
        exit(1);

    ///////////////////////////////
    //  The LOOOOOP
    ///////////////////////////////
    do
    {
        t = glfwGetTime();
        float magik = (((int)(t*100) % 4000) - 2000)/(float)100;

        // Upload value
        glProgramUniform1f(programObject, timeLocation, t);

        // Mouse states
        int leftButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT );
        int rightButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_RIGHT );
        int middleButton = glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_MIDDLE );

        if( leftButton == GLFW_PRESS )
            guiStates.turnLock = true;
        else
            guiStates.turnLock = false;

        if( rightButton == GLFW_PRESS )
            guiStates.zoomLock = true;
        else
            guiStates.zoomLock = false;

        if( middleButton == GLFW_PRESS )
            guiStates.panLock = true;
        else
            guiStates.panLock = false;

        // Camera movements
        int altPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
        if (!altPressed && (leftButton == GLFW_PRESS || rightButton == GLFW_PRESS || middleButton == GLFW_PRESS))
        {
            double x; double y;
            glfwGetCursorPos(window, &x, &y);
            guiStates.lockPositionX = x;
            guiStates.lockPositionY = y;
        }
        if (altPressed == GLFW_PRESS)
        {
            double mousex; double mousey;
            glfwGetCursorPos(window, &mousex, &mousey);
            int diffLockPositionX = mousex - guiStates.lockPositionX;
            int diffLockPositionY = mousey - guiStates.lockPositionY;
            if (guiStates.zoomLock)
            {
                float zoomDir = 0.0;
                if (diffLockPositionX > 0)
                    zoomDir = -1.f;
                else if (diffLockPositionX < 0 )
                    zoomDir = 1.f;
                camera_zoom(camera, zoomDir * GUIStates::MOUSE_ZOOM_SPEED);
            }
            else if (guiStates.turnLock)
            {
                camera_turn(camera, diffLockPositionY * GUIStates::MOUSE_TURN_SPEED,
                            diffLockPositionX * GUIStates::MOUSE_TURN_SPEED);

            }
            else if (guiStates.panLock)
            {
                camera_pan(camera, diffLockPositionX * GUIStates::MOUSE_PAN_SPEED,
                            diffLockPositionY * GUIStates::MOUSE_PAN_SPEED);
            }
            guiStates.lockPositionX = mousex;
            guiStates.lockPositionY = mousey;
        }

        ///////////////////////////////
        //  Buffer Storage : Lights  //
        ///////////////////////////////
        int pointLightBufferSize = 4 * sizeof(int) + sizeof(PointLight) * pointLightCount;
        int directionalLightBufferSize = 4 * sizeof(int) + sizeof(DirectionalLight) * directionalLightCount;
        int spotLightBufferSize = 4 * sizeof(int) + sizeof(SpotLight) * spotLightCount;

        void * lightBuffer = NULL;

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[0]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, pointLightBufferSize, 0, GL_DYNAMIC_COPY);
        lightBuffer = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        ((int*) lightBuffer)[0] = pointLightCount;
        for (int i = 0; i < pointLightCount; ++i)
        {
            PointLight p =
            {
                glm::vec3(1, 2, 0), 0,
                glm::vec3(fabsf(cos(i*2.f)), 1.-fabsf(sinf(i)) , 0.5f + 0.5f-fabsf(cosf(i)) ),
                1.0
            };
            ((PointLight*) ((int*) lightBuffer + 4))[i] = p;
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[1]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, directionalLightBufferSize, 0, GL_DYNAMIC_COPY);
        lightBuffer = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        ((int*) lightBuffer)[0] = directionalLightCount;
        for (int i = 0; i < directionalLightCount; ++i)
        {
            DirectionalLight p =
            {
                glm::vec3(3, 3, 0), 0,
                glm::vec3(0.9, 0.9, 0.9),  
                .5
            };
            ((DirectionalLight*) ((int*) lightBuffer + 4))[i] = p;
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo[2]);
        glBufferData(GL_SHADER_STORAGE_BUFFER, spotLightBufferSize, 0, GL_DYNAMIC_COPY);
        lightBuffer = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        ((int*) lightBuffer)[0] = spotLightCount;
        for (int i = 0; i < spotLightCount; ++i)
        {
            SpotLight p =
            {
                glm::vec3(.8, 3, 0),
                45,
                glm::vec3(0, -1, 0),
                .5,
                 glm::vec3(.9, .9, .9),
                1.
            };
            ((SpotLight*) ((int*) lightBuffer + 4))[i] = p;
        }
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 0, ssbo[0], 0, pointLightBufferSize);
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 1, ssbo[1], 0, directionalLightBufferSize);
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, 2, ssbo[2], 0, spotLightBufferSize);

        //////////////////////////////////
        //  MVP Stuff (Camera & Light)  //
        //////////////////////////////////
        // Get camera matrices
        glm::mat4 projection = glm::perspective(45.0f, widthf / heightf, 0.1f, 100.f); 
        glm::mat4 worldToView = glm::lookAt(camera.eye, camera.o, camera.up);
        glm::mat4 objectToWorld;
        glm::mat4 mvp = projection * worldToView * objectToWorld;
        // Compute the inverse worldToScreen matrix
        glm::mat4 screenToWorld = glm::transpose(glm::inverse(mvp));

        // Light space matrices
        // From light space to shadow map screen space
        glm::mat4 projectionLight = glm::perspective(glm::radians(45*2.f), 1.f, 1.f, 100.f);
        // From world to light
        glm::mat4 worldToLight = glm::lookAt(glm::vec3(0, 3, 0), glm::vec3(0, 3, 0) + glm::vec3(0, -1, 0), glm::vec3(0.f, 0.f, -1.f));
        // From object to light (MV for light)
        glm::mat4 objectToLight = worldToLight * objectToWorld;
        // From object to shadow map screen space (MVP for light)
        glm::mat4 objectToLightScreen = projectionLight * objectToLight;
        // From world to shadow map screen space 
        glm::mat4 worldToLightScreen = projectionLight * worldToLight;

        // Send transformations
        glProgramUniformMatrix4fv(programObject, mvpLocation, 1, 0, glm::value_ptr(mvp));
        glProgramUniformMatrix4fv(programShadowMap, mvpLightLocation, 1, 0, glm::value_ptr(objectToLightScreen));
        glProgramUniformMatrix4fv(programShadowMap, objectToLightLocation, 1, 0, glm::value_ptr(objectToLight));

        // Send camera position
        glProgramUniform3f(programPointLight, pl_cameraPositionLocation, camera.eye.x, camera.eye.y, camera.eye.z);
        glProgramUniform3f(programDirectionalLight, dl_cameraPositionLocation, camera.eye.x, camera.eye.y, camera.eye.z);
        glProgramUniform3f(programSpotLight, sl_cameraPositionLocation, camera.eye.x, camera.eye.y, camera.eye.z);

        glProgramUniformMatrix4fv(programPointLight, pl_screenToWorldLocation, 1, 0, glm::value_ptr(screenToWorld));
        glProgramUniformMatrix4fv(programDirectionalLight, dl_screenToWorldLocation, 1, 0, glm::value_ptr(screenToWorld));
        glProgramUniformMatrix4fv(programSpotLight, sl_screenToWorldLocation, 1, 0, glm::value_ptr(screenToWorld));

        ////////////////////////
        //  Render ShadowMap  //
        ////////////////////////
        glEnable(GL_DEPTH_TEST);
        // Bind the shadow FBO
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFbo);

        // Set the viewport corresponding to shadow texture resolution
        glViewport(0, 0, 512, 512);

        // Clear only the depth buffer
        glClear(GL_DEPTH_BUFFER_BIT);

        glUseProgram(programShadowMap);

        // Render vaos
        glBindVertexArray(vao[0]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, 1);
        glBindVertexArray(vao[1]);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // Fallback to default framebuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Revert to window size viewport
        glViewport(0, 0, width, height);

        //////////////////////////////////////
        //  Render Object - AOGL in gBuffer //
        //////////////////////////////////////
        // Objects / aogl shaders
        glEnable(GL_DEPTH_TEST);

        // default frame buffer (screen)
        glBindFramebuffer(GL_FRAMEBUFFER, gbufferFbo);
        // Clear the front buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(programObject);

        // Bind texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures[1]);

        // Render vaos
        glBindVertexArray(vao[0]);
        glDrawElementsInstanced(GL_TRIANGLES, cube_triangleCount * 3, GL_UNSIGNED_INT, (void*)0, 1);
        glBindVertexArray(vao[1]);
        glDrawElements(GL_TRIANGLES, plane_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        ///////////////////////////
        //  Render FBO - screen  //
        ///////////////////////////
        // Bind gbuffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // Clear screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        glEnable(GL_BLEND);
        // Setup additive blending
        glBlendFunc(GL_ONE, GL_ONE);

        // Main Viewport
        glViewport(0, 0, width, height);

        glBindVertexArray(vao[2]);
        
        glActiveTexture(GL_TEXTURE0 + 0);
        glBindTexture(GL_TEXTURE_2D, textures[2]);
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, textures[3]);
        glActiveTexture(GL_TEXTURE0 + 2);
        glBindTexture(GL_TEXTURE_2D, textures[4]);

        // PointLight render
        glUseProgram(programPointLight);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // DirectionalLight render
        glUseProgram(programDirectionalLight);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        // SpotLight render
        glUseProgram(programSpotLight);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

        //////////////////
        // Blit screens //
        //////////////////
        glActiveTexture(GL_TEXTURE0);
        glDisable(GL_BLEND);

        // Use the blit program
        glUseProgram(programBlit);
        glBindVertexArray(vao[2]);
        glProgramUniform1i(programBlit, isDepthLocation, 0);

        // Viewport1
        glViewport(0, 0, width/4, height/4);
        glBindTexture(GL_TEXTURE_2D, textures[2]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        // Viewport2
        glViewport(width/4, 0, width/4, height/4);
        glBindTexture(GL_TEXTURE_2D, textures[3]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        // Viewport3
        glViewport(2*width/4, 0, width/4, height/4);
        glBindTexture(GL_TEXTURE_2D, textures[4]);
        glProgramUniform1i(programBlit, isDepthLocation, 1);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);
        // Viewport4
        glViewport(3*width/4, 0, width/4, height/4);
        glBindTexture(GL_TEXTURE_2D, textures[5]);
        glDrawElements(GL_TRIANGLES, quad_triangleCount * 3, GL_UNSIGNED_INT, (void*)0);

#if 1
        // Draw UI
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glViewport(0, 0, width, height);

        unsigned char mbut = 0;
        int mscroll = 0;
        double mousex; double mousey;
        glfwGetCursorPos(window, &mousex, &mousey);
        mousex*=DPI;
        mousey*=DPI;
        mousey = height - mousey;

        if( leftButton == GLFW_PRESS )
            mbut |= IMGUI_MBUT_LEFT;

        imguiBeginFrame(mousex, mousey, mbut, mscroll);
        int logScroll = 0;
        char lineBuffer[512];
        imguiBeginScrollArea("aogl", width - 210, height - 310, 200, 300, &logScroll);
        sprintf(lineBuffer, "FPS %f", fps);
        imguiLabel(lineBuffer);
        imguiSlider("Nb PointLights", &pointLightCount, 0, 100, 1);
        imguiSlider("Nb SpotLight", &spotLightCount, 0, 100, 1);
        imguiSlider("Nb DirectionalLight", &directionalLightCount, 0, 100, 1);

        imguiEndScrollArea();
        imguiEndFrame();
        imguiRenderGLDraw(width, height);

        glDisable(GL_BLEND);
#endif
        // Check for errors
        checkError("End loop");

        glfwSwapBuffers(window);
        glfwPollEvents();

        double newTime = glfwGetTime();
        fps = 1.f/ (newTime - t);
    } // Check if the ESC key was pressed
    while( glfwGetKey( window, GLFW_KEY_ESCAPE ) != GLFW_PRESS );

    // Unbind everything
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    exit( EXIT_SUCCESS );
}

// No windows implementation of strsep
char * strsep_custom(char **stringp, const char *delim)
{
    register char *s;
    register const char *spanp;
    register int c, sc;
    char *tok;
    if ((s = *stringp) == NULL)
        return (NULL);
    for (tok = s; ; ) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return (tok);
            }
        } while (sc != 0);
    }
    return 0;
}

int check_compile_error(GLuint shader, const char ** sourceBuffer)
{
    // Get error log size and print it eventually
    int logLength;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetShaderInfoLog(shader, logLength, &logLength, log);
        char *token, *string;
        string = strdup(sourceBuffer[0]);
        int lc = 0;
        while ((token = strsep(&string, "\n")) != NULL) {
           printf("%3d : %s\n", lc, token);
           ++lc;
        }
        fprintf(stderr, "Compile : %s", log);
        delete[] log;
    }
    // If an error happend quit
    int status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE)
        return -1;     
    return 0;
}

int check_link_error(GLuint program)
{
    // Get link error log size and print it eventually
    int logLength;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 1)
    {
        char * log = new char[logLength];
        glGetProgramInfoLog(program, logLength, &logLength, log);
        fprintf(stderr, "Link : %s \n", log);
        delete[] log;
    }
    int status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);        
    if (status == GL_FALSE)
        return -1;
    return 0;
}


GLuint compile_shader(GLenum shaderType, const char * sourceBuffer, int bufferSize)
{
    GLuint shaderObject = glCreateShader(shaderType);
    const char * sc[1] = { sourceBuffer };
    glShaderSource(shaderObject, 
                   1, 
                   sc,
                   NULL);
    glCompileShader(shaderObject);
    check_compile_error(shaderObject, sc);
    return shaderObject;
}

GLuint compile_shader_from_file(GLenum shaderType, const char * path)
{
    FILE * shaderFileDesc = fopen( path, "rb" );
    if (!shaderFileDesc)
        return 0;
    fseek ( shaderFileDesc , 0 , SEEK_END );
    long fileSize = ftell ( shaderFileDesc );
    rewind ( shaderFileDesc );
    char * buffer = new char[fileSize + 1];
    fread( buffer, 1, fileSize, shaderFileDesc );
    buffer[fileSize] = '\0';
    GLuint shaderObject = compile_shader(shaderType, buffer, fileSize );
    delete[] buffer;
    return shaderObject;
}


bool checkError(const char* title)
{
    int error;
    if((error = glGetError()) != GL_NO_ERROR)
    {
        std::string errorString;
        switch(error)
        {
        case GL_INVALID_ENUM:
            errorString = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorString = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorString = "GL_INVALID_OPERATION";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorString = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        case GL_OUT_OF_MEMORY:
            errorString = "GL_OUT_OF_MEMORY";
            break;
        default:
            errorString = "UNKNOWN";
            break;
        }
        fprintf(stdout, "OpenGL Error(%s): %s\n", errorString.c_str(), title);
    }
    return error == GL_NO_ERROR;
}

void camera_compute(Camera & c)
{
    c.eye.x = cos(c.theta) * sin(c.phi) * c.radius + c.o.x;   
    c.eye.y = cos(c.phi) * c.radius + c.o.y ;
    c.eye.z = sin(c.theta) * sin(c.phi) * c.radius + c.o.z;   
    c.up = glm::vec3(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
}

void camera_defaults(Camera & c)
{
    c.phi = 3.14/2.f;
    c.theta = 3.14/2.f;
    c.radius = 10.f;
    camera_compute(c);
}

void camera_zoom(Camera & c, float factor)
{
    c.radius += factor * c.radius ;
    if (c.radius < 0.1)
    {
        c.radius = 10.f;
        c.o = c.eye + glm::normalize(c.o - c.eye) * c.radius;
    }
    camera_compute(c);
}

void camera_turn(Camera & c, float phi, float theta)
{
    c.theta += 1.f * theta;
    c.phi   -= 1.f * phi;
    if (c.phi >= (2 * M_PI) - 0.1 )
        c.phi = 0.00001;
    else if (c.phi <= 0 )
        c.phi = 2 * M_PI - 0.1;
    camera_compute(c);
}

void camera_pan(Camera & c, float x, float y)
{
    glm::vec3 up(0.f, c.phi < M_PI ?1.f:-1.f, 0.f);
    glm::vec3 fwd = glm::normalize(c.o - c.eye);
    glm::vec3 side = glm::normalize(glm::cross(fwd, up));
    c.up = glm::normalize(glm::cross(side, fwd));
    c.o[0] += up[0] * y * c.radius * 2;
    c.o[1] += up[1] * y * c.radius * 2;
    c.o[2] += up[2] * y * c.radius * 2;
    c.o[0] -= side[0] * x * c.radius * 2;
    c.o[1] -= side[1] * x * c.radius * 2;
    c.o[2] -= side[2] * x * c.radius * 2;       
    camera_compute(c);
}

void init_gui_states(GUIStates & guiStates)
{
    guiStates.panLock = false;
    guiStates.turnLock = false;
    guiStates.zoomLock = false;
    guiStates.lockPositionX = 0;
    guiStates.lockPositionY = 0;
    guiStates.camera = 0;
    guiStates.time = 0.0;
    guiStates.playing = false;
}