#pragma once

#include <bitmap_image.h>
#include <stdutils/io.h>

#include <cstddef>

namespace bitmap {
namespace io {

ColorImage read_raw(const std::byte* raw_buffer, std::size_t buffer_size, const stdutils::io::ErrorHandler& err_handler, const char* raw_name = nullptr) noexcept;

} // namespace io
} // namespace bitmap
