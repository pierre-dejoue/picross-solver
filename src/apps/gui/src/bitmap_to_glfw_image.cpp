#include "bitmap_to_glfw_image.h"

#include "bitmap_image.h"
#include "glfw_image.h"

#include <GLFW/glfw3.h>

#include <cassert>

GLFWImageWithData to_glfw_image(const bitmap::ColorImage& color_image)
{
    const unsigned int w = color_image.width();
    const unsigned int h = color_image.height();
    GLFWImageWithData glfw_image(w, h);
    unsigned char* out_row = glfw_image.pixels.data();
    assert(out_row != nullptr);
    for (unsigned int y = 0; y < h; y++)
    {
        const bitmap::ColorPixel* in_row = color_image.row(y);
        assert(in_row != nullptr);
        for (unsigned int x = 0; x < w; x++)
        {
            const auto& pixel = *in_row++;
            *out_row++ = pixel.r;
            *out_row++ = pixel.g;
            *out_row++ = pixel.b;
            *out_row++ = pixel.a;
        }
    }
    return glfw_image;
}

bitmap::ColorImage to_color_image(const GLFWimage& glfw_image)
{
    const auto w = static_cast<unsigned int>(glfw_image.width);
    const auto h = static_cast<unsigned int>(glfw_image.height);
    bitmap::ColorImage color_image(w, h);
    const unsigned char* in_row = glfw_image.pixels;
    for (unsigned int y = 0; y < h; y++)
    {
        bitmap::ColorPixel* out_row = color_image.row(y);
        assert(out_row != nullptr);
        for (unsigned int x = 0; x < w; x++)
        {
            auto& pixel = *out_row++;
            pixel.r = *in_row++;
            pixel.g = *in_row++;
            pixel.b = *in_row++;
            pixel.a = *in_row++;
        }
    }
    return color_image;
}
