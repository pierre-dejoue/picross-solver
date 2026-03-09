#include "logo.h"

#include "bitmap_image.h"
#include "embedded_file.h"
#include "imgui_helpers.h"
#include "logo_file.h"

ImGuiImage make_app_logo(DearImGuiContext& imgui_context, const stdutils::io::ErrorHandler& err_handler)
{
    const auto embedded = get_logo_file();
    const bitmap::ColorImage logo = parse_embedded_color_image(embedded, err_handler);
    const auto internal_id = imgui_context.upload_texture(logo);
    return ImGuiImage(internal_id, logo);
}
