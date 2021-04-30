/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PRIVATE API of the Picross solver
 *
 * Copyright (c) 2010-2021 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <picross/picross.h>

#include <exception>


namespace picross
{

class PicrossLineAdditionError : public std::runtime_error
{
public:
    PicrossLineAdditionError() : std::runtime_error("[PicrossSolver] An error was detected adding two lines")
    {}
};

class PicrossLineDeltaError : public std::runtime_error
{
public:
    PicrossLineDeltaError() : std::runtime_error("[PicrossSolver] An error was detected computing the delta of two lines")
    {}
};


/*
 * Tile namespace.
 *
 *   A tile is the base element to construct lines and grids: it can be empty (ZERO) of filled (ONE).
 *   The following namespace defines the constants, types and functions used for the manipulation of tiles.
 */
namespace Tile
{
    char str(Type t);
    Type add(Type t1, Type t2);
    Type compatible(Type t1, Type t2);
    Type delta(Type t1, Type t2);
    Type reduce(Type t1, Type t2);
}


/*
 * Line related functions
 */
bool is_all_one_color(const Line& line, Tile::Type color);
bool is_fully_defined(const Line& line);
std::string str_line(const Line& line);
std::string str_line_type(Line::Type type);
Line line_delta(const Line& line1, const Line& line2);

} // namespace picross
