/*******************************************************************************
 * PICROSS SOLVER
 *
 *   Declaration of the PUBLIC API of the Picross solver
 *
 *   - Observer
 *
 * Copyright (c) 2010-2023 Pierre DEJOUE
 ******************************************************************************/
#pragma once

#include <cstdint>
#include <functional>
#include <ostream>
#include <string>


namespace picross
{

//
// The observer is a function object with the following signature:
//
// void observer(Event event, const Line* line, { uint32_t depth, uint32_t misc_i, float misc_f });
//
//  * event = KNOWN_LINE        line = known_line           depth is set        misc_i = nb_alternatives (before)
//  * event = DELTA_LINE        line = delta                depth is set        misc_i = nb_alternatives (after)
//
//      A line of the output grid has been updated, the delta between the previous value of that line
//      and the new one is given in event DELTA_LINE.
//      The depth is set to zero initially, then it is the same value as that of the last BRANCHING event.
//      The number of alternatives before (resp. after) the line solver are given in the event KNOWN_LINE
//      (resp. DELTA_LINE).
//
//  * event = BRANCHING         line = known_line           depth >= 0          misc_i = nb_alternatives    (NODE)
//  * event = BRANCHING         line = nullptr              depth > 0           misc_i = 0                  (EDGE)
//
//      This event occurs when the algorithm is branching between several alternative solutions, or
//      when it is going back to an earlier branch. Upon starting a new branch the depth is increased
//      by one. In the other case, depth is set the the value of the earlier branch.
//      NB: The user must keep track of the state of the grid at each branching point in order to be
//          able to reconstruct the final solutions based on the observer events only.
//      NB: There are two kinds of BRANCHING events: the NODE event which provides the known_tiles of the
//          branching line and the number of alternatives, and the EDGE event each time an alternative
//          of the branching line is being tested.
//
//  * event = SOLVED_GRID       line = nullptr              depth is set        misc_i = 0
//
//      A solution grid has been found. The sum of all the delta lines up until that stage is the solved grid.
//
//  * event = INTERNAL_STATE    line = nullptr              depth is set        misc_i = internal state
//
//      Internal state of the solver given as an integer
//
//  * event = PROGRESS          line = nullptr              depth is set        misc_f = progress_ratio
//
enum class ObserverEvent
{
    KNOWN_LINE,
    DELTA_LINE,
    BRANCHING,
    SOLVED_GRID,
    INTERNAL_STATE,
    PROGRESS
};
struct ObserverData
{
    std::uint32_t   m_depth = 0u;
    std::uint32_t   m_misc_i = 0u;
    float           m_misc_f = 0.f;
};
class Line;
using Observer = std::function<void(ObserverEvent,const Line*,const ObserverData&)>;

std::ostream& operator<<(std::ostream& out, ObserverEvent event);

std::string str_solver_internal_state(std::uint32_t internal_state);

} // namespace picross
