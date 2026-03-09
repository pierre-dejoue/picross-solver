#pragma once

#include "bitmap_image.h"
#include "screen.h"
#include "window_layout.h"

#include <imgui_wrap.h>

#include <ostream>

inline ScreenPos to_screen_pos(ImVec2 vec2)
{
    return ScreenPos(vec2.x, vec2.y);
}

inline ScreenSize to_screen_size(ImVec2 vec2)
{
    return ScreenSize(vec2.x, vec2.y);
}

inline ImVec2 to_imgui_vec2(ScreenPos pos)
{
    return ImVec2(pos.x, pos.y);
}

namespace ImGui {

constexpr char* NO_SHORTCUT = nullptr;
constexpr bool* NO_POPEN = nullptr;
constexpr bool  NOT_SELECTED = false;

struct KeyShortcut
{
    KeyShortcut(ImGuiKeyChord kc, const char* lbl = nullptr)
        : key_chord(kc)
        , label(lbl)
    { }

    ImGuiKeyChord key_chord;
    const char* label;
};

void HelpMarker(const char* desc);          // Function taken from imgui_demo.cpp
void SetNextWindowPosAndSize(const WindowLayout& window_layout, ImGuiCond cond = 0);

} // namespace ImGui

/**
 * A texture identifier independent of the backend
 *
 * With OpenGL, ImageInternalId is simply the GLuint identifier returned by glGenTextures.
 */
using TextureInternalId = std::uintptr_t;

// Do not call this class ImGuiContext because this is an internal class of Dear ImGui
struct GLFWwindow;
class DearImGuiContext
{
public:
    explicit DearImGuiContext(GLFWwindow* glfw_window, bool& any_fatal_error) noexcept;
    ~DearImGuiContext();
    DearImGuiContext(const DearImGuiContext&) = delete;
    DearImGuiContext(DearImGuiContext&&) = delete;
    DearImGuiContext& operator=(const DearImGuiContext&) = delete;
    DearImGuiContext& operator=(DearImGuiContext&&) = delete;

    void new_frame() const;
    void render() const;
    void backend_info(std::ostream& out) const;

    /**
     * Raw texture upload to the GPU.
     * The resources are automatically freed up at the destruction of the DearImGuiContext.
     */
    TextureInternalId upload_texture(const bitmap::ColorImage& color_image);

private:
    void delete_texture(TextureInternalId internal_id);
    void clear_textures();

    std::vector<TextureInternalId> m_uploaded_textures;
};

struct ImGuiImage
{
    ImGuiImage(TextureInternalId tex_id, const bitmap::ColorImage& color_image);

    ImTextureRef texture_ref;
    ImVec2       image_size;
};

namespace ImGui {

void ImageWithBorder(const ImGuiImage img, ImVec4 border_color, float border_size);

} // namespace ImGui
