#pragma once

#include "bitmap_image.h"
#include "glfw_image.h"

#include <GLFW/glfw3.h>

// ColorImage <-> GLFWimage
GLFWImageWithData to_glfw_image(const bitmap::ColorImage& color_image);
bitmap::ColorImage to_color_image(const GLFWimage& glfw_image);
