/*  Boolean Propagation Solver

    Copyright Rick van der Meiden 2013 

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

#include "bps.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

// logging
#ifdef LOG_PROPAGATIONS
    #include <stdio.h>
    char* log_file_name = "bps_prop.log";
    FILE* log_file_handle = NULL;
#endif


// ----- naming conventions --- 
// types start with a capital and camelCase; MyType
// functions also start with a capital and camelCase: MyFunction
// global variables start with 'g' and camelCase; gMyVariable
// local variables are lowercase : myvariable 
// defines and macros are uppercase: MYCONST

// ------ set of integers ----
// A set of integers, up to a given number (0 <= value < capacity).
// Implemented as a double ended queue and a map from values to nodes.
// Fast, but not memory efficient. Space is allocated for full capacity. 
// membeship test cost O(1)
// next/prev value in set (ordered by append/insert time) cost O(1)
// number of values in set cost O(1)
// add/remove cost O(1)


Node* NewNode (int value) {
    //DEBUGPRINTF ("New Literal %d\n",value);
    Node* l = malloc(sizeof(Node));
    l->value = value;
    l->next = NULL;
    l->prev = NULL;
    return l;
}

Set* NewSet (int capacity) 
{
    Set* newset = (Set*)malloc(sizeof(Set));
    newset->capacity = capacity;
    newset->count = 0;
    newset->first = NULL;
    newset->last = NULL;
    newset->value2node = (Node**)malloc(newset->capacity*sizeof(Node*));
    int value;
    for (value=0;value<newset->capacity;value++) {
        newset->value2node[value] = (Node*)NULL;
    }
    return newset;
}

void DeleteSet (Set* set) 
{
    if (set != NULL) {
        int value;
        for (value=0;value<set->capacity;value++) {
            Node* node = set->value2node[value];
            if (node != NULL) free(node);
        }
        if (set->value2node != NULL) free(set->value2node);
        free(set);
    }
}



Node* GetSetNode (Set* set, int value) 
{
    if (set != NULL) {
        if (value >=0 && value < set->capacity) {
            return set->value2node[value];        
        }  
    }
    return NULL;
}


Node* AddSet (Set* set, int value, int front) 
{
    Node* node = NULL;
    if (set != NULL) {
        Node* node = GetSetNode(set, value);
        if (node == NULL) {
            if (value >= 0 && value < set->capacity) {
                // create new node
                node = NewNode(value);
                // store in map
                set->value2node[value] = node;
                if (front) {
                    // prepend to list
                    //DEBUGPRINTF("prepend %d\n", value);
                    if (set->first != NULL)
                        set->first->prev = node;
                    node->next = set->first;
                    set->first = node;
                    if (set->last == NULL)
                        set->last = node;
                } else {
                    // append to list
                    //DEBUGPRINTF("append %d\n", value);
                    if (set->last != NULL) {
                        set->last->next = node;
                    }
                    node->prev = set->last;
                    set->last = node;
                    if (set->first == NULL)
                        set->first = node;
                }
                // update count
                set->count++;
                //DEBUGPRINTF("count=%d\n", set->count);
            }
        }
    }
    return node;
}

Node* RemSet(Set* set, int value) {

    if (set != NULL) {
        Node* node = GetSetNode(set, value);
        if (node != NULL) {
            //DEBUGPRINTF("remove %d\n", value);
            set->value2node[value] = NULL;
            if (node->prev != NULL) ((Node*)node->prev)->next = node->next;    
            if (node->next != NULL) ((Node*)node->next)->prev = node->prev;    
            if (node == set->first) set->first = node->next;
            if (node == set->last) set->last = node->prev;
            free(node);
            set->count--;
            //DEBUGPRINTF("count=%d\n", set->count);
        }    
   }
   return NULL; 
}

// ------ list of integers ----
// A list of integers.
// Implemented as a double ended queue.
// append/insert cost O(1)
// next/previous value cost O(1)
// number of values in list cost O(1)

List* NewList () 
{
    List* newlist = (List*)malloc(sizeof(Set));
    newlist->count = 0;
    newlist->first = NULL;
    newlist->last = NULL;
    return newlist;
}

void DeleteList (List* list) 
{
    if (list != NULL) {
            
        Node* node = list->first;
        Node* next;
        while(node) {
            next = node->next;    
            if (node != NULL) free(node);   
            node = next;
        }
        free(list);
    }
}

Node* ListPrepend (List* list, int value) 
{
    if (list != NULL) {
        // create new node
        Node* node = NewNode(value);
        // prepend to list
        //DEBUGPRINTF("prepend %d\n", value);
        if (list->first != NULL)
            list->first->prev = node;
        node->next = list->first;
        list->first = node;
        if (list->last == NULL)
            list->last = node;
        // update count
        list->count++;
        return node; 
    } else
        return NULL;
}

Node* ListAppend (List* list, int value) 
{
    if (list != NULL) {
        // create new node
        Node* node = NewNode(value);
        // append to list
        //DEBUGPRINTF("append %d\n", value);
        if (list->last != NULL) {
            list->last->next = node;
        }
        node->prev = list->last;
        list->last = node;
        if (list->first == NULL)
            list->first = node;
        // update count
        list->count++;
        return node; 
    } else
        return NULL;
}

int ListPopEnd (List* list) 
{
    if (list != NULL && list->last != NULL) {
        int value = list->last->value;
        Node* prev = (Node*)list->last->prev;
        if (prev != NULL) prev->next = NULL;
        if (list->last == list->first) list->first = NULL;
        free(list->last);
        list->last = prev;
        list->count--;
        return value;
    }
    return 0;
}

int ListPopFront (List* list) 
{
    if (list != NULL && list->first != NULL) {
        int value = list->first->value;
        Node* next = (Node*)list->first->next;
        if (next != NULL) next->prev = NULL;
        if (list->first == list->last) list->last = NULL;
        free(list->first);
        list->first = next;
        list->count--;
        return value;
    }
    return 0;
}

// ------------------------ algorithmics -------------
// Adds a variable (input may also be a negative literal)
// updates s->gNumberOfVariables to the max abs value
void AddVariable (Solver* s, int value) {
    if (value > 0 && value > s->gNumberOfVariables)
        s->gNumberOfVariables = value;
    else if (value < 0 && -value > s->gNumberOfVariables)
        s->gNumberOfVariables = -value;
}


// Literal2Index computes index in literal array from positive or negative literal value
// note that value 0 (not a literal) maps to -1
// -1 maps to 0
// +1 maps to 1
// -2 maps to 2
// +2 maps to 3
// etc
int Literal2Index(int value) {
    if (value >= 0) 
        return value*2-1;
    else
        return (-value)*2-2;
}

int Index2Literal(int index) {
    if (index % 2 == 0)
        return -(index/2+1);
    else
        return (index/2+1);
}

int OppositeIndex(int index) {
    if (index % 2 == 0)
        return index+1;
    else 
        return index-1;
}

/* assign color to given literal and propagate to other literals by following rules 
   the color is only assigned to free literals (value 0).
   If updateFreeVars is TRUE then the global free variables are updated.
   Returns zero if there is a conflict, i.e. opposite literal is in the solution. 
   Returns nonzero if ok
*/
int Propagate(Solver* s, int lv, int color, int updateFreeVars) {
    int li,ci,c,ki,k;  
    li = Literal2Index(lv);
    if (s->gMarkers[li]!=0) return 1;   // already has a non-zero color
    int oi = Literal2Index(-lv);
    if (s->gMarkers[oi]!=0) { 
            // DEBUGPRINTF("conflict progagating literal %d (because of literal %d)\n",lv,-lv);
            LOG("conflict\n")
            return 0;   // conflict
    }
    // make it so    
    s->gMarkers[li]=color;
    LOG("prop %d %d\n", lv,color)
    // statistics
    s->gNumProp++;      
    //DEBUGPRINTF("propagated literal %d=%d\n",lv,s->gMarkers[li]);
    // remove from free literals
    int var = abs(lv);
    if (updateFreeVars) RemSet(s->gFreeVars, var);
    //DEBUGPRINTF("s->gFreeVars removed %d\n", var);
    //DEBUGPRINTF("s->gFreeVars.count=%d\n",s->gFreeVars->count);
    // first decrement all counters 
    // because UnPropagate does not stop in for loop on concflict
    for (ci=0;ci<s->gL2N[li];ci++) {
        c = s->gL2C[li][ci];
        s->gCounters[c]-=1;
        s->gTotalCount-=1;
        ASSERT(s->gCounters[c]>=0);
    } 
    // fire rules
    for (ci=0;ci<s->gL2N[li];ci++) {
        c = s->gL2C[li][ci];
        if (s->gCounters[c]==0) {
            //DEBUGPRINTF("fire rule %d\n",c);
            for (ki=0;ki<s->gC2N[c];ki++) {
                k = s->gC2L[c][ki];
                // recurse and escape if failed
                if (!Propagate(s, k, color, updateFreeVars)) return 0;
            }
        }
    }
    return 1;   // succes!
}   

// undo a previously failed propagation
// assigns zero to literal if has the given color and propagates via rules
void Unpropagate(Solver* s, int lv, int color, int updateFreeVars) {
    int li,ci,c,ki,k;
    li = Literal2Index(lv);
    if (s->gMarkers[li]!=color) return;   // doesn't have given color, done.
    // make it so
    //DEBUGPRINTF("unpropagated literal %d=%d\n",lv,s->gMarkers[li]);
    s->gMarkers[li]=0;
    LOG("unprop %d %d\n", lv,0)
    // add to free variables if both literals of same var are marked 0
    if (updateFreeVars && s->gMarkers[Literal2Index(-lv)] == 0) {
        AddSet(s->gFreeVars, abs(lv), FALSE);
        //DEBUGPRINTF("s->gFreeVars added %d\n", abs(lv));
        //DEBUGPRINTF("s->gFreeVars.count=%d\n",s->gFreeVars->count);
    }
    // un-propagate rules
    for (ci=0;ci<s->gL2N[li];ci++) {
        c = s->gL2C[li][ci];
        // note: only unpropagate if counter = 0; no wait for increment
        s->gCounters[c]+=1;
        s->gTotalCount+=1;
        if (s->gCounters[c]==1) {
            //DEBUGPRINTF("unfire rule %d\n",c);
            for (ki=0;ki<s->gC2N[c];ki++) {
                k = s->gC2L[c][ki];
                Unpropagate(s, k, color, updateFreeVars);
            }
        }
    }
} 

void init(Solver* s, int *problem) {

    // open log file
    #ifdef LOG_PROPAGATIONS 
           
        if (log_file_handle != NULL) {
            fclose(log_file_handle);
        }
        log_file_handle=fopen(log_file_name,"aw");
    
    #endif

    LOG("init\n")

    // ------------- parse problem -------------
    s->gNumberOfVariables = 0;    
    s->gNumberOfRules = 0;      

    /* First count number of variables and rules; mark start-indices in problem of all rules */
    int parsepos = 0;
    int value = 0;
    while (1) {
        // read lhs
        int lhscount = 0;
        while ((value = problem[parsepos++])!=0) {
            lhscount++;
            AddVariable(s, value);
        }
        // read rhs
        int rhscount = 0;
        while ((value = problem[parsepos++])!=0) {
            rhscount++;
            AddVariable(s, value);
        }
        // add rule or stop on empty rule
        if (lhscount == 0 && rhscount == 0)
            break;
        else 
            ++s->gNumberOfRules;
    } 
    // next iterations don't need to parse further than this
    int stoppos = parsepos;

    DEBUGPRINTF("gNumberOfRules=%d\n",s->gNumberOfRules);
    LOG("vars %d\n",s->gNumberOfVariables);     
    LOG("rules %d\n",s->gNumberOfRules);     

    /* 
    s->gFirstRule = NULL;       
    int value;
    int state = 0;  // 0=new rule, new lhs, 1=add lhs, 2=new rhs, 3=add to rhs
    Rule* prevrule = NULL;
    Rule* newrule = NULL;
    Node* newlit = NULL;
    Node* prevlit = NULL;
    
    int parsepos = 0;
    while (1) {
        value = problem[parsepos];
        parsepos++;
        // DEBUGPRINTF("Input value = %d   State = %d\n", value, state); 
        if (state == 0) {
                // new rule, new lhs
                prevrule = newrule;
                newrule = NewRule();
                //AddRule(s, newrule);
        }
        if (value != 0) 
        {
            // create new literal
            prevlit = newlit;
            newlit = NewNode(value);
            AddVariable(s, value);
            // start new lhs/rhs 
            if (state == 0) { 
                // DEBUGPRINTF("New lhs\n");
                newrule->lhs = newlit;
                state = 1;
                prevlit = NULL;
            }
            else if (state == 2) {
                // DEBUGPRINTF("New rhs\n");
                newrule->rhs = newlit;
                state = 3;
                prevlit = NULL;
            }
            if (prevlit != NULL)
                prevlit->next = newlit;
        }
        else // (value == 0) 
        {
            switch (state) {
                case 0: state = 2; break;
                case 1: state = 2; break;
                case 2: state = 0; break;
                case 3: state = 0; break;
            }
            // process new rule
            if (state == 0) {
                if (newrule != NULL) {
                    // check if last rule is empty  -> end input
                    if (newrule->lhs==NULL && newrule->rhs==NULL) {
                        break;
                    }
                    // add rule (if not empty lhs or empty rhs
                    else if (newrule->lhs!=NULL && newrule->rhs!=NULL) {
                        AddRule(s, newrule);
                    }
                }
            }
        }  
    } // while True
    */

    /* ------------ allocate datastructures for propagation--------- */
    int numlits = 2*s->gNumberOfVariables;

    // s->gMarkers: is a array of booleans representing literals that are in the solution
    // literals +x are mapped to 2x-1 and -x are mapped to -2x-2
    s->gMarkers = (int*)malloc(numlits*sizeof(int));
    
    // s->gCounters: counts for each rule the number of literals needed to fire the rule
    // note that a rule can have at most 255 input literals or the counter will overflow
    s->gCounters = (int*)malloc(s->gNumberOfRules*sizeof(int));

    // a map from literals to a array of counters
    // when a literal is added to the solution, the counters are decremented 
    s->gL2C = (int**)malloc(numlits*sizeof(int*));
    // the number of counters per input literal
    s->gL2N = (int*)malloc(numlits*sizeof(int));

    // a map from counters to an array of literals
    // when a counter reaches zero, the literals are added to the solution
    s->gC2L = (int**)malloc(s->gNumberOfRules*sizeof(int*));
    // the number of output literals per counter
    s->gC2N = (int*)malloc(s->gNumberOfRules*sizeof(int));

    // init all data to zero 
    int li = 0;     // literal index
    int r = 0;     // rule index
    
    for (li=0;li<numlits;li++) {
        s->gMarkers[li] = 0;
        s->gL2N[li] = 0;
    }    

    for (r=0;r<s->gNumberOfRules;r++) {
        s->gCounters[r] = 0;
        s->gC2N[r] = 0;
    }
    s->gTotalCount=0;

    // ----- convert rules to the above datastructure --------
    // first we only count the size of the arrays in s->gL2C and s->gC2L
    // and init the s->gCounters to the number of inputs per rule
    // for all rules
    
    parsepos = 0; 
    r = 0;    
    while (parsepos < stoppos) {
        // for all lhs
        while ((value = problem[parsepos++])!=0) {
            li = Literal2Index(value);
            s->gCounters[r]++;      // count the number of input literals of rule
            s->gTotalCount++;       // total of all counters; debug assertion
            s->gL2N[li] ++;         // count number of counters per literals 
        }
        // for all rhs     
        while ((value = problem[parsepos++])!=0) {
            s->gC2N[r]++;          // count the number of output literals per rule
        }
        r++;
    }

    // allocate the arrays s->gL2C and s->gC2L
    // clear the s->gL2N and s->gC2N arrays to use them as index to counters
    for (li=0;li<numlits;li++) {
        //DEBUGPRINTF("%d counters from literal %d\n",s->gL2N[li],Index2Literal(li));
        s->gL2C[li] = (int*)malloc(s->gL2N[li]*sizeof(int));
        s->gL2N[li] = 0;
    }
    for (r=0;r<s->gNumberOfRules;r++) {
        //DEBUGPRINTF("%d literals from counter %d\n",s->gC2N[r],r);
        s->gC2L[r] = (int*)malloc(s->gC2N[r]*sizeof(int));
        s->gC2N[r] = 0;
    }

    // now map the rules to s->gL2C and s->gC2L
    
    // for all rules
    parsepos = 0;
    r = 0;
    while (parsepos < stoppos) {
        // for all lhs
        while ((value = problem[parsepos++])!=0) {
            li = Literal2Index(value);
            s->gL2C[li][s->gL2N[li]] = r;    
            s->gL2N[li] ++;          
        }
        // for all rhs     
        while ((value = problem[parsepos++])!=0) {
            s->gC2L[r][s->gC2N[r]] = value; 
            s->gC2N[r]++;     
        }
        r++;
    }
     
    // init set of free variables 
    s->gFreeVars = NewSet(s->gNumberOfVariables+1); 
    int v;   
    for (v=1; v<s->gNumberOfVariables+1;v++) {
        AddSet(s->gFreeVars, v, FALSE);
    }
    
    // clear progagation color and stack
    s->gStack = NULL;
    s->gColor = 1;

    // clear statistics
    s->gNumProp = 0;
    s->gNumChoice = 0;
 
    // no ordered variables
    s->ordered = NewList();

} // init

// ------------- API ----------- 

// continue solving until solution found or no (more) solutions exist 
// returns non-zero if a solution was found
int NextSolution(Solver* s) {
    
    LOG("next\n")

    DEBUGPRINTF("NextSolution; color = %d\n",s->gColor);
  
    int backtrack = 0;
    if (s->gFreeVars->count == 0) backtrack = 1; 
    
    while (1) {

        // backtrack - undo last choice and try alternative if there is one 
        if (backtrack == 1) {
            DEBUGPRINTF("Backtrack, color=%d, freevars=%d\n", s->gColor,s->gFreeVars->count);
            LOG("backtrack\n")
            //  check stack
            if (s->gStack != NULL) { 

                // if the last entry is negative literal, try positive
                if  (s->gStack->value > s->gNumberOfVariables) {
                    // special value indicates a fixed literal on the stack
                    // undo fix 
                    int lit = s->gStack->value - 2*s->gNumberOfVariables;
                    Unpropagate(s, lit, s->gColor, TRUE);
                    s->gColor-=1;
                    DEBUGPRINTF("Unfix: %d color=%d, freevars=%d\n", lit, s->gColor,s->gFreeVars->count);
                    // pop stack 
                    Node* todelete = s->gStack;
                    s->gStack = s->gStack->next; 
                    free(todelete); 
                    // continue backtracking 
                    backtrack = 1;
                }
                else if  (s->gStack->value < 0) {
                    // statistics
                    s->gNumChoice++;
                    // undo previous choice
                    Unpropagate(s, s->gStack->value, s->gColor, TRUE);
                    s->gColor-=1;
                    DEBUGPRINTF("Unchoose: %d color=%d, freevars=%d\n", s->gStack->value, s->gColor,s->gFreeVars->count);
                    // negate stack value (no need to pop and push)
                    s->gStack->value = -s->gStack->value;
                    s->gColor+=1;
                    int success = Propagate(s, s->gStack->value, s->gColor, TRUE);
                    DEBUGPRINTF("Alternative: %d color=%d, freevars=%d\n", s->gStack->value, s->gColor,s->gFreeVars->count);
                    if (success) {
                        //DEBUGPRINTF("s->gFreeVars.count=%d\n",s->gFreeVars->count);
                        if (s->gFreeVars->count == 0) {
                            LOG("solution\n")
                            return 1;           // solution found 
                        }
                        else 
                        {
                            backtrack = 0;      // go forward
                        }
                    }
                    else {
                        printf("Error: choose alternative failed!");
                        exit(1);
                    }
                }
                else 
                {
                    // undo previous choice (alternative)
                    Unpropagate(s, s->gStack->value, s->gColor, TRUE);
                    s->gColor-=1;
                    DEBUGPRINTF("Unalternative: %d color=%d, freevars=%d\n", s->gStack->value, s->gColor,s->gFreeVars->count);
                    
                    // pop stack 
                    Node* todelete = s->gStack;
                    s->gStack = s->gStack->next; 
                    free(todelete); 
         
                    // continue backtracking
                    backtrack = 1;
                }
            }
            else
            {
                // empty stack, search exhausted!
                return 0;
            }
        } // endif backtrack   
        else
        {   // forward pass - try to fix variables and make choice if you have to 
            DEBUGPRINTF("Forwards, color=%d freevars=%d\n", s->gColor,s->gFreeVars->count);

            // propagate all literals for which the antagonist cannot be propagated
            // and backtrack if variables are found with no possible literal propagations
            int lastcount = -1;
            int i = 0;
            while (s->gFreeVars->count != lastcount) { 
                lastcount = s->gFreeVars->count;
                i++;
                DEBUGPRINTF("Iter %d, color=%d freevars=%d\n", i, s->gColor,s->gFreeVars->count);
            
                // unfortunately need to copy because order of s->gFreeVars can change during (Un)Propagate so we cannot iterate over it
                // NOTE: A simply linked list stack or queue would be suffient too!
                // Or maybe we can change the order in which variables are pushed onto s->gFreeVars?

                Set* freevarscopy = NewSet(s->gNumberOfVariables+1);
                Node* freevar = s->gFreeVars->first;
                while (freevar) {
                    AddSet(freevarscopy, freevar->value, FALSE);
                    freevar = freevar -> next;
                }

                // for all freevars in copy (note: freevars may be left at end of while)
                freevar = freevarscopy->first;
                while (freevar) {
                    int var = freevar->value;
                    // check that var is still free
                    if (GetSetNode(s->gFreeVars, var) == NULL) { 
                        DEBUGPRINTF("Skip literal %d color=%d freevars=%d\n",+var, s->gColor, s->gFreeVars->count);
                        ASSERT(s->gMarkers[Literal2Index(var)]!=0 || s->gMarkers[Literal2Index(-var)]!=0);
                        freevar = freevar->next;
                        continue;
                    }
                    ASSERT(s->gMarkers[Literal2Index(var)]==0 && s->gMarkers[Literal2Index(-var)]==0);
                    int count1 = s->gFreeVars->count;
                    int total1 = s->gTotalCount;
                   
                    // try TRUE
                    int pos = Propagate(s, +var, -1, FALSE);   // color -1, does not conflict with Fixes and Choices! 
                    Unpropagate(s, +var, -1, FALSE);
                    // try FALSE
                    int neg = Propagate(s, -var, -1, FALSE);   // color -1, does not conflict with Fixes and Choices! 
                    Unpropagate(s, -var, -1, FALSE);
                    
                    int count2 = s->gFreeVars->count;
                    int total2 = s->gTotalCount;
                    ASSERT(count1==count2);
                    ASSERT(total1==total2);

                    // which combination?
                    if (pos && !neg) { 
                        s->gColor+=1;
                        ASSERT(Propagate(s, +var, s->gColor, TRUE));
                        DEBUGPRINTF("Fix literal %d color=%d freevars=%d\n",+var, s->gColor, s->gFreeVars->count);
                        // push special value on stack
                        Node* newstack = NewNode(var + 2*s->gNumberOfVariables);
                        newstack->next = s->gStack;  
                        s->gStack = newstack;
                    }
                    else if (neg && !pos) {
                        s->gColor+=1;
                        ASSERT(Propagate(s, -var, s->gColor, TRUE));
                        DEBUGPRINTF("Fix literal %d color=%d freevars=%d\n",-var, s->gColor, s->gFreeVars->count);
                        // push special value on stack
                        Node* newstack = NewNode(-var + 2*s->gNumberOfVariables);
                        newstack->next = s->gStack;  
                        s->gStack = newstack;
                    }
                    else if (!neg && !pos) 
                    {    
                        DEBUGPRINTF("No options for variable %d\n",var);
                        // conflict! Backtrack
                        // return Backtrack();
                        backtrack = 1;
                        break;
                    }
                    else {
                        DEBUGPRINTF("Keep free variable %d\n",var);
                    }
                    freevar = freevar->next;
                }   // for
            
                DeleteSet(freevarscopy);
    
                if (backtrack == 1) break;       // a variable has no options; goto backtrack
            }       // while freevars->count != lastcount
            
            if (backtrack == 1) continue;       // a variable has no options; goto backtrack
            

            //DEBUGPRINTF("s->gFreeVars.count=%d\n",s->gFreeVars->count);
            if (s->gFreeVars->count == 0) {
                LOG("solution\n")
                return 1;           // found solution!
            }
            else
            {
                // before making a choice, order free variables (if any order specified)
                Node* ordervar = s->ordered->first;
                while (ordervar) {
                    int var = ordervar->value;
                    if (GetSetNode(s->gFreeVars, var)) {
                        RemSet(s->gFreeVars, var);
                        AddSet(s->gFreeVars, var, TRUE);       // true means add to front!
                        break;      // only the first free var will be used here; no need to continue
                    }
                    ordervar = ordervar->next;
                } 

                // statistics
                s->gNumChoice++;
                // try negative value for any variable with more than one option
                // (positive will be done by next backtrack)
                int var = -s->gFreeVars->first->value;
                // push on stack
                Node* newstack = NewNode(+var);
                newstack->next = s->gStack;  
                s->gStack = newstack;
                // propagate and recurse
                s->gColor ++;
                int success = Propagate(s, +var, s->gColor, TRUE);
                DEBUGPRINTF("Choice %d color=%d freevars=%d\n", var,s->gColor,s->gFreeVars->count);
                LOG("choice %d\n",var)
                if (success) { 
                    //DEBUGPRINTF("s->gFreeVars.count=%d\n",s->gFreeVars->count);
                    if (s->gFreeVars->count == 0) { 
                        LOG("solution\n")
                        return 1;           // found solution! 
                    }
                    else
                        backtrack = 0;      // continue forward search
                } else {
                    printf("Error: choose literal failed!");
                    exit(1); 
                }
            } 
        } // endif backtrack   
    }  // while

    return 0;       // unreachable, for now

}


Solver* NewSolver(int* problem) {
    Solver* s = (Solver*)malloc(sizeof(Solver));
    init(s, problem);
    return s;
}

Solver* NewSolver2(int* problem, List* ordered) {
    Solver* s = (Solver*)malloc(sizeof(Solver));
    init(s, problem);
    // copy variable ordering to solver
    if (ordered) {
        Node* var = ordered->first;
        while (var) {
            ListAppend(s->ordered, var->value);
            var = var->next;
        }
    }
    return s;
}


void DeleteSolver(Solver* s) {
    if (s != NULL) {
        if (s->gMarkers != NULL) free(s->gMarkers);
        if (s->gCounters!=NULL) free(s->gCounters);
        if (s->gL2C != NULL) {
            int l;
            for (l=0;l<2*s->gNumberOfVariables;l++) {
                if (s->gL2C[l] != NULL) free(s->gL2C[l]);
            }
            free(s->gL2C);
        }
        if (s->gL2N != NULL) free(s->gL2N);
        if (s->gC2L != NULL) {
            int r;
            for (r=0;r<s->gNumberOfRules;r++) {
                if (s->gC2L[r] != NULL) free(s->gC2L[r]);
            }
            free(s->gC2L);
        }
        if (s->gC2N != NULL) free(s->gC2N);
        DeleteSet(s->gFreeVars);
        // stack of previous guesses 
        // Node* gStack;
        free(s);
    }
}


int GetNumber (Solver* s, int* literals) {
    int number = 0;
    int lit; 
    int index = 0;
    while ((lit = literals[index++])!=0) {
        // shift right
        number <<= 1;
        // if literal in solution, add 1
        if (s->gMarkers[Literal2Index(lit)]!=0) ++number;
    } 
    return number;
}

