#include <iostream>
#include <stdio.h>

#include "SDL.h"
#include <GL/glew.h>

#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "glwindow.h"
#include "geometry.h"

using namespace std;

float light1x = 0.0f;
float light1y = 0.0f;
float light1z = 0.0f;

float light2x = 0.0f;
float light2y = 0.0f;
float light2z = 0.0f;

int lightNo = 0;

const char* glGetErrorString(GLenum error)
{
    switch(error)
    {
    case GL_NO_ERROR:
        return "GL_NO_ERROR";
    case GL_INVALID_ENUM:
        return "GL_INVALID_ENUM";
    case GL_INVALID_VALUE:
        return "GL_INVALID_VALUE";
    case GL_INVALID_OPERATION:
        return "GL_INVALID_OPERATION";
    case GL_INVALID_FRAMEBUFFER_OPERATION:
        return "GL_INVALID_FRAMEBUFFER_OPERATION";
    case GL_OUT_OF_MEMORY:
        return "GL_OUT_OF_MEMORY";
    default:
        return "UNRECOGNIZED";
    }
}

void glPrintError(const char* label="Unlabelled Error Checkpoint", bool alwaysPrint=false)
{
    GLenum error = glGetError();
    if(alwaysPrint || (error != GL_NO_ERROR))
    {
        printf("%s: OpenGL error flag is %s\n", label, glGetErrorString(error));
    }
}

GLuint loadShader(const char* shaderFilename, GLenum shaderType)
{
    FILE* shaderFile = fopen(shaderFilename, "r");
    if(!shaderFile)
    {
        return 0;
    }

    fseek(shaderFile, 0, SEEK_END);
    long shaderSize = ftell(shaderFile);
    fseek(shaderFile, 0, SEEK_SET);

    char* shaderText = new char[shaderSize+1];
    size_t readCount = fread(shaderText, 1, shaderSize, shaderFile);
    shaderText[readCount] = '\0';
    fclose(shaderFile);

    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, (const char**)&shaderText, NULL);
    glCompileShader(shader);

    delete[] shaderText;

    return shader;
}

GLuint loadShaderProgram(const char* vertShaderFilename,
                       const char* fragShaderFilename)
{
    GLuint vertShader = loadShader(vertShaderFilename, GL_VERTEX_SHADER);
    GLuint fragShader = loadShader(fragShaderFilename, GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    GLint linkStatus;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    if(linkStatus != GL_TRUE)
    {
        GLsizei logLength = 0;
        GLchar message[1024];
        glGetProgramInfoLog(program, 1024, &logLength, message);
        cout << "Shader load error: " << message << endl;
        return 0;
    }

    return program;
}

OpenGLWindow::OpenGLWindow()
{
    
    parentEntity.position = glm::vec3(0.0f, 0.0f, 0.0f);
    parentEntity.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    parentEntity.scale = glm::vec3(1.0f, 1.0f, 1.0f);

    childEntity.position = glm::vec3(1.0f, 0.0f, 0.0f);
    childEntity.rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    childEntity.scale = glm::vec3(0.5f, 0.5f, 0.5f);

    colorIndex = 0;
    translateDirection = 0;
    rotateDirection = 0;
    scaleDirection = 0;
}


void OpenGLWindow::initGL()
{

    // We need to first specify what type of OpenGL context we need before we can create the window
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    sdlWin = SDL_CreateWindow("OpenGL Prac 1",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 480, SDL_WINDOW_OPENGL);
    if(!sdlWin)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Error", "Unable to create window", 0);
    }
    SDL_GLContext glc = SDL_GL_CreateContext(sdlWin);
    SDL_GL_MakeCurrent(sdlWin, glc);
    SDL_GL_SetSwapInterval(1);

    glewExperimental = true;
    GLenum glewInitResult = glewInit();
    glGetError(); // Consume the error erroneously set by glewInit()
    if(glewInitResult != GLEW_OK)
    {
        const GLubyte* errorString = glewGetErrorString(glewInitResult);
        cout << "Unable to initialize glew: " << errorString;
    }

    int glMajorVersion;
    int glMinorVersion;
    glGetIntegerv(GL_MAJOR_VERSION, &glMajorVersion);
    glGetIntegerv(GL_MINOR_VERSION, &glMinorVersion);
    cout << "Loaded OpenGL " << glMajorVersion << "." << glMinorVersion << " with:" << endl;
    cout << "\tVendor: " << glGetString(GL_VENDOR) << endl;
    cout << "\tRenderer: " << glGetString(GL_RENDERER) << endl;
    cout << "\tVersion: " << glGetString(GL_VERSION) << endl;
    cout << "\tGLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    glCullFace(GL_BACK);
    glClearColor(0,0,0,1);

    
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    shader = loadShaderProgram("simple.vert", "simple.frag");
    
    
    glUseProgram(shader);

    // Set our viewing and projection matrices, since these do not change over time
    glm::mat4 projectionMat = glm::perspective(glm::radians(90.0f), 4.0f/3.0f, 0.1f, 10.0f);
    int projectionMatrixLoc = glGetUniformLocation(shader, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixLoc, 1, false, &projectionMat[0][0]);

    glm::vec3 eyeLoc(0.0f, 0.0f, 4.0f);
    glm::vec3 targetLoc(0.0f, 0.0f, 0.0f);
    glm::vec3 upDir(0.0f, 1.0f, 0.0f);
    glm::mat4 viewingMat = glm::lookAt(eyeLoc, targetLoc, upDir);

    int viewingMatrixLoc = glGetUniformLocation(shader, "viewingMatrix");
    glUniformMatrix4fv(viewingMatrixLoc, 1, false, &viewingMat[0][0]);


    int eyePos = glGetUniformLocation(shader, "eyePos");
    glUniform3fv(eyePos, 1, &eyeLoc[0]);

    // Load the model that we want to use and buffer the vertex attributes
    geometry.loadFromOBJFile("sphere.obj");

    int vertexLoc = glGetAttribLocation(shader, "position");
    glGenBuffers(1, &vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, 3*geometry.vertexCount()*sizeof(float),
                 geometry.vertexData(), GL_STATIC_DRAW);
    glVertexAttribPointer(vertexLoc, 3, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(vertexLoc);
    glPrintError("Setup complete", true);

    int normalLoc = glGetAttribLocation(shader, "normalPos");
    glGenBuffers(1, &normalBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffer);
    glBufferData(GL_ARRAY_BUFFER, 3*geometry.vertexCount()*sizeof(float),
                 geometry.normalData(), GL_STATIC_DRAW);
    glVertexAttribPointer(normalLoc, 3, GL_FLOAT, false, 0, 0);
    glEnableVertexAttribArray(normalLoc);
    glPrintError("Setup complete", true);


}


void OpenGLWindow::render()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float entityColors[15] = { 1.0f, 1.0f, 1.0f,
                               1.0f, 0.0f, 0.0f,
                               0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f,
                               0.2f, 0.2f, 0.2f };

    
    float light1Color[4] = { 0.5f, 1.0f, 1.0f, 1.0f};
    float light2Color[4] = { 1.0f, 0.2f, 1.0f, 1.0f};


    glm::vec3 light2Pos(-2.0f, 0.0f, 1.0f);
    glm::vec3 light1Pos(2.0f, 0.0f, 1.0f);
    
    // move light1
    light1Pos.x += light1x;
    light1Pos.y += light1y;
    light1Pos.z += light1z;
    //move light2
    light2Pos.x += light2x;
    light2Pos.y += light2y;
    light2Pos.z += light2z;

    // NOTE: glm::translate/rotate/scale apply the transformation by right-multiplying by the
    //       corresponding transformation matrix (T). IE glm::translate(M, v) = M * T, not T*M
    //       This means that the transformation you apply last, will effectively occur first
    glm::mat4 modelMat(1.0f);
    modelMat = glm::translate(modelMat, parentEntity.position);
    modelMat = glm::rotate(modelMat, parentEntity.rotation.z, glm::vec3(0.0f, 0.0f, 1.0f));
    modelMat = glm::rotate(modelMat, parentEntity.rotation.y, glm::vec3(0.0f, 1.0f, 0.0f));
    modelMat = glm::rotate(modelMat, parentEntity.rotation.x, glm::vec3(1.0f, 0.0f, 0.0f));
    modelMat = glm::scale(modelMat, parentEntity.scale);

    int modelMatrixLoc = glGetUniformLocation(shader, "modelMatrix");
    glUniformMatrix4fv(modelMatrixLoc, 1, false, &modelMat[0][0]);

    int colorLoc = glGetUniformLocation(shader, "objectColor");
    glUniform3fv(colorLoc, 1, &entityColors[3*colorIndex]);

    int light1Col = glGetUniformLocation(shader, "light1Color");
    glUniform3fv(light1Col, 1, &light1Color[0]);

    int light2Col = glGetUniformLocation(shader, "light2Color");
    glUniform3fv(light2Col, 1, &light2Color[0]);

    int light1Loc = glGetUniformLocation(shader, "light1Pos");
    glUniform3fv(light1Loc, 1, &light1Pos[0]);

    int light2Loc = glGetUniformLocation(shader, "light2Pos");
    glUniform3fv(light2Loc, 1, &light2Pos[0]);
    
    glDrawArrays(GL_TRIANGLES, 0, geometry.vertexCount());

    // Swap the front and back buffers on the window, effectively putting what we just "drew"
    // onto the screen (whereas previously it only existed in memory)
    SDL_GL_SwapWindow(sdlWin);
}

// The program will exit if this function returns false
bool OpenGLWindow::handleEvent(SDL_Event e)
{
    
    // A list of keycode constants is available here: https://wiki.libsdl.org/SDL_Keycode
    // Note that SDL provides both Scancodes (which correspond to physical positions on the keyboard)
    // and Keycodes (which correspond to symbols on the keyboard, and might differ across layouts)
    if(e.type == SDL_KEYDOWN)
    {

        if(e.key.keysym.sym == SDLK_ESCAPE)
        {
            return false;
        }
        else if(e.key.keysym.sym == SDLK_1)
        {
            colorIndex = 0;
        }
        else if(e.key.keysym.sym == SDLK_2)
        {
            colorIndex = 1;
        }
        else if(e.key.keysym.sym == SDLK_3)
        {
            colorIndex = 2;
        }
        else if(e.key.keysym.sym == SDLK_4)
        {
            colorIndex = 3;
        }
        else if(e.key.keysym.sym == SDLK_5)
        {
            colorIndex = 4;
        }

        else if(e.key.keysym.sym == SDLK_q)
        {
            parentEntity.position[translateDirection] -= 0.5f;
        }

        // Light 2X down
        else if(e.key.keysym.sym == SDLK_i)
        {
            light2x -= 0.3f;
            
        }
        // Light 1X down
        else if(e.key.keysym.sym == SDLK_KP_7)
        {
            light1x -= 0.3f;     
        }
        // Light 2X up
        else if(e.key.keysym.sym == SDLK_o)
        {
            light2x += 0.3f;
        }
        // Light 1X up
        else if(e.key.keysym.sym == SDLK_KP_8)
        {
            light1x += 0.3f;
        }
        // Light 2Y down
        else if(e.key.keysym.sym == SDLK_j)
        {
            light2y -= 0.3f;
        }
        // Light 1Y down
        else if(e.key.keysym.sym == SDLK_KP_4)
        {
            light1y -= 0.3f;
        }
        // Light 2Y Up
        else if(e.key.keysym.sym == SDLK_k)
        {
            light2y += 0.3f;
        }
        // Light 1Y Up
        else if(e.key.keysym.sym == SDLK_KP_5)
        {
            light1y += 0.3f;
        }
        // Light 2Z Down
        else if(e.key.keysym.sym == SDLK_n)
        {
            light2z -= 0.3f;
        }
        // Light 1Z Down
        else if(e.key.keysym.sym == SDLK_KP_1)
        {
            light1z -= 0.3f;
        }
        // Light 2Z Up
        else if(e.key.keysym.sym == SDLK_m)
        {
            light2z += 0.3f;
        }
        // Light 1Z Up
        else if(e.key.keysym.sym == SDLK_KP_2)
        {
            light1z += 0.3f;
        }
        else if(e.key.keysym.sym == SDLK_w)
        {
            translateDirection = (translateDirection+1)%3;
        }
        else if(e.key.keysym.sym == SDLK_e)
        {
            parentEntity.position[translateDirection] += 0.5f;
        }

        else if(e.key.keysym.sym == SDLK_a)
        {
            parentEntity.rotation[rotateDirection] -= glm::radians(15.0f);
        }
        else if(e.key.keysym.sym == SDLK_s)
        {
            rotateDirection = (rotateDirection+1)%3;
        }
        else if(e.key.keysym.sym == SDLK_d)
        {
            parentEntity.rotation[rotateDirection] += glm::radians(15.0f);
        }

        else if(e.key.keysym.sym == SDLK_z)
        {
            parentEntity.scale[scaleDirection] -= 0.2f;
        }
        else if(e.key.keysym.sym == SDLK_x)
        {
            scaleDirection = (scaleDirection+1)%3;
        }
        else if(e.key.keysym.sym == SDLK_c)
        {
            parentEntity.scale[scaleDirection] += 0.2f;
        }
    }
    return true;
}

void OpenGLWindow::cleanup()
{
    glDeleteBuffers(1, &vertexBuffer);
    glDeleteVertexArrays(1, &vao);
    SDL_DestroyWindow(sdlWin);
}
