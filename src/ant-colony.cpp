// ==========================================================-GLAD_setup-==============================================================================
// https://glad.dav1d.de/#language=c&specification=gl&api=gl%3D4.6&api=gles1%3Dnone&api=gles2%3Dnone&api=glsc2%3Dnone&profile=core&extensions=GL_ARB_arrays_of_arrays&extensions=GL_ARB_buffer_storage&extensions=GL_ARB_clear_buffer_object&extensions=GL_ARB_clear_texture&extensions=GL_ARB_compute_shader&extensions=GL_ARB_copy_buffer&extensions=GL_ARB_copy_image&extensions=GL_ARB_direct_state_access&extensions=GL_ARB_shader_objects&extensions=GL_ARB_shader_storage_buffer_object&extensions=GL_ARB_texture_buffer_object&extensions=GL_ARB_texture_buffer_object_rgb32&loader=on
// ====================================================================================================================================================
#include "ant-colony.h"

#include "../opengl/imgui-src/imgui.h"
#include "../opengl/imgui-src/imgui_impl_glfw.h"
#include "../opengl/opengl-objects/computeShader.h"
#include "../opengl/opengl-objects/framebuffer.h"
#include "../opengl/opengl-objects/shader.h"
#include "../opengl/opengl-objects/shaderStorageBuffer.h"
#include "../opengl/opengl-objects/texture.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

// settings
const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

const unsigned int TEXTURE_WIDTH = 1920, TEXTURE_HEIGHT = 1080;

int WINDOW_WIDTH, WINDOW_HEIGHT;

const unsigned int numAnts = 1024 * 500;
const int radius = 200;

int main() {
  GLFWwindow *window;
  if (glfwSetup(window) == -1)
    return -1;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  //
  ImGuiWindowFlags wFlags;
  // wFlags |= ImGuiWindowFlags_NoTitleBar;
  // wFlags |= ImGuiWindowFlags_NoResize;
  // wFlags |= ImGuiWindowFlags_NoScrollbar;
  // wFlags |= ImGuiWindowFlags_NoScrollWithMouse;

  ImGui::StyleColorsDark();
  ImGuiStyle &style = ImGui::GetStyle();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    style.WindowRounding = 0.5f;
    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
  }
  // glad: load all OpenGL function pointers
  // ---------------------------------------
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cout << "Failed to initialize GLAD" << std::endl;
    return -1;
  }

  std::string project =
      "/" + std::filesystem::path(__FILE__).parent_path().string();

  // Shader creation
  Shader shader(project, "vertexShader.vs.glsl", "fragmentShader.fs.glsl");
  ComputeShader moveAntShader(project, "moveAnts");
  ComputeShader drawAntImage(project, "drawAntImage");

  // Texture creation
  Texture texFB(0, TEXTURE_WIDTH, TEXTURE_HEIGHT);
  Texture texCS(1, TEXTURE_WIDTH, TEXTURE_HEIGHT);

  // Set initial values of compute shaders
  moveAntShader.use();
  moveAntShader.setUIvec2("texSize", TEXTURE_WIDTH, TEXTURE_HEIGHT);

  drawAntImage.use();
  drawAntImage.setUIvec2("texSize", TEXTURE_WIDTH, TEXTURE_HEIGHT);
  drawAntImage.setInt("imgOutput", texCS.texNum);
  //

  Framebuffer fbo(&texFB);

  std::vector<Ant> ant(numAnts);
  std::vector<Image> image(TEXTURE_WIDTH * TEXTURE_HEIGHT);
  int simSpeed = 100;
  float vel = 1;
  float dist = 20;
  float angle = 15;
  float turnAngle = 5;

  for (size_t i = 0; i < numAnts; i++) {
    float r = std::sqrt(rand() / (float)RAND_MAX * std::pow(radius, 2));
    float a = rand() / (float)RAND_MAX * 2 * M_PI;
    // ant[i].pos = {r * std::cos(a) + (float)TEXTURE_WIDTH / 2,
    //               r * std::sin(a) + (float)TEXTURE_HEIGHT / 2};
    ant[i].pos = {rand() / (float)RAND_MAX * TEXTURE_WIDTH - 10,
                  rand() / (float)RAND_MAX * TEXTURE_HEIGHT - 10};
    // ant[i].dir = a + M_PI;
    ant[i].dir = rand() / (float)RAND_MAX * 2 * M_PI;
    ant[i].color = {0.0, 0.0, 0.0, 1.0};
    ant[i].u = 0.0;
  }

  ShaderStorageBuffer ssbo(0);
  ShaderStorageBuffer antImage(1);

  ssbo.storeData((const void *)&ant[0], sizeof(Ant) * ant.size());
  antImage.storeData((const void *)&image[0], sizeof(Image) * image.size());

  GLuint vbo, vao;
  std::array<int, 4> screenPosArray{};
  int windowPadding = 1;
  glfwGetWindowSize(window, &WINDOW_WIDTH, &WINDOW_HEIGHT);
  std::array<int, 2> buttonFrameSize = {200, WINDOW_WIDTH};

  ImGui_ImplGlfw_InitForOpenGL(window, true);
  ImGui_ImplOpenGL3_Init("#version 440");

  const char *fps = "FPS";
  const char *fpsAvg = "FPS";
  double totTime;
  size_t it = 0;
  bool runIt = true;
  bool pause = true;
  // render loop
  // -----------
  while (!glfwWindowShouldClose(window)) {
    it++;
    // input
    // -----
    // processInput(window);

    auto startTime = glfwGetTime();
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();

    fbo.bindFramebuffer();
    ImGui::NewFrame();
    {
      ImGui::SetNextWindowSize(ImVec2(buttonFrameSize[0], WINDOW_HEIGHT));
      // ImGui::SetCursorPos(ImVec2(0, 0));
      ImGui::SetNextWindowPos(ImVec2(0, 0));
      // Create Buttons window
      ImGui::Begin("Parameters");
      ImVec2 initPos = ImGui::GetCursorPos();
      ImGui::Text("%.5s", fps);
      ImGui::Text("%.5s", fpsAvg);
      if (ImGui::Button("close")) {
        std::cout << "True" << std::endl;
        glfwSetWindowShouldClose(window, 1);
      }
      static char shaderFile[32] = "moveAnts";
      ImGui::SliderInt("SimSpeed", &simSpeed, 100, 0);
      ImGui::SliderFloat("Velocity", &vel, 0, 1.0);
      ImGui::SliderFloat("Look ahead", &dist, 0, 20);
      ImGui::SliderFloat("Angle", &angle, 0, 20);
      ImGui::SliderFloat("Turning angle", &turnAngle, 0, 10);
      if (ImGui::Button("||")) {
        pause = true;
        runIt = false;
      }
      ImGui::SameLine();
      if (ImGui::Button(">"))
        runIt = true;
      ImGui::SameLine();
      if (ImGui::Button(">>")) {
        runIt = true;
        pause = false;
      }
      if (ImGui ::Button("Reset Sim"))
        resetSimulation(ant, ssbo);

      int padding = ImGui::GetStyle().FramePadding.y + 1;
      ImGui::SetCursorPos(ImVec2(8, WINDOW_HEIGHT - initPos.y - padding -
                                        ImGui::GetFrameHeight()));
      if (ImGui::Button("Recompile Shader")) {
        if (sizeof(shaderFile) / sizeof(shaderFile[0]) < 40) {
          std::cout << "Recompiling script: " << shaderFile << std::endl;
          shaderInputCallback(drawAntImage, shaderFile);
        }
      }
      ImGui::InputText("Shader", shaderFile, IM_ARRAYSIZE(shaderFile));
      ImGui::End();
    }

    {
      screenPosArray[2] = screenPosArray[0] + buttonFrameSize[0];
      ImGui::SetNextWindowPos(ImVec2(screenPosArray[2], screenPosArray[3]));
      ImGui::SetNextWindowSize(
          ImVec2(WINDOW_WIDTH - screenPosArray[2], WINDOW_HEIGHT));
      ImGui::Begin("Scene");
      const float windowWidth = ImGui::GetContentRegionAvail().x;
      const float windowHeight = ImGui::GetContentRegionAvail().y;

      fbo.rescaleFramebuffer(windowWidth, windowHeight);
      glViewport(0, 0, windowWidth, windowHeight);

      ImVec2 pos = ImGui::GetCursorScreenPos();

      ImGui::Image((ImTextureID)(intptr_t)texCS.texture,
                   ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
      ImGui::End();
    }

    ImGui::Render();
    fbo.unbindFramebuffer();
    if (runIt) {
      texCS.bindTexture();
      moveAntShader.use();
      moveAntShader.setUint("stage", 0);
      moveAntShader.setFloat("vel", vel);
      moveAntShader.setFloat("dist", dist);
      moveAntShader.setFloat("angle", angle / 180 * M_PI);
      moveAntShader.setFloat("turnAngle", turnAngle / 180 * M_PI);
      uint *totTimeInt = (uint *)&totTime;
      moveAntShader.setUint("t", *totTimeInt);
      glDispatchCompute((unsigned int)numAnts / 1024, 1, 1);

      drawAntImage.use();
      drawAntImage.setUint("t", *totTimeInt);
      glDispatchCompute((unsigned int)TEXTURE_WIDTH * TEXTURE_HEIGHT / 1024, 1,
                        1);

      glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

      texCS.unbindTexture();
      runIt = pause ? false : true;
    }

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      GLFWwindow *backupCurrentContext = glfwGetCurrentContext();
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
      glfwMakeContextCurrent(backupCurrentContext);
    }

    glfwSwapBuffers(window);

    std::this_thread::sleep_for(
        std::chrono::microseconds(100 * (100 - simSpeed)));
    totTime += glfwGetTime() - startTime;
    fps = (std::to_string(1 / (glfwGetTime() - startTime))).c_str();
    fpsAvg = (std::to_string(it / totTime) + " fps").c_str();
  }

  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

  // glfw: terminate, clearing all previously allocated GLFW resources.
  // ------------------------------------------------------------------
  glfwDestroyWindow(window);
  glfwTerminate();
  return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released
// this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
  if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

void createBox(GLuint &vbo, GLuint &vao) {
  float quadVertices[] = {
      // positions        // texture Coords
      -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
      1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,
  };
  // setup plane vao
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
               GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                        (void *)(3 * sizeof(float)));
}

void renderQuad(GLuint &vao) {
  glBindVertexArray(vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);
}

void shaderInputCallback(ComputeShader &shader, const char *file) {
  shader.fileName = file;
  shader.update();
}

int resetSimulation(std::vector<Ant> ant, ShaderStorageBuffer ssbo) {
  ssbo.updateData((const void *)&ant[0], sizeof(Ant) * ant.size());
  std::cout << "Resetting Simulation...\n";
  std::cout << sizeof(Ant) * ant.size() << std::endl;
  return 0;
}

// glfw: whenever the window size changed (by OS or user resize) this
// callback function executes
// ---------------------------------------------------------------------------------------------
void framebufferSizeCallback(GLFWwindow *window, int width, int height) {
  // make sure the viewport matches the new window dimensions; note that width
  // and height will be significantly larger than specified on retina
  // displays.
  WINDOW_WIDTH = width;
  WINDOW_HEIGHT = height;
  glViewport(0, 0, width, height);
}

int glfwSetup(GLFWwindow *&window) {
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // glfw window creation
  // --------------------

  window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Test Window",
                            /* glfwGetPrimaryMonitor() */ NULL, NULL);
  if (window == NULL) {
    std::cout << "Failed to create GLFW window" << std::endl;
    glfwTerminate();
    return -1;
  }

  glfwMakeContextCurrent(window);
  glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
  glfwSwapInterval(0);
  return 0;
}
