#pragma once

#include "bitmap_image.h"

#include <stdutils/io.h>
#include <stdutils/span.h>

#include <cassert>
#include <cstddef>
#include <ostream>
#include <string_view>

/**
 * Embed binary files inside the application executable
 */
struct EmbeddedFile
{
    enum class Format
    {
        RAW,
        TTF,
    };

    enum class Encoding
    {
        Bytes,
    };

    using Data = stdutils::Span<const std::byte>;

    EmbeddedFile(Format format, Encoding encoding, std::string_view source, std::string_view encoded_str)
        : format(format)
        , encoding(encoding)
        , source(source)
        , data()
    {
        data = Data(reinterpret_cast<const std::byte*>(encoded_str.data()), encoded_str.size());
    }

    EmbeddedFile(Format format, Encoding encoding, std::string_view source, const Data& data)
        : format(format)
        , encoding(encoding)
        , source(source)
        , data(data)
    { }

    EmbeddedFile(Format format, Encoding encoding, std::string_view source, const std::byte* data_ptr, std::size_t data_sz)
        : EmbeddedFile(format, encoding, source, Data(data_ptr, data_sz))
    { }

    std::string_view data_as_string_view() const
    {
        return std::string_view(reinterpret_cast<const char*>(data.data()), data.size());
    }

    Format              format;
    Encoding            encoding;
    std::string_view    source;
    Data                data;
};

std::ostream& operator<<(std::ostream& out, const typename EmbeddedFile::Format& format);
std::ostream& operator<<(std::ostream& out, const typename EmbeddedFile::Encoding& encoding);

bitmap::ColorImage parse_embedded_color_image(const EmbeddedFile& embedded_file, const stdutils::io::ErrorHandler& err_handler);
