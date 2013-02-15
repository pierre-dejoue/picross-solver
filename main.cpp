/*******************************************************************************
 * PICROSS SOLVER: main.cpp
 *
 *   My implementation of a solver for Picross puzzles (nonograms). This file
 *   parses the input text file with the information for one or several grids
 *   and solve them. If several solutions exist, an exhaustive search is done
 *   and all of them are displayed.
 *
 * Author/Copyright: Pierre DEJOUE - pdejoue.perso.neuf.fr - 2010
 ******************************************************************************/
#include <fstream>
#include "picross_solver.h"

using namespace std;

enum ParsingState {
    FILE_START,
    GRID_START,
    ROW_SECTION,
    COLUMN_SECTION
};
ParsingState parsing_state = FILE_START;
const int INPUT_BUFFER_SZ = 2048;

bool parse_picross_input_file(const string& line_to_parse, vector<GridInput>& grids);

/*******************************************************************************
 * MAIN()
 ******************************************************************************/
int main(int argc, char *argv[])
{
    /***************************************************************************
     * I - Process command line
     **************************************************************************/
    ifstream inputstream;
    char * filename;
    if(argc == 2)
    {
        filename = argv[1];  // input filename
        inputstream.open(filename);
        if(!inputstream.is_open())
        {
            cerr << "Cannot open file " << filename << endl;
            exit(1);
        }
    }
    else
    {
        cerr << "Usage:" << endl;
        cerr << "    picross_parser input_filename" << endl;
        exit(1);
    }

    /***************************************************************************
     * II - Parse input file
     **************************************************************************/

    /* Line buffer */
    char line[INPUT_BUFFER_SZ];
    /* Container for grids input data */
    vector<GridInput> grids_to_solve;
    int line_nb = 0;
    bool is_line_ok;
    GridStats* stats = new GridStats;
    /* Start parsing */
    while(inputstream.good())
    {
        line_nb++;
        inputstream.getline(line, INPUT_BUFFER_SZ - 1);
        if((is_line_ok = parse_picross_input_file(string(line), grids_to_solve)) == false)
        {
            cerr << "Parsing error on line " << line_nb << " (parsing_state = " << parsing_state << "): " << line << endl;
        }
    }
    /* Close file */
    inputstream.close();

    /***************************************************************************
     * III - Solve Picross puzzles.
     **************************************************************************/
    try
    {
        int count_grids = 0;
        for(vector<GridInput>::iterator grid_input = grids_to_solve.begin(); grid_input != grids_to_solve.end(); grid_input++)
        {
            cout << "GRID " << ++count_grids << ": " << grid_input->name << endl;
            /* Container for the solutions */
            Line line();
            vector<Grid> solutions;
            solutions.reserve(16);
            Grid grid(grid_input->columns.size(), grid_input->rows.size(), *grid_input, solutions, stats);

            /* Reset the stats */
            reset_grid_stats(stats);

            /* Sanity check of the input data */
            if(!grid_input->sanity_check())
            {
                cout << " > Invalid grid. Check the input file." << endl << endl;
            }
            else
            {
                /* Solve the grid */
                if(!grid.solve())
                {
                    cout << " > Could not solve that grid :-(" << endl << endl;
                }
                else
                {
                    cout << " > Found " << solutions.size() << " solution(s):" << endl << endl;
                    for(vector<Grid>::iterator solution = solutions.begin(); solution != solutions.end(); solution++) { solution->print(); }
                }

                /* Display stats */
                print_grid_stats(stats);
                cout << endl;
            }

        }
    }
    catch (exception& e) { cout << "ERROR: " << e.what() << endl; }
    catch (...)          { cout << "UNEXPECTED ERROR." << endl; }

    /***************************************************************************
     * IV - Exit
     **************************************************************************/
    delete stats;
    return 0;
}

/*******************************************************************************
 * parse_picross_input_file: parse one line from the input file, based on parsing_state
 *
 *    file format:
 *      GRID <title>        <--- beginning of a new grid
 *      ROWS                <--- marker to start listing the constraints on the rows
 *      [ 1 2 3 ...         <--- constraint on one line (here a row)
 *      ...
 *      COLUMNS             <--- marker for columns
 *      ...
 ******************************************************************************/
bool parse_picross_input_file(const string& line_to_parse, vector<GridInput>& grids)
{
    bool valid_line = false;
    istringstream iss(line_to_parse);
    string token;

    /* Copy the first word in 'token' */
    iss >> token;

    if(token ==  "GRID")
    {
        parsing_state = GRID_START;
        valid_line = true;
        grids.push_back(GridInput());

        grids.back().name = line_to_parse.substr(5);
    }
    else if(token == "ROWS")
    {
        if(parsing_state != FILE_START)
        {
            parsing_state = ROW_SECTION;
            valid_line = true;
        }
    }
    else if(token == "COLUMNS")
    {
        if(parsing_state != FILE_START)
        {
            parsing_state = COLUMN_SECTION;
            valid_line = true;
        }
    }
    else if(token == "[")
    {
        if(parsing_state == ROW_SECTION)
        {
            vector<int> new_row;
            int n;
            while(iss >> n) { new_row.push_back(n); }
            grids.back().rows.push_back(Constraint(Line::ROW, new_row));
            valid_line = true;
        }
        else if(parsing_state == COLUMN_SECTION)
        {
            vector<int> new_column;
            int n;
            while(iss >> n) { new_column.push_back(n); }
            grids.back().columns.push_back(Constraint(Line::COLUMN, new_column));
            valid_line = true;
        }
    }
    else if(token == "")
    {
        // Empty line
        valid_line = true;
    }

    return valid_line;
}

