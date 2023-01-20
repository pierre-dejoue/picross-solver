#include <catch2/catch_test_macros.hpp>
#include <picross/picross.h>
#include <utils/text_io.h>


TEST_CASE("build_output_grid_from", "[text_io]")
{

    picross::OutputGrid note1 = picross::build_output_grid_from(6, 5, R"(
        ...###
        ...#.#
        ...#.#
        .###..
        .###..
    )");

    picross::OutputGrid note2 = picross::build_output_grid_from(6, 5, R"(
    Note
        000111
        000101
        000101
        011100
        011100
    )");

    picross::OutputGrid note3 = picross::build_output_grid_from(R"(
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
