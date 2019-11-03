#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "instance.h"

int main(int argc, char *argv[]) {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  GLFWwindow* window = glfwCreateWindow(800, 600, "soda: Vulkan", NULL, NULL);
  Instance instance = CreateInstance();
  DestroyInstance(instance);

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
