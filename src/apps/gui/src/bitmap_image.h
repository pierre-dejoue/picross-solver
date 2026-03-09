#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <type_traits>
#include <vector>

namespace bitmap {

struct ColorPixel
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

using coord_t           = std::uint32_t;
using coord_2d_t        = std::array<coord_t, 2>;
using signed_coord_t    = std::int32_t;
using signed_coord_2d_t = std::array<signed_coord_t, 2>;

/**
 * A generic bitmap image
 */
template <typename Pixel>
class Image
{
public:
    using pixel_t = Pixel;
    using pixel_container_t = std::vector<pixel_t>;

    Image() noexcept;
    Image(coord_t width, coord_t height, const pixel_t& c = pixel_t());     // Filled image

    // Moveable
    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    // Non-copyable. Use copy() to explicitly copy.
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image copy() const;

    coord_t width() const noexcept { return m_width; }
    coord_t height() const noexcept { return m_height; }

    coord_2d_t size() const noexcept { return coord_2d_t{ m_width, m_height }; }

    bool empty() const noexcept { return m_width == 0 || m_height == 0; }

    coord_t row_stride() const noexcept { return m_width; }             // In number of pixels

    pixel_t* row(coord_t y);
    const pixel_t* row(coord_t y) const;

    Pixel& pixel(coord_t x, coord_t y);
    const Pixel& pixel(coord_t x, coord_t y) const;
    Pixel& pixel(const coord_2d_t& coordinates);
    const Pixel& pixel(const coord_2d_t& coordinates) const;

    // Fill the image with a constant color
    Image& fill(const Pixel& c);

    // Data is organized in row major. The length of a row is given by row_stride, which is equal to the width
    const Pixel* raw_data() const noexcept { return m_pixels.data(); }
    Pixel*       raw_data()       noexcept { return m_pixels.data(); }

    // In number of pixels
    std::size_t  raw_size() const noexcept { return m_pixels.size(); }

    bool operator==(const Image& other) const noexcept;
    bool operator!=(const Image& other) const noexcept;

private:
    coord_t             m_width;
    coord_t             m_height;
    pixel_container_t   m_pixels;
};

using ColorImage = Image<ColorPixel>;


//
//
// Implementation
//
//


template <typename Pixel>
Image<Pixel>::Image() noexcept
    : m_width(0)
    , m_height(0)
    , m_pixels()
{ }

template <typename Pixel>
Image<Pixel>::Image(coord_t width, coord_t height, const Pixel& c)
    : m_width(width)
    , m_height(height)
    , m_pixels(width * height, c)
{ }

template <typename Pixel>
Image<Pixel>::Image(Image&& other) noexcept
    : m_width{other.m_width}
    , m_height{other.m_height}
    , m_pixels(std::move(other.m_pixels))
{
    other.m_width = 0;
    other.m_height = 0;
}

template <typename Pixel>
Image<Pixel>& Image<Pixel>::operator=(Image&& other) noexcept
{
    m_width = other.m_width;
    m_height = other.m_height;
    m_pixels = std::move(other.m_pixels);
    other.m_width = 0;
    other.m_height = 0;
    return *this;
}

template <typename Pixel>
Image<Pixel> Image<Pixel>::copy() const
{
    Image<Pixel> cpy(m_width, m_height);
    cpy.m_pixels = m_pixels;
    return cpy;
}

template <typename Pixel>
Pixel* Image<Pixel>::row(coord_t y)
{
    assert(y < m_height);
    return m_pixels.data() + y * m_width;
}

template <typename Pixel>
const Pixel* Image<Pixel>::row(coord_t y) const
{
    assert(y < m_height);
    return m_pixels.data() + y * m_width;
}

template <typename Pixel>
Pixel& Image<Pixel>::pixel(coord_t x, coord_t y)
{
    assert(x < m_width);
    assert(y < m_height);
    assert(m_pixels.data());
    return m_pixels.data()[m_width * y + x];
}

template <typename Pixel>
const Pixel& Image<Pixel>::pixel(coord_t x, coord_t y) const
{
    assert(x < m_width);
    assert(y < m_height);
    assert(m_pixels.data());
    return m_pixels.data()[m_width * y + x];
}

template <typename Pixel>
Pixel& Image<Pixel>::pixel(const coord_2d_t& coordinates)
{
    return pixel(coordinates[0], coordinates[1]);
}

template <typename Pixel>
const Pixel& Image<Pixel>::pixel(const coord_2d_t& coordinates) const
{
    return pixel(coordinates[0], coordinates[1]);
}

template <typename Pixel>
Image<Pixel>& Image<Pixel>::fill(const Pixel& c)
{
    auto pixels = m_pixels.span();
    std::fill(std::begin(pixels), std::end(pixels), c);
    return *this;
}

template <typename Pixel>
bool Image<Pixel>::operator==(const Image<Pixel>& other) const noexcept
{
    if (width() != other.width())
        return false;
    if (height() != other.height())
        return false;
    for (coord_t y = 0; y < m_height; y++)
    {
        for (coord_t x = 0; x < m_width; x++)
        {
            if (pixel(x, y) != other.pixel(x, y))
                return false;
        }
    }
    return true;
}

template <typename Pixel>
bool Image<Pixel>::operator!=(const Image<Pixel>& other) const noexcept
{
    return !(*this == other);
}

} // namespace bitmap
