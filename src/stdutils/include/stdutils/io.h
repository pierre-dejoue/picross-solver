// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <algorithm>
#include <exception>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <istream>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace stdutils {
namespace io {

/**
 * IO error handling
 *
 * Severity code:
 *   Negative: Non-recoverable, output should be ignored.
 *   Positive: Output is usable despite the errors.
 *
 * The user is free to expand the list of severity codes proposed below
 */
using SeverityCode = int;
struct Severity
{
    static constexpr SeverityCode FATAL = -2;
    static constexpr SeverityCode EXCPT = -1;
    static constexpr SeverityCode ERR   =  1;
    static constexpr SeverityCode WARN  =  2;
    static constexpr SeverityCode INFO  =  3;
    static constexpr SeverityCode TRACE =  4;
};

std::string_view str_severity_code(SeverityCode code) noexcept;

using ErrorMessage = std::string_view;

using ErrorHandler = std::function<void(SeverityCode, ErrorMessage)>;

/**
 * Error code + message
 *
 * The user is free to expand the list of error codes proposed below
 */
using ErrorCode = int;
struct Error
{
    static constexpr ErrorCode None = 0;
    static constexpr ErrorCode Unspecified = 1;
};

class WhatError
{
public:
    WhatError()
        : m_err_msg()
        , m_err_code{Error::None}
    { }

    WhatError(std::string err_msg, ErrorCode err_code = Error::Unspecified)
        : m_err_msg(std::move(err_msg))
        , m_err_code(err_code)
    { }

    std::string_view msg()  const noexcept { return m_err_msg; }
    ErrorCode        code() const noexcept { return m_err_code; }

    explicit operator bool() const noexcept { return m_err_code != Error::None; }

    void clear() noexcept { m_err_msg.clear();  m_err_code = Error::None; }

private:
    std::string m_err_msg;
    ErrorCode   m_err_code;
};

/**
 * Floating point IO precision
 *
 * Set the output stream precision so that the round-trip fp -> text -> fp is exact.
 * Return the original precision of the stream.
 */
template <typename F, typename CharT>
int accurate_fp_precision(std::basic_ostream<CharT, std::char_traits<CharT>>& out);

/**
 * Floating point IO precision
 *
 * Set the output stream precision. If passed a negative value, set to the same precision as stdutlls::accurate_fp_precision
 * Return the original precision of the stream.
 */
template <typename F, typename CharT>
int fp_precision(std::basic_ostream<CharT, std::char_traits<CharT>>& out, int precision = -1);

/**
 * RAII class to save/restore stream format for numerical values
 */
template <typename CharT = char>
class SaveNumericFormat
{
public:
    SaveNumericFormat(std::basic_ostream<CharT, std::char_traits<CharT>>& out);
    ~SaveNumericFormat();
private:
    std::basic_ostream<CharT, std::char_traits<CharT>>& m_out;
    std::ios_base::fmtflags m_flags;
    std::streamsize m_precision;
};

/**
 * Pass a file to a parser of std::basic_istream
 */
template <typename Ret, typename CharT>
using StreamParser = std::function<Ret(std::basic_istream<CharT, std::char_traits<CharT>>&, const stdutils::io::ErrorHandler&)>;

template <typename Ret, typename CharT>
Ret open_and_parse_txt_file(const std::filesystem::path& filepath, const StreamParser<Ret, CharT>& stream_parser, const stdutils::io::ErrorHandler& err_handler) noexcept;
template <typename Ret, typename CharT>
Ret open_and_parse_bin_file(const std::filesystem::path& filepath, const StreamParser<Ret, CharT>& stream_parser, const stdutils::io::ErrorHandler& err_handler) noexcept;

/**
 * Save a file with a writer to std::basic_ostream
 */
template <typename Obj, typename CharT>
using StreamWriter = std::function<void(std::basic_ostream<CharT, std::char_traits<CharT>>&, const Obj&, const stdutils::io::ErrorHandler&)>;

template <typename Obj, typename CharT>
void save_txt_file(const std::filesystem::path& filepath, const StreamWriter<Obj, CharT>& stream_writer, const Obj& obj, const stdutils::io::ErrorHandler& err_handler) noexcept;
template <typename Obj, typename CharT>
void save_bin_file(const std::filesystem::path& filepath, const StreamWriter<Obj, CharT>& stream_writer, const Obj& obj, const stdutils::io::ErrorHandler& err_handler) noexcept;

/**
 * LineStream: A wrapper around std::getline to count line nb
 */
template <typename CharT>
class Basic_LineStream
{
public:
    using istream_t = std::basic_istream<CharT, std::char_traits<CharT>>;

    explicit Basic_LineStream(istream_t& source);

    // Return a line read from the input string, and the line number (starting with line 1)
    // When false is returned, out_str is cleared, the line number is the one of the last line successfully read
    // which is also equal to the total number of lines in the input stream
    // Note: When EOF is first encountered, true is returned. false will be returned on the next call.
    bool getline(std::basic_string<CharT>& out_str, std::size_t& line_nb);

    // Line number of the last line read from the input stream
    // After reading the stream entirely this is equal to the total number of lines
    std::size_t line_nb() const noexcept { return m_line_nb; }

    const istream_t& stream() const { return m_stream; }

private:
    istream_t&  m_stream;
    std::size_t m_line_nb;
};

using LineStream = Basic_LineStream<char>;

/**
 * SkipLineStream: A LineStream with a set of conditions to skip lines
 */
template <typename CharT>
class Basic_SkipLineStream
{
public:
    using istream_t = typename Basic_LineStream<CharT>::istream_t;

    explicit Basic_SkipLineStream(istream_t& source);
    explicit Basic_SkipLineStream(const Basic_LineStream<CharT>& linetream);
    ~Basic_SkipLineStream();

    // Skip lines
    Basic_SkipLineStream<CharT>& skip_empty_lines();
    Basic_SkipLineStream<CharT>& skip_blank_lines();
    Basic_SkipLineStream<CharT>& skip_comment_lines(std::string_view comment_token);

    // Read comments on Basic_LineStream::getline()
    bool getline(std::basic_string<CharT>& out_str, std::size_t& line_nb);

    // Read comments on Basic_LineStream::line_nb()
    std::size_t line_nb() const noexcept { return m_linestream.line_nb(); }

    const typename Basic_LineStream<CharT>::istream_t& stream() const { return m_linestream.stream(); }

private:
    bool skip_line(const std::string& line) const;

    Basic_LineStream<CharT>     m_linestream;
    std::vector<std::string>    m_skip_tokens;
    bool                        m_skip_empty_lines;
    bool                        m_skip_blank_lines;
};

using SkipLineStream = Basic_SkipLineStream<char>;

/**
 * countlines()
 *
 * The number of lines is the same whether that the last one ends with '\n' or not.
 */
template <typename CharT>
std::size_t countlines(std::basic_istream<CharT, std::char_traits<CharT>>& istream);


//
//
// Implementation
//
//


template <typename F, typename CharT>
int accurate_fp_precision(std::basic_ostream<CharT, std::char_traits<CharT>>& out)
{
    // The precision max_digits10 (9 for float, 17 for double) ensures the round-trip "fp -> text -> fp" will be exact.
    constexpr int max_fp_digits = std::numeric_limits<F>::max_digits10;
    const int initial_fp_digits = static_cast<int>(out.precision());
    out << std::setprecision(max_fp_digits);
    return initial_fp_digits;
}

template <typename F, typename CharT>
int fp_precision(std::basic_ostream<CharT, std::char_traits<CharT>>& out, int precision)
{
    const int initial_fp_digits = static_cast<int>(out.precision());
    if (precision < 0)
        out << std::setprecision(std::numeric_limits<F>::max_digits10);
    else
        out << std::setprecision(precision);
    return initial_fp_digits;
}

template <typename CharT>
SaveNumericFormat<CharT>::SaveNumericFormat(std::basic_ostream<CharT, std::char_traits<CharT>>& out)
    : m_out(out)
    , m_flags(out.flags())
    , m_precision(out.precision())
{ }

template <typename CharT>
SaveNumericFormat<CharT>::~SaveNumericFormat()
{
    m_out.flags(m_flags);
    m_out.precision(m_precision);
}

namespace details {

template <typename Ret, typename CharT>
Ret open_and_parse_file(const std::filesystem::path& filepath, bool binary, const StreamParser<Ret, CharT>& stream_parser, const stdutils::io::ErrorHandler& err_handler) noexcept
{
    static_assert(std::is_void_v<Ret> || std::is_nothrow_default_constructible_v<Ret>);
    try
    {
        std::ios_base::openmode mode = std::ios_base::in;
        if (binary) { mode |= std::ios_base::binary; }
        std::basic_ifstream<CharT> inputstream(filepath, mode);
        if (inputstream.is_open())
        {
            return stream_parser(inputstream, err_handler);
        }
        else
        {
            std::stringstream oss;
            oss << "Cannot open file: " << filepath;
            err_handler(stdutils::io::Severity::ERR, oss.str());
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream oss;
        oss << "Exception in open_and_parse_file(" << filepath << ", " << (binary ? "BIN" : "TXT") << "): " << e.what();
        err_handler(stdutils::io::Severity::EXCPT, oss.str());
    }
    return Ret();
}

template <typename Obj, typename CharT>
void save_file(const std::filesystem::path& filepath, bool binary, const StreamWriter<Obj, CharT>& stream_writer, const Obj& obj, const stdutils::io::ErrorHandler& err_handler) noexcept
{
    try
    {
        std::ios_base::openmode mode = std::ios_base::out;
        if (binary) { mode |= std::ios_base::binary; }
        std::basic_ofstream<CharT> outputstream(filepath, mode);
        if (outputstream.is_open())
        {
            stream_writer(outputstream, obj, err_handler);
        }
        else
        {
            std::stringstream oss;
            oss << "Cannot open file: " << filepath;
            err_handler(stdutils::io::Severity::ERR, oss.str());
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream oss;
        oss << "Exception in save_file(" << filepath << ", " << (binary ? "BIN" : "TXT") << "): " << e.what();
        err_handler(stdutils::io::Severity::EXCPT, oss.str());
    }
}

} // namespace details

template <typename Ret, typename CharT>
Ret open_and_parse_txt_file(const std::filesystem::path& filepath, const StreamParser<Ret, CharT>& stream_parser, const stdutils::io::ErrorHandler& err_handler) noexcept
{
    const bool binary_file = false;
    return details::open_and_parse_file(filepath, binary_file, stream_parser, err_handler);
}

template <typename Ret, typename CharT>
Ret open_and_parse_bin_file(const std::filesystem::path& filepath, const StreamParser<Ret, CharT>& stream_parser, const stdutils::io::ErrorHandler& err_handler) noexcept
{
    const bool binary_file = true;
    return details::open_and_parse_file(filepath, binary_file, stream_parser, err_handler);
}

template <typename Obj, typename CharT>
void save_txt_file(const std::filesystem::path& filepath, const StreamWriter<Obj, CharT>& stream_writer, const Obj& obj, const stdutils::io::ErrorHandler& err_handler) noexcept
{
    const bool binary_file = false;
    details::save_file(filepath, binary_file, stream_writer, obj, err_handler);
}

template <typename Obj, typename CharT>
void save_bin_file(const std::filesystem::path& filepath, const StreamWriter<Obj, CharT>& stream_writer, const Obj& obj, const stdutils::io::ErrorHandler& err_handler) noexcept
{
    const bool binary_file = true;
    details::save_file(filepath, binary_file, stream_writer, obj, err_handler);
}

template <typename CharT>
Basic_LineStream<CharT>::Basic_LineStream(istream_t& source)
    : m_stream(source)
    , m_line_nb(0)
{
}

template <typename CharT>
bool Basic_LineStream<CharT>::getline(std::basic_string<CharT>& out_str, std::size_t& line_nb)
{
    out_str.clear();
    const bool no_fail = !m_stream.fail() && static_cast<bool>(std::getline(m_stream, out_str));
    if (no_fail) { m_line_nb++; }
    line_nb = m_line_nb;
    return no_fail;
}


template <typename CharT>
Basic_SkipLineStream<CharT>::Basic_SkipLineStream(typename Basic_LineStream<CharT>::istream_t& source)
    : m_linestream(source)
    , m_skip_tokens()
    , m_skip_empty_lines(false)
    , m_skip_blank_lines(false)
{}

template <typename CharT>
Basic_SkipLineStream<CharT>::Basic_SkipLineStream(const Basic_LineStream<CharT>& linestream)
    : m_linestream(linestream)
    , m_skip_tokens()
    , m_skip_empty_lines(false)
    , m_skip_blank_lines(false)
{}

template <typename CharT>
Basic_SkipLineStream<CharT>::~Basic_SkipLineStream()
{
    // TODO: Remove me. This is to appease the gcc compiler regarding the automatic destruction of the m_skip_tokens vector.
    // /usr/include/c++/13/bits/basic_string.h:223:28: error: potential null pointer dereference [-Werror=null-dereference]
    // |       { return _M_dataplus._M_p; }
    // |                            ^~~~
    m_skip_tokens.clear();
}

template <typename CharT>
Basic_SkipLineStream<CharT>& Basic_SkipLineStream<CharT>::skip_empty_lines()
{
    m_skip_empty_lines = true;
    return *this;
}

template <typename CharT>
Basic_SkipLineStream<CharT>& Basic_SkipLineStream<CharT>::skip_blank_lines()
{
    m_skip_empty_lines = true;
    m_skip_blank_lines = true;
    return *this;
}

template <typename CharT>
Basic_SkipLineStream<CharT>& Basic_SkipLineStream<CharT>::skip_comment_lines(std::string_view comment_token)
{
    m_skip_tokens.emplace_back(comment_token);
    return *this;
}

template <typename CharT>
bool Basic_SkipLineStream<CharT>::skip_line(const std::string& line) const
{
    if (m_skip_empty_lines && line.empty())
    {
        return true;
    }
    if (m_skip_blank_lines || !m_skip_tokens.empty())
    {
        std::istringstream iss(line);
        std::string token;
        iss >> token;               // leading whitespaces are skipped
        if (m_skip_blank_lines && token.empty())
        {
            return true;
        }
        return std::any_of(m_skip_tokens.cbegin(), m_skip_tokens.cend(), [&token](const auto& comment_token) {
            return token.substr(0, comment_token.size()) == comment_token;
        });
    }
    return false;
}

template <typename CharT>
bool Basic_SkipLineStream<CharT>::getline(std::basic_string<CharT>& out_str, std::size_t& line_nb)
{
    bool no_fail = false;
    bool skip = false;
    do
    {
        no_fail = m_linestream.getline(out_str, line_nb);
        skip = no_fail && skip_line(out_str);
    } while (skip);
    return no_fail;
}

template <typename CharT>
std::size_t countlines(std::basic_istream<CharT, std::char_traits<CharT>>& istream)
{
    LineStream line_stream(istream);
    std::string line;
    std::size_t line_nb{0u};
    while (line_stream.getline(line, line_nb));
    return line_nb;
}

} // namespace io
} // namespace stdutils
