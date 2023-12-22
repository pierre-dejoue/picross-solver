// Copyright (c) 2023 Pierre DEJOUE
// This code is distributed under the terms of the MIT License
#include <catch_amalgamated.hpp>

#include <stdutils/string.h>

#include <sstream>
#include <string>

TEST_CASE("strings tolower, toupper and capitalize", "[stdutils::string]")
{
    CHECK(stdutils::string::tolower("All Those MOMENTS")  == "all those moments");
    CHECK(stdutils::string::toupper("will be lost in time") == "WILL BE LOST IN TIME");
    CHECK(stdutils::string::capitalize("like tears in rain.") == "Like tears in rain.");
}

TEST_CASE("Indentation", "[stdutils::string]")
{
    const stdutils::string::Indent indent(4);       // My indentation is 4 spaces
    {
        std::stringstream out;
        out << indent;                              // Output 1 indentation
        CHECK(out.str().size() == 4);
    }
    {
        std::stringstream out;
        out << indent(2);                           // Output 2 indentations
        CHECK(out.str().size() == 8);
    }
    {
        std::stringstream out;
        out << indent(0);                           // Output zero indentation
        CHECK(out.str().empty());
    }
}
