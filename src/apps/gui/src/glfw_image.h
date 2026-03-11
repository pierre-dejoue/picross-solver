#pragma once

#include <GLFW/glfw3.h>

#include <cassert>
#include <vector>

/**
 * A container for a GLFWimage and the associated pixel buffer
 *
 * Thankfully the expected pixel format is the same for mouse cursors and the window icon. According to the GLFW doc:
 *
 *   "The image data is 32-bit, little-endian, non-premultiplied RGBA, i.e. eight bits per channel
 *    with the red channel first. The pixels are arranged canonically as sequential rows, starting
 *    from the top-left corner."
 */
struct GLFWImageWithData
{
    GLFWImageWithData(unsigned int w, unsigned int h)
        : pixels(w * h * 4, 255u)
        , glfw_image()
    {
        glfw_image.width = static_cast<int>(w);
        glfw_image.height = static_cast<int>(h);
        assert(pixels.data());
        glfw_image.pixels = pixels.data();
    }

    std::vector<unsigned char>  pixels;
    GLFWimage                   glfw_image;
};
