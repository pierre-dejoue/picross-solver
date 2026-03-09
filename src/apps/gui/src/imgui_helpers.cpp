#include "imgui_helpers.h"

#include <imgui_wrap.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_opengl3_loader.h>
#include <GLFW/glfw3.h>
#include "glfw_context.h"

namespace ImGui {

void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip())
    {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// Place the window in the working area, that is the position of the viewport minus task bars, menus bars, status bars, etc.
void SetNextWindowPosAndSize(const WindowLayout& window_layout, ImGuiCond cond)
{
    const auto work_tl_corner = ImGui::GetMainViewport()->WorkPos;
    const auto work_size = ImGui::GetMainViewport()->WorkSize;
    const ImVec2 tl_corner(window_layout.m_position.x + work_tl_corner.x, window_layout.m_position.y + work_tl_corner.y);
    ImGui::SetNextWindowPos(tl_corner, cond);
    ImGui::SetNextWindowSize(to_imgui_vec2(window_layout.window_size(to_screen_size(work_size))), cond);
}

} // namespace ImGui

DearImGuiContext::DearImGuiContext(GLFWwindow* glfw_window, bool& any_fatal_error) noexcept
{
    any_fatal_error = false;
    try
    {
        const bool versions_ok = IMGUI_CHECKVERSION();
        const auto* ctx = ImGui::CreateContext();

        // Setup Platform/Renderer backends
        const bool init_glfw = ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
        const bool init_opengl3 = ImGui_ImplOpenGL3_Init(glsl_version());

        any_fatal_error = !versions_ok || (ctx == nullptr) || !init_glfw || !init_opengl3;
    }
    catch(const std::exception&)
    {
        any_fatal_error = true;
    }
}

DearImGuiContext::~DearImGuiContext()
{
    clear_textures();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void DearImGuiContext::new_frame() const
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DearImGuiContext::render() const
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void DearImGuiContext::backend_info(std::ostream& out) const
{
    // Dear ImGui
    const ImGuiIO& io = ImGui::GetIO();
    out << "Dear ImGui " << IMGUI_VERSION
        << " (Backend platform: " << (io.BackendPlatformName ? io.BackendPlatformName : "NULL")
        << ", renderer: " << (io.BackendRendererName ? io.BackendRendererName : "NULL") << ")" << std::endl;

    // GLFW
    out << "GLFW " << GLFW_VERSION_MAJOR << "." << GLFW_VERSION_MINOR << "." << GLFW_VERSION_REVISION << std::endl;

    // OpenGL
    const auto* open_gl_version_str = glGetString(GL_VERSION);      // Will return NULL if there is no current OpenGL context!
    if (open_gl_version_str)
        out << "OpenGL Version: " << open_gl_version_str << std::endl;
    const auto* open_gl_vendor_str = glGetString(GL_VENDOR);
    const auto* open_gl_renderer_str = glGetString(GL_RENDERER);
    if (open_gl_vendor_str && open_gl_renderer_str)
        out << "OpenGL Vendor: " << open_gl_vendor_str << "; Renderer: " << open_gl_renderer_str << std::endl;
}

/**
 * Texture upload
 */
namespace {
namespace texture_details {

// Symbols missing from the ImGui OpenGL loader
#define GL_RGBA8                          0x8058
#define GL_CLAMP_TO_BORDER                0x812D

// Supported input image types
enum class ImageType
{
    Undefined,
    ColorImage,
};

// Texture traits depending in the image type
template <typename Bitmap>
struct TextureTraits
{
    static constexpr ImageType image_type      = ImageType::Undefined;
    static constexpr GLint     internal_format = 0;
    static constexpr GLenum    format          = 0u;
    static constexpr GLenum    type            = 0u;
};

template <>
struct TextureTraits<bitmap::ColorImage>
{
    static constexpr ImageType image_type      = ImageType::ColorImage;
    static constexpr GLint     internal_format = GL_RGBA8;
    static constexpr GLenum    format          = GL_RGBA;
    static constexpr GLenum    type            = GL_UNSIGNED_BYTE;
};

template <typename Bitmap>
void upload_generic_texture(GLuint texture_id, const Bitmap& input_bitmap)
{
    assert(texture_id);

    const GLsizei row_stride_i = static_cast<GLsizei>(input_bitmap.row_stride());
    const GLsizei nb_rows_i    = static_cast<GLsizei>(input_bitmap.height());

    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(
        /* target */            GL_TEXTURE_2D,
        /* level */             0,
        /* internalformat */    TextureTraits<Bitmap>::internal_format,
        /* width */             row_stride_i,
        /* height */            nb_rows_i,
        /* border */            0,
        /* format */            TextureTraits<Bitmap>::format,
        /* type */              TextureTraits<Bitmap>::type,
        /* data */              reinterpret_cast<const void*>(input_bitmap.raw_data())
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace texture_details

template <typename Bitmap>
TextureInternalId create_and_upload_generic_texture(const Bitmap& input_bitmap)
{
    GLuint texture_id{0};
    glGenTextures(1, &texture_id);
    if (texture_id)
    {
        texture_details::upload_generic_texture<Bitmap>(texture_id, input_bitmap);
    }
    return static_cast<TextureInternalId>(texture_id);
}

} // namespace

TextureInternalId DearImGuiContext::upload_texture(const bitmap::ColorImage& color_image)
{
    const auto internal_id = create_and_upload_generic_texture(color_image);
    if (internal_id)
    {
        m_uploaded_textures.push_back(internal_id);
    }
    return internal_id;
}

void DearImGuiContext::delete_texture(TextureInternalId internal_id)
{
    GLuint texture_id = static_cast<GLuint>(internal_id);
    glDeleteTextures(1, &texture_id);
}

void DearImGuiContext::clear_textures()
{
    for (TextureInternalId internal_id : m_uploaded_textures)
    {
        assert(internal_id);
        delete_texture(internal_id);
    }
    m_uploaded_textures.clear();
}

ImGuiImage::ImGuiImage(TextureInternalId tex_id, const bitmap::ColorImage& color_image)
    : texture_ref(static_cast<ImTextureID>(tex_id))
    , image_size{}
{
    const bitmap::coord_2d_t& bitmap_size = color_image.size();
    image_size = ImVec2{static_cast<float>(bitmap_size[0]), static_cast<float>(bitmap_size[1])};
}

void ImGui::ImageWithBorder(const ImGuiImage img, ImVec4 border_color, float border_size)
{
    ImGui::PushStyleColor(ImGuiCol_Border, border_color);
    ImGui::PushStyleVar(ImGuiStyleVar_ImageBorderSize, border_size);
    ImGui::Image(img.texture_ref, img.image_size);
    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
}
