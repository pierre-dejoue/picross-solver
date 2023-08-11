#include <catch_amalgamated.hpp>

#include <stdutils/io.h>

#include <string>
#include <sstream>


// Not really a test, just going step by step while the user is reading from a stream with std::getline
TEST_CASE("Basics of std::basic_istream", "[stdutils::io]")
{
    static const char* example_txt =
R"(a
b

c)";

    std::istringstream sstream(example_txt);
    REQUIRE(sstream.good() == true);
    bool no_failure = false;
    std::string line;

    no_failure = static_cast<bool>(std::getline(sstream, line));
    REQUIRE(no_failure == true);
    CHECK(line == "a");
    CHECK(sstream.good() == true);
    CHECK(sstream.eof() == false);
    CHECK(sstream.fail() == false);
    CHECK(sstream.bad() == false);

    no_failure = static_cast<bool>(std::getline(sstream, line));
    REQUIRE(no_failure == true);
    CHECK(line == "b");

    no_failure = static_cast<bool>(std::getline(sstream, line));
    REQUIRE(no_failure == true);
    CHECK(line == "");

    no_failure = static_cast<bool>(std::getline(sstream, line));        // Last line
    REQUIRE(no_failure == true);
    CHECK(line == "c");
    CHECK(sstream.good() == false);
    CHECK(sstream.eof() == true);
    CHECK(sstream.fail() == false);
    CHECK(sstream.bad() == false);

    no_failure = static_cast<bool>(std::getline(sstream, line));        // Past last line
    REQUIRE(no_failure == false);
    CHECK(line == "c");                 // The output line is untampered with
    CHECK(sstream.good() == false);
    CHECK(sstream.eof() == true);
    CHECK(sstream.fail() == true);
    CHECK(sstream.bad() == false);
}

TEST_CASE("LineStream on a one-liner", "[stdutils::io]")
{
    static const char* oneliner = "They threw the Warden of Heaven out of the airlock!";

    std::istringstream sstream(oneliner);
    stdutils::io::LineStream line_stream(sstream);
    REQUIRE(line_stream.stream().good() == true);

    std::string line;
    bool no_failure = line_stream.getline(line);
    REQUIRE(no_failure == true);
    CHECK(line.empty() == false);
    CHECK(line_stream.line_nb() == 1);
    CHECK(line_stream.stream().good() == false);
    CHECK(line_stream.stream().eof() == true);
    CHECK(line_stream.stream().fail() == false);

    no_failure = line_stream.getline(line);        // read past EOF
    REQUIRE(no_failure == false);
    // Once getline returns false, the content of the line string and the line_nb are irrelevant
    CHECK(line_stream.stream().good() == false);
    CHECK(line_stream.stream().eof() == true);
    CHECK(line_stream.stream().fail() == true);

    no_failure = line_stream.getline(line);        // still past EOF
    REQUIRE(no_failure == false);
    CHECK(line_stream.stream().good() == false);
    CHECK(line_stream.stream().eof() == true);
    CHECK(line_stream.stream().fail() == true);
}

TEST_CASE("LineStream on a multiline text", "[stdutils::io]")
{
    // 6 non-empty lines + 1 empty line (the first one)
    static const char* example_txt = R"(
I've seen things you people wouldn't believe.
Attack ships on fire off the shoulder of Orion.
I watched C-beams glitter in the dark near the Tannhauser gate.
All those moments will be lost in time...
like tears in rain...
Time to die.)";

    std::istringstream sstream(example_txt);
    stdutils::io::LineStream line_stream(sstream);
    REQUIRE(line_stream.stream().good() == true);

    // Parse all lines (including the empty ones)
    std::string line;
    std::size_t count_parsed_lines = 0;
    while(line_stream.getline(line))
    {
        count_parsed_lines++;
        CHECK(line.empty() == (count_parsed_lines == 1));
        CHECK(line_stream.line_nb() == count_parsed_lines);
        CHECK(line_stream.stream().fail() == false);        // But, good() might be false and eof() might be true
    }
    CHECK(count_parsed_lines == 7);
    CHECK(line_stream.stream().good() == false);
    CHECK(line_stream.stream().eof() == true);
    CHECK(line_stream.stream().fail() == true);
    CHECK(line_stream.stream().bad() == false);

    // Read past failure
    const bool no_failure = line_stream.getline(line);
    REQUIRE(no_failure == false);
    CHECK(line_stream.stream().good() == false);
    CHECK(line_stream.stream().eof() == true);
    CHECK(line_stream.stream().fail() == true);
    CHECK(line_stream.stream().bad() == false);
}

TEST_CASE("countlines on empty string", "[stdutils::io]")
{
    static const char* test = "";
    std::istringstream istream(test);
    CHECK(stdutils::io::countlines(istream) == 0);
}

TEST_CASE("countlines on one-liner", "[stdutils::io]")
{
    static const char* test = "hello" ;
    std::istringstream istream(test);
    CHECK(stdutils::io::countlines(istream) == 1);
}

TEST_CASE("countlines on multiline no eof", "[stdutils::io]")
{
    static const char* test =
R"(hello

world)";
    std::istringstream istream(test);
    CHECK(stdutils::io::countlines(istream) == 3);
}

TEST_CASE("countlines on multiline with eof", "[stdutils::io]")
{
    static const char* test =
R"(hello

world
)";
    std::istringstream istream(test);
    CHECK(stdutils::io::countlines(istream) == 3);
}

TEST_CASE("SkipLineStream to skip empty lines", "[stdutils::io]")
{
    // 6 non-empty lines
    static const char* example_txt = R"(
I've seen things you people wouldn't believe.
Attack ships on fire off the shoulder of Orion.
I watched C-beams glitter in the dark near the Tannhauser gate.

All those moments will be lost in time...

like tears in rain...

Time to die.


)";

    std::istringstream sstream(example_txt);
    auto line_stream = stdutils::io::SkipLineStream(sstream).skip_empty_lines();
    REQUIRE(line_stream.stream().good() == true);

    // Read all lines, skipping the empty ones
    std::string line;
    std::size_t count_non_empty_lines = 0;
    while(line_stream.getline(line))
    {
        count_non_empty_lines++;
        CHECK(line.empty() == false);
        CHECK(line_stream.line_nb() >= count_non_empty_lines);
    }
    CHECK(count_non_empty_lines == 6);
    CHECK(line_stream.stream().good() == false);
    CHECK(line_stream.stream().eof() == true);
    CHECK(line_stream.stream().fail() == true);
    CHECK(line_stream.stream().bad() == false);

    // Read past failure
    const bool no_failure = line_stream.getline(line);
    REQUIRE(no_failure == false);
    CHECK(line_stream.stream().good() == false);
    CHECK(line_stream.stream().eof() == true);
    CHECK(line_stream.stream().fail() == true);
    CHECK(line_stream.stream().bad() == false);
}

TEST_CASE("SkipLineStream to skip blank lines", "[stdutils::io]")
{
    static const char* example_txt = "a\n\n  \n\t\nb\n\n";

    std::istringstream sstream(example_txt);
    auto line_stream = stdutils::io::SkipLineStream(sstream).skip_blank_lines();

    std::string line;
    bool no_failure = line_stream.getline(line);
    REQUIRE(no_failure == true);
    CHECK(line == "a");
    CHECK(line_stream.line_nb() == 1);

    no_failure = line_stream.getline(line);         // Last non-blank line
    REQUIRE(no_failure == true);
    CHECK(line == "b");
    CHECK(line_stream.line_nb() == 5);

    no_failure = line_stream.getline(line);         // Past the last line
    CHECK(no_failure == false);
    // Once getline returns false, the content of the line string and the line_nb are irrelevant
}

TEST_CASE("SkipLineStream to skip empty lines and comments", "[stdutils::io]")
{

    // 10 lines of code + 5 empty lines + lots of comments
    static const char* python_txt = R"(
#!/usr/bin/env python
# -*- coding: utf-8 -*-
import itertools

# Puzzle. The goal is to draw a path from A to S with some constraints
#
#   A
#  B C
# D E F
#  G H
# I J K
#  L M
# N O P
#  Q R
#   S
#

def dfs(graph, start, exit_condition):
    for path in dfs_recurse(graph, [ start ], exit_condition):
        yield path

#
# Main()
#
def main():
    total_count = 0
    credits_ok_count = 0
    count_by_length = { }

if __name__ == "__main__":
    main()
)";
    const std::size_t expected_lines_of_code = 10;
    const std::size_t expected_empty_lines = 5;

    std::size_t count_lines_of_code = 0;
    std::size_t count_lines_non_empty = 0;
    std::size_t count_lines_of_code_or_empty = 0;
    std::size_t count_lines_total = 0;
    std::string line;

    {
        std::istringstream sstream(python_txt);
        auto line_stream = stdutils::io::SkipLineStream(sstream).skip_empty_lines().skip_comment_lines("#");
        while(line_stream.getline(line))
        {
            count_lines_of_code++;
        }
        CHECK(count_lines_of_code == expected_lines_of_code);
    }
    {
        std::istringstream sstream(python_txt);
        auto line_stream = stdutils::io::SkipLineStream(sstream).skip_empty_lines();
        while(line_stream.getline(line))
        {
            count_lines_non_empty++;
        }
        CHECK(count_lines_non_empty > expected_lines_of_code);
    }
    {
        std::istringstream sstream(python_txt);
        auto line_stream = stdutils::io::SkipLineStream(sstream).skip_comment_lines("#");
        while(line_stream.getline(line))
        {
            count_lines_of_code_or_empty++;
        }
        CHECK(count_lines_of_code_or_empty == expected_lines_of_code + expected_empty_lines);
    }
    {
        std::istringstream sstream(python_txt);
        count_lines_total = stdutils::io::countlines(sstream);
    }
    CHECK(count_lines_non_empty + expected_empty_lines == count_lines_total);
}

TEST_CASE("SkipLineStream to skip comments", "[stdutils::io]")
{

    static const char* commented_txt =
R"(/    a
//   b
///  c
//
/
d
e
  /    a
  //   b
  ///  c
  //
  /)";

    std::string line;
    {
        std::istringstream sstream(commented_txt);
        auto line_stream = stdutils::io::SkipLineStream(sstream).skip_comment_lines("/");
        std::size_t count_lines_without_comment = 0;
        while(line_stream.getline(line))
        {
            count_lines_without_comment++;
        }
        CHECK(count_lines_without_comment == 2);
    }
    {
        std::istringstream sstream(commented_txt);
        auto line_stream = stdutils::io::SkipLineStream(sstream).skip_comment_lines("//");
        std::size_t count_lines_without_comment = 0;
        while(line_stream.getline(line))
        {
            count_lines_without_comment++;
        }
        CHECK(count_lines_without_comment == 6);
    }
    {
        std::istringstream sstream(commented_txt);
        auto line_stream = stdutils::io::SkipLineStream(sstream).skip_comment_lines("///");
        std::size_t count_lines_without_comment = 0;
        while(line_stream.getline(line))
        {
            count_lines_without_comment++;
        }
        CHECK(count_lines_without_comment == 10);
    }
    {
        std::istringstream sstream(commented_txt);
        auto line_stream = stdutils::io::SkipLineStream(sstream).skip_comment_lines("////");
        std::size_t count_lines_without_comment = 0;
        while(line_stream.getline(line))
        {
            count_lines_without_comment++;
        }
        CHECK(count_lines_without_comment == 12);
    }
    {
        std::istringstream sstream(commented_txt);
        CHECK(stdutils::io::countlines(sstream) == 12);
    }
}
