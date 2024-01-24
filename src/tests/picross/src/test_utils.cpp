#include <catch_amalgamated.hpp>
#include <picross/picross.h>
#include <utils/text_io.h>

namespace picross {

TEST_CASE("build_output_grid_from", "[text_io]")
{
    OutputGrid note1 = build_output_grid_from(6, 5, R"(
        ...###
        ...#.#
        ...#.#
        .###..
        .###..
    )");

    OutputGrid note2 = build_output_grid_from(6, 5, R"(
    Note
        000111
        000101
        000101
        011100
        011100
    )");

    OutputGrid note3 = build_output_grid_from(R"(
    Note

        ...###
        ...#.#
        ...#.#
        .###..

        .###..


    )");

    CHECK(note3.width() == 6);
    CHECK(note3.height() == 5);
    CHECK(note1 == note2);
    CHECK(note2 == note3);
}

} // namespace picross
