#include <GLES2/gl2.h>
#include <climits>
#include <fstream>
#include <glad/glad.h>
#include "SDL3/SDL_video.h"
#include <SDL3/SDL.h>
#include <sstream>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#include <iostream>

#define JACOBI_ITERATION_COUNT 128

SDL_Window* window = nullptr;
SDL_GLContext glContext = nullptr;
GLuint VAO, VBO, EBO;
GLuint shaderProgram;
GLuint normalTexture, divergenceTexture, outputTexture, heightMapTexture1, heightMapTexture2, minMaxTexture;

int width, height;
bool run = true;

/* Helper fgv shader-ekhez */
GLuint CompileShader(GLenum type, const char* filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open shader file: " << filepath << "\n";
        exit(1);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string code = buffer.str();
    const char* src = code.c_str();

    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
        exit(1);
    }
    return shader;
}

/* Szintén helper fgv shader programokhoz */
GLuint CreateProgram(GLuint shader1, GLuint shader2 = UINT_MAX) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, shader1);
    if (shader2 != UINT_MAX) glAttachShader(prog, shader2);
    glLinkProgram(prog);

    GLint ok;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "Program link error: " << log << "\n";
        exit(1);
    }
    return prog;
}

/* Előkészíti az SDL3 + OpenGL 4.3-as környezetet */
void InitializeWindow() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    window = SDL_CreateWindow("Texture Display", width, height, SDL_WINDOW_OPENGL);
    glContext = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, glContext);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        exit(1);
    }

    int drawableW, drawableH;
    SDL_GetWindowSize(window, &drawableW, &drawableH);
    glViewport(0, 0, width, height);
}

void LoadTexture(unsigned char* data, GLuint* texture) {

    glGenTextures(1, texture);
    glBindTexture(GL_TEXTURE_2D, *texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
}

/* Elkészíti a quad-ot amire kirajzoljuk a végső height map-ot */
void SetupQuad() {
    float vertices[] = {
        -1.f, -1.f,    0.f, 0.f,
         1.f, -1.f,    1.f, 0.f,
         1.f,  1.f,    1.f, 1.f,
        -1.f,  1.f,    0.f, 1.f
    };

    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

/* Rajzol 👍 */
void Render() {
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    SDL_GL_SwapWindow(window);
}

/* Elkészíti a divergenciát tartalmazó textúrát a normal map alapján */
void GenerateDivergenceMap() {
    glGenTextures(1, &divergenceTexture);
    glBindTexture(GL_TEXTURE_2D, divergenceTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width, height);

    GLuint compProgram = CreateProgram(CompileShader(GL_COMPUTE_SHADER, "sources/comp_divergence.glsl"));
    
    glUseProgram(compProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glUniform1i(glGetUniformLocation(compProgram, "normalTex"), 0);

    glBindImageTexture(1, divergenceTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    int workgroupSizeX = (width + 15) / 16;
    int workgroupSizeY = (height + 15) / 16;
    glDispatchCompute(workgroupSizeX, workgroupSizeY, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

/* Lefuttatja a felállított poisson-rendszer megoldásáre szükséges algoritmust */
void JacobiSolver() {
    std::vector<float> zero(width * height, 0.0f);

    glGenTextures(1, &heightMapTexture1);
    glBindTexture(GL_TEXTURE_2D, heightMapTexture1);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, zero.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &heightMapTexture2);
    glBindTexture(GL_TEXTURE_2D, heightMapTexture2);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width, height);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, zero.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLuint compProgram = CreateProgram(CompileShader(GL_COMPUTE_SHADER, "sources/comp_jacobi.glsl"));
    glUseProgram(compProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, divergenceTexture);
    glUniform1i(glGetUniformLocation(compProgram, "divergenceTex"), 0);

    int workgroupSizeX = (width + 15) / 16;
    int workgroupSizeY = (height + 15) / 16;

    for (int i = 0; i < JACOBI_ITERATION_COUNT; ++i) {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, heightMapTexture1);
        glUniform1i(glGetUniformLocation(compProgram, "inputHeightMap"), 1);
        glBindImageTexture(2, heightMapTexture2, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

        glDispatchCompute(workgroupSizeX, workgroupSizeY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        std::swap(heightMapTexture1, heightMapTexture2);
    }
}

/* Kiszámítja redukálással a Jacobi által elkészített height map min és max értékét normalizálásra */
void GenerateMinMax()
{
    GLuint heightMapTex = JACOBI_ITERATION_COUNT % 2 == 0 ? heightMapTexture2 : heightMapTexture1;
    glGenTextures(1, &minMaxTexture);
    glBindTexture(GL_TEXTURE_2D, minMaxTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, 1, 1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    GLuint compProgram = CreateProgram(CompileShader(GL_COMPUTE_SHADER, "sources/comp_minmax.glsl"));

    glUseProgram(compProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightMapTex);
    glUniform1i(glGetUniformLocation(compProgram, "heightMap"), 0);

    glBindImageTexture(1, minMaxTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG32F);

    glDispatchCompute(
        (width + 15) / 16,
        (height + 15) / 16,
        1
    );

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

/* A min és max érték alapján véglegesíti a height map-ot */
void FinalizeHeightMap()
{
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32F, width, height);

    std::vector<float> zero(width * height, 0.0f);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                    GL_RED, GL_FLOAT, zero.data());

    GLuint compProgram = CreateProgram(CompileShader(GL_COMPUTE_SHADER, "sources/comp_finalize_height.glsl"));
    
    glUseProgram(compProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, JACOBI_ITERATION_COUNT % 2 == 0 ? heightMapTexture2 : heightMapTexture1);
    glUniform1i(glGetUniformLocation(compProgram, "jacobiHeightMap"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, minMaxTexture);
    glUniform1i(glGetUniformLocation(compProgram, "minmax"), 1);

    glBindImageTexture(2, outputTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

    int workgroupSizeX = (width + 15) / 16;
    int workgroupSizeY = (height + 15) / 16;
    glDispatchCompute(workgroupSizeX, workgroupSizeY, 1);

    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindTexture(GL_TEXTURE_2D, outputTexture);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    std::vector<float> pixels(width * height);

    glGetTexImage(
        GL_TEXTURE_2D,
        0,
        GL_RED,
        GL_FLOAT,
        pixels.data()
    );

}

/* Mindent csinál ami kell hogy el lehessen kezdeni a konvertálást */
void InitializeProgram(const char* path)
{
    int ch;
    unsigned char* data = stbi_load(path, &width, &height, &ch, 4);
    if (!data) {
        std::cerr << "Failed to load image: " << path << "\n";
        exit(1);
    }
    
    InitializeWindow();
    LoadTexture(data, &normalTexture);

    GLuint vert = CompileShader(GL_VERTEX_SHADER, "sources/vert.glsl");
    GLuint frag = CompileShader(GL_FRAGMENT_SHADER, "sources/frag.glsl");
    shaderProgram = CreateProgram(vert, frag);
    
    glGenTextures(1, &outputTexture);
    glBindTexture(GL_TEXTURE_2D, outputTexture);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, width, width);

    SetupQuad();
}

int main(int argc, char* argv[]) {

    if (argc < 2) InitializeProgram("sources/example1.png");
    else InitializeProgram(argv[1]);

    GenerateDivergenceMap();
    JacobiSolver();
    GenerateMinMax();
    FinalizeHeightMap();

    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) run = false;
        }
        Render();
    }

    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}