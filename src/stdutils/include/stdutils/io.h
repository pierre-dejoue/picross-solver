// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#pragma once

#include <algorithm>
#include <exception>
#include <filesystem>
#include <functional>
#include <istream>
#include <fstream>
#include <sstream>
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
 * The user is free to expand or override the list of severity codes proposed below
 */
using SeverityCode = int;
struct Severity
{
    static constexpr SeverityCode FATAL = -2;
    static constexpr SeverityCode EXCPT = -1;
    static constexpr SeverityCode ERR   = 1;
    static constexpr SeverityCode WARN  = 2;
};

std::string_view str_severity_code(SeverityCode code);

using ErrorMessage = std::string_view;

using ErrorHandler = std::function<void(SeverityCode, ErrorMessage)>;

/**
 * Pass a file to a parser of std::basic_istream
 */
template <typename Ret, typename CharT>
using StreamParser = std::function<Ret(std::basic_istream<CharT, std::char_traits<CharT>>&, const stdutils::io::ErrorHandler&)>;

template <typename Ret, typename CharT>
Ret open_and_parse_file(const std::filesystem::path& filepath, const StreamParser<Ret, CharT>& stream_parser, const stdutils::io::ErrorHandler& err_handler) noexcept;

/**
 * LineStream: A wrapper around std::getline to count line nb
 */
template <typename CharT>
class Basic_LineStream
{
public:
    using stream_type = std::basic_istream<CharT, std::char_traits<CharT>>;

    Basic_LineStream(stream_type& source);

    const stream_type& stream() const { return m_stream; }
    std::size_t line_nb() const { return m_line_nb; }

    // Return the last value of static_cast<bool>(std::getline(...))
    // When false is returned, neither the content of out_str nor the line_nb() should be trusted.
    // Note: When EOF is first encountered, true is returned. false will be returned on the next call.
    bool getline(std::basic_string<CharT>& out_str);

private:
    stream_type& m_stream;
    std::size_t  m_line_nb;
};

using LineStream = Basic_LineStream<char>;

/**
 * SkipLineStream: A LineStream with a set of conditions to skip lines
 */
template <typename CharT>
class Basic_SkipLineStream
{
public:
    Basic_SkipLineStream(typename Basic_LineStream<CharT>::stream_type& source);
    Basic_SkipLineStream(const Basic_LineStream<CharT>& linestream);       // Implicit conversion from Basic_LineStream

    const typename Basic_LineStream<CharT>::stream_type& stream() const { return m_linestream.stream(); }
    std::size_t line_nb() const { return m_linestream.line_nb(); }

    // Skip lines
    Basic_SkipLineStream<CharT>& skip_empty_lines();
    Basic_SkipLineStream<CharT>& skip_blank_lines();
    Basic_SkipLineStream<CharT>& skip_comment_lines(std::string_view comment_token);

    // See notes on Basic_LineStream::getline()
    bool getline(std::basic_string<CharT>& out_str);

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
// IMPLEMENTATION
//
//

template <typename Ret, typename CharT>
Ret open_and_parse_file(const std::filesystem::path& filepath, const StreamParser<Ret, CharT>& stream_parser, const stdutils::io::ErrorHandler& err_handler) noexcept
{
    static_assert(std::is_nothrow_default_constructible_v<Ret>);
    try
    {
        std::basic_ifstream<CharT> inputstream(filepath);
        if (inputstream.is_open())
        {
            return stream_parser(inputstream, err_handler);
        }
        else
        {
            std::stringstream oss;
            oss << "Cannot open file " << filepath;
            err_handler(stdutils::io::Severity::FATAL, oss.str());
        }
    }
    catch(const std::exception& e)
    {
        std::stringstream oss;
        oss << "Exception: " << e.what();
        err_handler(stdutils::io::Severity::EXCPT, oss.str());
    }
    return Ret();
}

template <typename CharT>
Basic_LineStream<CharT>::Basic_LineStream(stream_type& source)
    : m_stream(source)
    , m_line_nb(0)
{
}

template <typename CharT>
bool Basic_LineStream<CharT>::getline(std::basic_string<CharT>& out_str)
{
    bool no_fail = !m_stream.fail();
    if (no_fail)
    {
        m_line_nb++;
        no_fail = static_cast<bool>(std::getline(m_stream, out_str));
    }
    return no_fail;
}


template <typename CharT>
Basic_SkipLineStream<CharT>::Basic_SkipLineStream(typename Basic_LineStream<CharT>::stream_type& source)
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
bool Basic_SkipLineStream<CharT>::getline(std::basic_string<CharT>& out_str)
{
    bool no_fail = false;
    bool skip = false;
    do
    {
        no_fail = m_linestream.getline(out_str);
        skip = no_fail && skip_line(out_str);
    } while (skip);
    return no_fail;
}

template <typename CharT>
std::size_t countlines(std::basic_istream<CharT, std::char_traits<CharT>>& istream)
{
    LineStream line_stream(istream);
    std::size_t result = 0;
    std::string line;
    while (line_stream.getline(line)) { result++; }
    return result;
}

} // namespace io
} // namespace stdutils
