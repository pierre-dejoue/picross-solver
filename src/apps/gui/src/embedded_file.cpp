#include "embedded_file.h"

#include "bitmap_io_raw.h"

#include <cassert>
#include <exception>
#include <sstream>

std::ostream& operator<<(std::ostream& out, const EmbeddedFile::Format& format)
{
    using Format = EmbeddedFile::Format;
    switch(format)
    {
        case Format::RAW:
            out << "RAW";
            break;

        case Format::TTF:
            out << "TTF";
            break;

        default:
            assert(0);
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const EmbeddedFile::Encoding& encoding)
{
    using Encoding = EmbeddedFile::Encoding;
    switch(encoding)
    {
        case Encoding::Bytes:
            out << "Bytes";
            break;

        default:
            assert(0);
            break;
    }
    return out;
}

bitmap::ColorImage parse_embedded_color_image(const EmbeddedFile& embedded_file, const stdutils::io::ErrorHandler& err_handler)
{
    using Data = EmbeddedFile::Data;
    using Format = EmbeddedFile::Format;

    // Decode the file data
    assert(embedded_file.encoding == EmbeddedFile::Encoding::Bytes);
    const Data decoded_file_data = embedded_file.data;

    // Decode the image
    bitmap::ColorImage img;
    switch (embedded_file.format)
    {
        case Format::RAW:
            img = bitmap::io::read_raw(decoded_file_data.data(), decoded_file_data.size(), err_handler, embedded_file.source.data());
            break;

        case Format::TTF:
        {
            std::stringstream out;
            out << "Not an image format (" << embedded_file.format << ")";
            throw std::invalid_argument(out.str());
        }

        default:
            assert(0);
            break;
    }

    return img;
}
