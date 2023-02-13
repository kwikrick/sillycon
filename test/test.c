/*  Test program for Boolean Propagation Solver 

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

Usage: run as
  
    testbps

or
  
    testbps <infile> 

When <infile> is given, rules are read from file but number inputs are read
from stdin.  If no <infile> is given,  rules and input are read from stdin.
(Actually, suppossed to read from file first, then stdin, but transition is
buggy)

Input format: 

Input consists of a sequence of decimal integers sepearated by whitespace.
These are interpreted as rules, like this:

i1 i2 i3 ... 0 o1 o2 o3 .... 0

Where i1, i2 etc and o1, o2.. are integers other than zero, which are
interpreted as literals.  A positive literal +x represent x=True and a
negavtive literal -x represents x=False. A solution cannot contain +x and -x,
Literals i1 etc are input literals of the rule and o1 etc are output literals.
When all input literals are in a solution, all output literals will\ also be in
the solution, unless this leads to conflict.

Every rule contains exactly two zeros. No other separation 
(e.g. newlines) between rules is nessecary. 

Special rules:

0 o0 o1 o2 ... oN 0  - input rule; The program will ask for a decimal number on
stdin which will be converted to N-bit twos complement represenation and the
given literals or their opposites will be constrained to be in the solution. 

i1 i2 ... iN 0 0  - output rule; The program will converted the N-bit twos
complement represenation to a decimal number and print it on stdout.

*/

#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include "bps.h"

// local problem data types

typedef struct { 
    int value;     
    void* next;     // (Node*)
    void* prev;     // (Node*)
} TestNode; 

typedef struct {
    TestNode* lhs;
    TestNode* rhs;
    void* next;     // (Rule*)
} TestRule;

// global rule store

TestRule* gFirstTestRule = NULL;
int gNumberOfTestRules = 0;
int gNumberOfVariables = 0;

// Create new empty rule
TestRule* NewTestRule () {
    //DEBUGPRINTF ("New TestRule\n");
    TestRule* rule =  malloc(sizeof(TestRule));
    rule->lhs = NULL;
    rule->rhs = NULL;
    rule->next = NULL;
    return rule;
} 

TestNode* NewTestNode (int value) {
    //DEBUGPRINTF ("New Literal %d\n",value);
    TestNode* l = malloc(sizeof(TestNode));
    l->value = value;
    l->next = NULL;
    l->prev = NULL;
    return l;
}

// TestAdd a rule to the global rule list (gFirstTestRule and gNumberOfTestRules)
void TestAddTestRule (TestRule* rule) {
    rule->next = gFirstTestRule;
    gFirstTestRule = rule;
    gNumberOfTestRules++;
}

// TestAdds a variable (input may also be a negative literal)
// updates gNumberOfVariables to the max abs value
void TestAddVariable (int value) {
    if (value > 0 && value > gNumberOfVariables)
        gNumberOfVariables = value;
    else if (value < 0 && -value > gNumberOfVariables)
        gNumberOfVariables = -value;
}


/* determines a number defined by a list of literals and the current solution 
   numbers are defined most significant bit first
*/
int TestGetNumber (Solver* s, TestNode* node) {
    int number = 0;
    while (node != NULL) {
        // shift right
        number <<= 1;
        // if literal in solution, add 1
        if (s->gMarkers[Literal2Index(node->value)]!=0) number += 1;
        // next
        node = node -> next;
    } 
    return number;
}

/* Determines a list of variables (by adding rules?) from an integer number.   
   Numbers are defined most significant bit first.
   Prepends new rules to gFirstTestRule.
*/

void TestSetNumber (TestNode* node, int number) {
    // find last node
    if (node == NULL) return;
    while (node->next != NULL) {
        ((TestNode*)node->next)->prev = node;    // make it double linked!
        node = node->next;
    }
    // fix variables
    while (node != NULL) {
        // if number is odd, fix variable
        if ((number & 0x1) == 1) {
            TestRule* newrule = NewTestRule();
            newrule->lhs = NewTestNode(-node->value);
            newrule->rhs = NewTestNode(node->value);
            TestAddTestRule(newrule);
        }
        else
        {
            TestRule* newrule = NewTestRule();
            newrule->lhs = NewTestNode(node->value);
            newrule->rhs = NewTestNode(-node->value);
            TestAddTestRule(newrule);
        }
        number >>= 1;
        // next
        node = node->prev;
    }
}


int main(int argc, char** argv) {
   
    // open input; defaults to stdin
    FILE* input = stdin;
    if (argc >=2) {
        char* inputfilename = argv[1];
        input = fopen(inputfilename, "rt");
        if (input == NULL) { 
            printf("failed to open file %s\n",inputfilename);
            return 1;
        }
    }

    // ------- parse input stream ---------
    int value;
    int state = 0;  // 0=new rule, new lhs, 1=add lhs, 2=new rhs, 3=add to rhs
    TestRule* newrule = NULL;
    TestNode* newlit = NULL;
    TestNode* prevlit = NULL;
    while (fscanf(input, "%d", &value) != EOF) {
        // DEBUGPRINTF("Input value = %d   State = %d\n", value, state); 
        if (state == 0) {
                // new rule, new lhs
                newrule = NewTestRule();
                //TestAddTestRule(newrule);
                //newrule->next = gFirstTestRule;
                //gFirstTestRule = newrule;
                //gNumberOfTestRules++;
        }
        if (value != 0) 
        {
            // create new literal
            prevlit = newlit;
            newlit = NewTestNode(value);
            //TestAddVariable(value);
            if (value > gNumberOfVariables) gNumberOfVariables = value;
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
            // process complete rules
            if (state == 0) {
                if (newrule != NULL) {
                    // check if last rule is empty  -> end input
                    if (newrule->lhs==NULL && newrule->rhs==NULL) {
                        printf("Empty rule ends input.\n");
                        break;
                    } else {
                        TestAddTestRule(newrule);
                    }
                }
            }
        }  
    } // while (!eof())
    
    if (feof(input))
        printf("EOF ends input.\n");

    // switch to stdin if inputfile eof
    if (feof(input)) input = stdin;

    // ---- get input numbers (rules without lhs) 
    // this will fix variables by adding rules
    
    TestRule* rule = gFirstTestRule;
    int inputcount = 0;
    while (rule != NULL) {
        if (rule->lhs == NULL && rule->rhs != NULL) {
            // count number of bits
            int bitcount = 0;
            TestNode* node = rule->rhs;
            while (node != NULL) {
                node = node -> next;
                bitcount++;
            }
            // get input
            if (input == stdin)
                printf("input #%d (%d bits) : ",++inputcount, bitcount);
            int number;
            if (fscanf(input, "%d",&number)!= EOF)
                TestSetNumber(rule->rhs, number); 
        }
        rule = rule -> next;
    }

    // -------------- print all rules

    // for all rules
    newrule = gFirstTestRule;
    int count = 0;
    while (newrule) {
        printf("Rule: ");
        // for all lhs
        newlit = newrule->lhs;
        while (newlit) {
            printf("%d ",newlit->value);
            newlit = newlit->next;
        }
        printf("-> ");
        // for all rhs     
        newlit = newrule->rhs;
        while (newlit) {
            printf("%d ",newlit->value);
            newlit = newlit->next;
        }
        printf("\n");
        newrule = newrule->next;
        // dont print huge problems
        count++;
        if (count > 100) {
            printf("...\n");
            break;
        }
    }
    printf("number of variables = %d\n",gNumberOfVariables);
    printf("number of rules = %d\n",gNumberOfTestRules);

    // -------------- convert problem -------

    // allocate array for problem
    rule = gFirstTestRule;
    int arraysize = 0;
    while (rule) {
        newlit = rule->lhs;
        while (newlit) {
            arraysize++;
            newlit = newlit->next;
        }
        arraysize++; // terminating zero lhs
        newlit = rule->rhs;
        while (newlit) {
            arraysize++;
            newlit = newlit->next;
        }
        rule = rule->next;
        arraysize++; // terminating zero rhs
    }
    arraysize++; // for the double-zero rule
    arraysize++; // for the double-zero rule
    
    int* array = (int*)malloc(arraysize*sizeof(int));
    
     // convert problem to array
    rule = gFirstTestRule;
    int index = 0;
    while (rule) {
        newlit = rule->lhs;
        while (newlit) {
            array[index++] = newlit->value;
            newlit = newlit->next;
        }
        array[index++] = 0;         // terminating zero
        newlit = rule->rhs;
        while (newlit) {
            array[index++] = newlit->value;
            newlit = newlit->next;
        }
        array[index++] = 0;         // terminating zero
        rule = rule->next;
    }
    array[index++] = 0;         // terminating zero
    array[index++] = 0;         // terminating zero

    // ---------- create solver and get solutions! -----

    Solver* solver = NewSolver(array);
    int numSolutions = 0;
    int v;
    while (NextSolution(solver)) {
        numSolutions++;
        printf("Solution #%d:\n",numSolutions);
        // print output numbers (rules with no rhs)
        TestRule* rule = gFirstTestRule;
        int outputcount = 0;
        while (rule != NULL) {
            if (rule->rhs == NULL && rule->lhs != NULL) {
                // count number of bits
                int bitcount = 0;
                TestNode* node = rule->lhs;
                while (node != NULL) {
                    node = node -> next;
                    bitcount++;
                }
                // show 
                printf("output #%d (%d bits) = %d\n",++outputcount, bitcount, TestGetNumber(solver,rule->lhs));
            }
            rule = rule -> next;
        }
        // print complete solution (if no output rules defined)
        if (outputcount == 0) {
            for (v=1;v<=gNumberOfVariables;v++) {
                if (solver->gMarkers[Literal2Index(v)]!=0 && solver->gMarkers[Literal2Index(-v)]!=0) printf("%d=!!! ",v);
                else if (solver->gMarkers[Literal2Index(v)]!=0) printf("%d=T ",v);
                else if (solver->gMarkers[Literal2Index(-v)]!=0) printf("%d=F ",v);
                else printf("%d=??? ",v);
                if (v > 10) {printf("..."); break;}
            }
            printf("\n");
        }

    }
    printf("%d solutions\n", numSolutions);
    return 0;
}
