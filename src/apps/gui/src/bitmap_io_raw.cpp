#include "bitmap_io_raw.h"

#include <stdutils/macros.h>
#include <stdutils/memory.h>

#include <cassert>
#include <cstdint>

namespace bitmap {
namespace io {

/**
 *
 * The RAW format is an internal format mostly used for in-memory images.
 * It is basically a direct serialization of a ColorImage.
 *
 * The format is as follows:
 *
 *  HEADER (16 bytes):
 *  {
 *      UINT32      width;
 *      UINT32      height;
 *      UINT32      reserved1;
 *      UINT32      reserved2;
 *  }
 *
 *  PIXEL ARRAY ((4 * width * height) bytes): Top to bottom rows of pixels encoded as 32 bit RGBA
 *
 */

namespace {

struct RawHeader
{
    std::uint32_t   width{0};
    std::uint32_t   height{0};
    std::uint32_t   reserved1{0};
    std::uint32_t   reserved2{0};
};

} // namespace

ColorImage read_raw(const std::byte* raw_buffer, std::size_t buffer_size, const stdutils::io::ErrorHandler& err_handler, const char* raw_name) noexcept
{
    ColorImage result_image;
    assert(raw_buffer);
    if (raw_buffer == nullptr || buffer_size == 0)
    {
        if (err_handler) { err_handler(stdutils::io::Severity::ERR, "Null RAW stream"); }
        return result_image;
    }
    constexpr std::size_t HEADER_SZ = sizeof(RawHeader);
    if (buffer_size < HEADER_SZ)  {
        if (err_handler) { err_handler(stdutils::io::Severity::ERR, "The RAW header is incomplete"); }
        return result_image;
    }
    if (raw_name == nullptr) { raw_name = "noname"; }

    // Read the header
    RawHeader raw_header;
    IGNORE_RETURN stdutils::memcpy<RawHeader>(&raw_header, raw_buffer);

    // Read the pixel array
    const std::size_t pixel_array_sz = std::size_t{4} * raw_header.width * raw_header.height;
    const std::byte* const raw_pixels = raw_buffer + HEADER_SZ;
    if (buffer_size < HEADER_SZ + pixel_array_sz)  {
        if (err_handler) { err_handler(stdutils::io::Severity::ERR, "The RAW pixel array is incomplete"); }
        return result_image;
    }
    result_image = ColorImage(raw_header.width, raw_header.height);
    assert(result_image.row_stride() == result_image.width());
    IGNORE_RETURN stdutils::memcpy<ColorPixel>(result_image.raw_data(), result_image.raw_size(), raw_pixels, pixel_array_sz);

    return result_image;
}

} // namespace io
} // namespace bitmap
