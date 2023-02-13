/*  Types used interally by BPS. 

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
*/

// --------- defines -----

//#define DEBUGPRINTF(...) printf(__VA_ARGS__)
#define DEBUGPRINTF(...)  
#define ASSERT(expr) if (!(expr)) {printf("assertion failed @ line %d\n",__LINE__); exit(1);}
#define TRUE (0==0)
#define FALSE (1==0)

// Node is used in Set and List
typedef struct { 
    int value;     
    void* next;     // (Node*)
    void* prev;     // (Node*)
} Node; 

Node* NewNode (int value);

// ------ set of integers ----
// A set of integers, up to a given number (0 <= value < capacity).
// Implemented as a double ended queue and a map from values to nodes.
// Fast, but not memory efficient. Space is allocated for full capacity. 
// membeship test cost O(1)
// next value in set (ordered by append/insert time) cost O(1)
// number of values in set cost O(1)
// add/remove cost O(1)

typedef struct {
    int capacity;
    int count;
    Node* first;
    Node* last;
    Node** value2node;
} Set;

Node* GetSetNode (Set* set, int value); 
Node* AddSet (Set* set, int value, int front); 
Set* NewSet (int capacity); 
void DeleteSet (Set* set); 
Node* RemSet(Set* set, int value);

// ------ list of integers ----
// A list of integers.
// Implemented as a double ended queue.
// append/insert cost O(1)
// next/previous value cost O(1)
// number of values in list cost O(1)

typedef struct {
    int count;
    Node* first;
    Node* last;
} List;

List* NewList (); 
void DeleteList (List* list);
Node* ListAppend (List* list, int value);
Node* ListPrepend (List* list, int value); 
int ListPopEnd (List* list); 
int ListPopFront (List* list); 


// ------------ solver ---------
// visible in the API, but user need not be concerned with the details

typedef struct {

    // counters for number of rules 
    int gNumberOfRules;

    // the maximum absolute value of all the literals in the rules
    int gNumberOfVariables;

    // gMarkers: is a array representing literals that are in the solution
    // literals +x are mapped index to 2x-1 and -x are mapped to index -2x-2
    // the value stored in the array is called a color (just a name really)
    // A literal marked with color 0 is not in the solution; any other color is in;
    int* gMarkers;
        
    // gCounters: counts for each rule the number of lhs literals needed to fire the rule
    // when a counter reaches zero, the rhs literals are added to the solution
    int* gCounters;
    
    // for debugging/assert algorithm correctness
    int gTotalCount;

    // a map from gMarkers indices to a array of gCounters indices
    // when a literal is added to the solution, all the counters are decremented 
    int** gL2C;

    // the number of counter indices per literal index
    int* gL2N;

    // a map from gCounters indices to an array of gMarkers indices
    int** gC2L;

    // the number of literal indices per counter
    int* gC2N; 

    // set of free variables; i.e. both literals marked with color=0 
    Set* gFreeVars;
  
    // color to use for propagating, corresponds more or less to depth
    int gColor;

    // stack of previous guesses 
    Node* gStack;

    // statistics - total number of propagations (so far)
    int gNumProp;
    
    // statistics - total number of choices (so far)
    int gNumChoice;

    // odered variables
    // the solver will first find the solution with the minimum value
    // for these variables (most significant first)
    List* ordered;
 
} Solver;

// Literal2Index computes index in a literal array from a positive or negative literal value.
// You'll need these if you want to examine the literals in a solution (i.e. Solver.gMarkers) 
// Note that value 0 (not a literal) maps to -1
// -1 maps to 0
// +1 maps to 1
// -2 maps to 2
// +2 maps to 3
// etc
int Literal2Index(int value);

// inverse of Literal2Index
int Index2Literal(int index);

// index of the opposite literal corresponding to the given index; e.g. 2 -> 3 and 3 -> 2.
// equivalent but faster than Literal2Index(-Index2Literal(index))  
int OppositeIndex(int index);


/* You should probably not use Propagate and Unpropagate directly 
   But they are here for use by Simplyify in the sillycon program. 
*/

/* Assign color to given literal and propagate to other literals by following rules. 
   The color is only assigned to free literals (value 0).
   If updateFreeVars is TRUE then the set of free variables is updated.
   Returns zero if there is a conflict, i.e. opposite literal is in the solution. 
   Returns nonzero if ok
*/
int Propagate(Solver* s, int lv, int color, int updateFreeVars);

/* undo a previously failed propagation
   assigns zero to literal if has the given color and propagates via rules
*/
void Unpropagate(Solver* s, int lv, int color, int updateFreeVars);


