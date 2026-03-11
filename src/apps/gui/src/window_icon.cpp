#include "window_icon.h"

#include "bitmap_image.h"
#include "bitmap_to_glfw_image.h"
#include "embedded_file.h"
#include "glfw_context.h"
#include "glfw_image.h"
#include "window_icon_files.h"

#include <GLFW/glfw3.h>

#include <stdutils/macros.h>

#include <cassert>
#include <vector>

#if defined(__APPLE__)
// On macOS the application icon is packaged inside the bundle
void set_window_icon(GLFWWindowContext&, const stdutils::io::ErrorHandler&)
{ }
#else
// Windows, Linux platforms
void set_window_icon(GLFWWindowContext& glfw_window_context, const stdutils::io::ErrorHandler& err_handler)
{
    assert(glfw_window_context.window() != nullptr);

    // Get the window icon in multiple resolutions
    const WindowIconFiles& icon_files = get_window_icon_files();

    // Transform to the GLFW image format
    std::vector<GLFWImageWithData> glfw_image_with_data;
    std::vector<GLFWimage> glfw_images_cpy;
    for (const auto& file : icon_files)
    {
        const bitmap::ColorImage image = parse_embedded_color_image(file, err_handler);
        if (image.empty())
            continue;
        auto& new_gflw_image = glfw_image_with_data.emplace_back(to_glfw_image(image));
        glfw_images_cpy.emplace_back(new_gflw_image.glfw_image);
    }

    if (glfw_images_cpy.empty())
    {
        if (err_handler) { err_handler(stdutils::io::Severity::WARN, "No window icon"); }
        return;
    }

    // Set window icon
    glfwSetWindowIcon(
        glfw_window_context.window(),
        static_cast<int>(glfw_images_cpy.size()),
        glfw_images_cpy.data()
    );
}
#endif
