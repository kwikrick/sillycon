/* API for Boolean Propagation Solver 

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    ----

    Usage:

    A solver is created by NewSolver for a given problem. The solver
    incrementally finds solutions by calling NextSolution.  Note that
    NextSolution must be called at least once, to determine wheter there is a
    solution at all.  The current solution (if there is one) can be queried
    using GetNumber.  Note that if there is no solution, it returns a
    meaningless number)

    ----
*/

#ifndef INCLUDE_BPS
#define INCLUDE_BPS

// if LOG_PROPAGATIONS is defined, then all propagations are written to a log file

//#define LOG_PROPAGATIONS

#ifdef LOG_PROPAGATIONS
    #define LOG(...) if (log_file_handle != NULL) fprintf(log_file_handle,__VA_ARGS__);
#else
    #define  LOG(...)
#endif
 
// the details of these types or not interesting to most API users
#include "bps_types.h"

/* Constructor. 
   Allocates memory and initialises the solver for a given problem.
   The problem array defines a sequence of rules as follows:
       a rule is a lhs (left-hand-side) followed by a rhs
       a lhs is a sequence of literals, followed by a zero
       a rhs is a sequence of literals, followed by a zero
       a positive integers x represents literal (x=True) 
       a negative integer -x represents literal (x=False)
       0 represents end of lhs or rhs
       empty rule ( 0 0 ) defines end of input
    The problem array can be de-allocated after creating the solver.  
    Note that the problem cannot be modified; a new solver must be 
    instantiated. 
*/
Solver* NewSolver(int* problem);

/* Like NewSolver, but additionally some variables may be ordered, 
   so that solutions with lower values (most significant bit first)
   are found first */
Solver* NewSolver2(int* problem, List* ordered);

/* Destructor. Frees allocated memory. */
void DeleteSolver(Solver* s);

/* Searches for the next solution.
   Note that this function must be called at least once, to determine the
    first solution, or to determine that there are no solutions. 
   Returns 1 if a new solution is found; 0 if there are no more solutions
*/
int NextSolution(Solver* s);

/* Get a number from the solution.
   A number is defined by a zero-terminated list of literals, most significant
   bit first.  For a single literal the function returns 1 if it is in the
   solution, or 0 if it is not in the solution. 
*/
int GetNumber(Solver* s, int* literals);

#endif
