/*  SillyCon

    Interpretor for a silly constraint language

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

#include "bps.h"

#define MAX_WORD_LENGTH     255     // buffer for parsing terms 
#define MAX_VARS    1000            // memory allocated for varibales
#define NUMBER_WIDTH    9           // size of numbers in bits
#define MAX_SOLUTIONS   100         // max number of solutions printed

typedef enum {NONE=0,ALPHA,DIGIT,SPACE,PUNCT,OTHER}  Kind;

/* parse a terminal from input, return it's kind and fill a string with the terminal's chars */ 
/* A terminal is a string that is either: 
        a string alhabetic chars
        a string of digits
        a punctuation character
   Never creates a string with whitespace or other characters
   Number of characters in string <= MAX_WORD_LENGTH
   Returns the kind of the term
   Note that term will be appended with a 0, so must be allocated for MAX_WORD_LENGTH+1
*/
Kind parseTerm(FILE* input, char* term) {
    int count = 0;
    int c;
    Kind lastkind = NONE;
    Kind kind = NONE;
    while ((c = fgetc(input)) != 0 && count < MAX_WORD_LENGTH && !feof(input)) {
        if (c == '"') { 
            // comment
            while (!feof(input) && fgetc(input) != '"') {}; 
            continue;
        }
        else if (isalpha(c)) kind = ALPHA;
        else if (isdigit(c)) kind = DIGIT;
        else if (isspace(c)) kind = SPACE;
        else if (ispunct(c)) kind = PUNCT;
        else kind=OTHER;
        if (lastkind == NONE) {
            if (kind == PUNCT) {
                term[count++] = c;
                lastkind = kind;
                break;
            }
            // also limit alpha's to one char, for now
             if (kind == ALPHA) {
                term[count++] = c;
                lastkind = kind;
                break;
            }
            else if (kind != SPACE && kind!=OTHER) {
                term[count++] = c;
                lastkind = kind;
            }
        }
        else if (kind == lastkind) { 
            term[count++] = c;
            lastkind = kind;
        }
        else 
        {
            ungetc(c, input);
            break;
        }
    }
    term[count]=0;
    return lastkind;
}

//  ----------- Expression tree ----------

typedef enum {VAR,NUM,                   // terminals
    NOT,NEG,                             // unary operators
    EQ,AND,OR,XOR,IMPL,                    // binary operators (logic) 
    ADD,SUB,MUL,DIV,MOD,                 // binary operators (artihmetic)
    LT,GT,                               // binary operators (comparisons)
    EVAL,MIN,MAX,IND,                    // special solvers (binary)
    COUNT,                               // counting solver (unary)
    CON,                                 // constrain equal 1 (unuary)
    PNTR                                  // a pointer to a variable 
} Operator;

typedef struct {
    Operator op;
    char* term;
    void* left;
    void* right;
} Expr;

Expr* newExpr(Operator op, char* term, Expr* left, Expr* right) {
        Expr* expr = malloc(sizeof(Expr));
        expr->op = op;
        expr->left = left;
        expr->right = right;
        int len = strlen(term)+1;
        expr->term = malloc(len*sizeof(char));
        strncpy(expr->term, term, len);
        return expr;
}

Expr* parseExpr(FILE* input) {
    char term[MAX_WORD_LENGTH+1];
    Kind kind = parseTerm(input, term);
    Expr* expr = NULL;
    if (kind == PUNCT) {
        Operator op;
        if (strcmp(term,"+")==0) op = ADD; 
        else if (strcmp(term,"*")==0) op = MUL; 
        else if (strcmp(term,"/")==0) op = DIV; 
        else if (strcmp(term,"%")==0) op = MOD; 
        else if (strcmp(term,"&")==0) op = AND; 
        else if (strcmp(term,"|")==0) op = OR;
        else if (strcmp(term,"^")==0) op = XOR;
        else if (strcmp(term,"=")==0) op = EQ;
        else if (strcmp(term,":")==0) op = IMPL;
        else if (strcmp(term,">")==0) op = GT;
        else if (strcmp(term,"<")==0) op = LT;
        else if (strcmp(term,"!")==0) op = NOT;
        // else if (strcmp(term,"-")==0) op = SUB; 
        else if (strcmp(term,"-")==0) op = NEG;
        else if (strcmp(term,"@")==0) op = CON;
        else if (strcmp(term,"#")==0) op = COUNT;
        else if (strcmp(term,"$")==0) op = MAX;
        else if (strcmp(term,"_")==0) op = MIN;
        else if (strcmp(term,"'")==0) op = EVAL;
        else if (strcmp(term,"`")==0) op = IND;
        else if (strcmp(term,"?")==0) op = PNTR;
        else {
            printf("Unknown operator: %s\n",term);
        return NULL;
        }
        // get left and right (for binary operators)
        Expr* left = NULL;
        Expr* right = NULL;
        if (op == CON || op == NOT || op == COUNT || op == NEG || op == PNTR) {
            // unary operators
            left = parseExpr(input);
            if (left == NULL) { 
                printf("Error: Incomplete input.\n");
                return NULL;
            }
        } else {
            // binary operators
            left = parseExpr(input);
            right = parseExpr(input);
            if (left == NULL) { 
                printf("Error: Incomplete Input.\n");
                return NULL;
            }
            if (right == NULL) { 
                printf("Error: Incomplete Input.\n");
                return NULL;
            }
        }
        expr = newExpr(op, term, left, right);
    }
    else if (kind == DIGIT) {
        expr = newExpr(NUM, term, NULL,NULL);
    }
    else if (kind == ALPHA) {
        expr = newExpr(VAR, term, NULL,NULL);
    }
    else if (kind == SPACE) {
        //printf("space\n");
    }
    else if (kind == OTHER) {
        //printf("other\n");
    }
    else if (kind == NONE) {
    }
    return expr;
}

void printExpr(Expr* expr, int depth) {
    if (expr == NULL) return;
    int i;
    for (i=0;i<depth;i++) {
        printf(" ");
    }
    printf("%s\n",expr->term);
    printExpr(expr->left, depth+1);
    printExpr(expr->right, depth+1);
}

// --------------- misc ------

List* copyList(List* list) {
    List* new = NewList();
    Node* node = list->first;
    while (node!=NULL) {
        ListAppend(new, node->value);
        node = node->next;
    }
    return new;
}

void printList(List* list) {
    if (list == NULL) {return;}
    printf("[ ");
    Node* node = list->first;
    while (node!=NULL) {
        printf("%d ",node->value);
        node = node->next;
    } 
    printf("]\n");
}

int* List2Array(List* l) {
    if (l==NULL) return NULL;
    if (l->count < 1) return NULL;
    int* a = malloc((l->count+2)*sizeof(int));
    int i = 0;
    Node* n = l->first;
    while (n) {
        a[i++] = n->value;
        n = n->next;
    }
    a[i++] = 0;
    a[i++] = 0;
    return a;
}

// ---------------- problem -------------


typedef struct {
    int maxlit;         // highest used literal
    List* rules;        // all the rules in the form of a list of literals with zeros to terminate clauses 
    int* var2lit;       // maps variable to first literal 
    int true;           // literal that is true (always in the solution)
    int false;          // literal that is false (never in the solution)
} Problem;

int newLit(Problem* p) {
    // new literal, increasing maxlit 
    int lit = ++p->maxlit;
    // add rule that does nothing but add literal to problem
    // int lhs[1] = {0};
    // int rhs[2] = {lit, 0};
    // addRule(p, lhs, rhs);   
    return lit; 
}

void addRule(Problem* p, int* lhs, int* rhs) {
    int lit;
    int i=0;
    while ((lit=lhs[i++])!=0) {
        ListAppend(p->rules, lit);
    }
    ListAppend(p->rules, 0);
    i=0;
    while ((lit=rhs[i++])!=0) {
        ListAppend(p->rules, lit);
    }
    ListAppend(p->rules, 0);
}

void addRule2(Problem* p, int l1, int l2, int r1, int r2) {
    if (l1 != 0) ListAppend(p->rules, l1);
    if (l2 != 0) ListAppend(p->rules, l2);
    ListAppend(p->rules, 0);
    if (r1 != 0) ListAppend(p->rules, r1);
    if (r2 != 0) ListAppend(p->rules, r2);
    ListAppend(p->rules, 0);
}

Problem* newProblem () {
    // allocate
    Problem* p = (Problem*) malloc(sizeof(Problem));
    p->maxlit = 0;
    p->rules = NewList(); 
    p->var2lit = malloc(MAX_VARS*sizeof(int));
    // clear mapping
    int i;
    for (i=0;i<MAX_VARS;i++) {     // unsi
        p->var2lit[i] = 0;
    }
    // define true and false; only true can be in a solution
    int lit = newLit(p);
    int lhs[2] = {-lit, 0};
    int rhs[2] = {lit, 0};
    addRule(p, lhs, rhs);
    p->true = lit;
    p->false = -lit;
    return p;        
} 

void deleteProblem (Problem* p) {
    if (p == NULL) return; 
    free(p->var2lit);
    DeleteList(p->rules);
    free(p);
}

Problem* copyProblem (Problem* p) {
    Problem* new = newProblem();
    DeleteList(new->rules);
    new->rules = copyList(p->rules);
    int i;
    for (i=0;i<MAX_VARS;i++) {
        new->var2lit[i] = p->var2lit[i];
    } 
    new->maxlit = p->maxlit;
    new->true = p->true;
    new->false = p->false;
    return new;
}

char* makeVarName(int var) {
    char* name = malloc(16);
    if (isalpha(var) && var <= 255) {
        char alphaname[2] = {(char)var,0};
        strncpy(name, alphaname,16);
    }
    else {
        name[0] = '?';
        char numname[13];
        snprintf(numname,12,"%d",var);
        strncpy(name+1, numname,12);
    }
    return name;
}

List* getVariable(Problem* p, char* name) {
    if (strlen(name)==0) {
        printf("Error: empty variable name.\n");
        return NULL;
    } 
    char c0= name[0];
    int var;
    if (isalpha(c0)) 
        var = (int)c0;
    else if (name[0] == '?')  
        var = atoi(name+1);
    else {
        printf("Error: illegal variable name: %s.\n",name);
        return NULL;
    }
    if (var < 1) {
        printf("Error: var < 1.\n");
        return NULL;
    }
    if (var >= MAX_VARS) {
        printf("Error: var >= MAX_VAR.\n");
        return NULL;
    }

    // find in mapping
    int firstlit = p->var2lit[var];
    if (firstlit == 0) {
        return NULL;
    }
    // return set of literals
    List* literals = NewList();
    int lit;
    for (lit=firstlit;lit<firstlit+NUMBER_WIDTH;lit++) {
        ListAppend(literals,lit);
    }
    return literals;
    
}

List* addOrGetVariable(Problem* p, char* name) {
    if (strlen(name)==0) {
        printf("Error: empty variable name.\n");
        return NULL;
    } 
    char c0 = name[0]; 
    int var;
    if (isalpha(c0)) 
        var = (int)c0;
    else if (name[0] == '?')  
        var = atoi(name+1);
    else {
        printf("Error: illegal variable name: %s.\n",name);
        return NULL;
    }
 
    if (var < 1) {
        printf("Error: var < 1.\n");
        return NULL;
    }
    if (var >= MAX_VARS) {
        printf("Error: var >= MAX_VAR.\n");
        return NULL;
    }
    // find in mapping
    int firstlit = p->var2lit[var];
    if (firstlit == 0) {
        // .. or create new
        firstlit = newLit(p); 
        int i;
        for (i=1;i<NUMBER_WIDTH;i++) {
            newLit(p);
        }
        // store mapping
        p->var2lit[var] = firstlit;
    }
    // return set of literals
    List* literals = NewList();
    int lit;
    for (lit=firstlit;lit<firstlit+NUMBER_WIDTH;lit++) {
        ListAppend(literals,lit);
    }
    return literals;
 
}

// ---------- constraints and expressions

// ----- boolean constraints ----

void constrainBoolTrue(Problem* problem, int lit)
{    
    addRule2(problem, -lit, 0, lit, 0);
}

void constrainBoolFalse(Problem* problem, int lit)
{    
    addRule2(problem, lit, 0, -lit, 0);
}

// --------- boolean expressions ------

// returns a new literal (positive literal, so also a varible!) and constrains it to negative 
// Note: you only need this if the result must be a variable, not a negative literal
// for exmaple when used for variable ordering
int makeBoolNot(Problem* p, int literal) {
    int var = newLit(p);
    addRule2(p, var, 0, -literal, 0);
    addRule2(p, -var, 0, +literal, 0);
    addRule2(p, literal, 0, -var, 0);
    addRule2(p, -literal, 0, var, 0);
    return var;
}
 
// returns a variable with the same value as the literal
// i.e. if the literal is negative, return a new var and constrain it with a not operator
// if the literal is positive, just return it
int literal2var(Problem* p, int literal) {
    if (literal < 0) 
        return makeBoolNot(p, literal);
    else
        return literal;
}

int makeBoolAnd(Problem* p, int l, int r) {
    int and = newLit(p);
    addRule2(p, l, r, and, 0 ); 
    addRule2(p, -l, 0, -and, 0 ); 
    addRule2(p, -r, 0, -and, 0 ); 
    addRule2(p, and, 0, l, r ); 
    addRule2(p, -and, l, -r,0 ); 
    addRule2(p, -and, r, -l,0 ); 
    return and;
}

int makeBoolOr(Problem* p, int l, int r) {
    int or = newLit(p);
    addRule2(p, -l, -r, -or, 0 ); 
    addRule2(p, l, 0, or, 0 ); 
    addRule2(p, r, 0, or, 0 ); 
    addRule2(p, -or, 0, -l, -r ); 
    addRule2(p, or, -l, r, 0 ); 
    addRule2(p, or, -r, l, 0 ); 
    return or;
}

int makeBoolXor(Problem* p, int l, int r) {
    int xor = newLit(p);
    addRule2(p, -l, -r, -xor, 0 ); 
    addRule2(p, -l, r, xor, 0 ); 
    addRule2(p, l, r, -xor, 0 ); 
    addRule2(p, l, -r, xor, 0 ); 
    addRule2(p, -xor, -l, -r, 0 ); 
    addRule2(p, -xor, -r, -l, 0 ); 
    addRule2(p, -xor, l, r, 0 ); 
    addRule2(p, -xor, r, l, 0 ); 
    addRule2(p, xor, -l, r, 0 ); 
    addRule2(p, xor, -r, l, 0 ); 
    addRule2(p, xor, l, -r, 0 ); 
    addRule2(p, xor, r, -l, 0 ); 
    return xor;
}


int makeBoolEq(Problem* p, int l, int r) {
    int eq = newLit(p);
    addRule2(p, l, r, eq, 0 ); 
    addRule2(p, -l, -r, eq, 0 ); 
    addRule2(p, l, -r, -eq, 0 ); 
    addRule2(p, -l, r, -eq, 0 ); 
    addRule2(p, eq, l , r, 0 ); 
    addRule2(p, eq, -l , -r, 0 ); 
    addRule2(p, eq, r , l, 0 ); 
    addRule2(p, eq, -r , -l, 0 ); 
    addRule2(p, -eq, l , -r, 0 ); 
    addRule2(p, -eq, -l , r, 0 ); 
    addRule2(p, -eq, r , -l, 0 ); 
    addRule2(p, -eq, -r , l, 0 ); 
    return eq;
}

int makeBoolImpl(Problem* p, int l, int r) {
    int impl = newLit(p);
    addRule2(p, -l, 0, impl, 0 );        
    addRule2(p, l, r, impl, 0 );        
    addRule2(p, l, -r, -impl, 0 );        
    addRule2(p, impl, l, r, 0 );        
    addRule2(p, -impl, 0, l, -r );        

    return impl;
}


void makeBoolAdd (Problem* problem, int a, int b, int carryin, int* out, int *carryout) {
    int out1 = makeBoolAnd(problem,a, makeBoolAnd(problem,-b,-carryin));
    int out2 = makeBoolAnd(problem,b, makeBoolAnd(problem,-a,-carryin));
    int out3 = makeBoolAnd(problem,carryin, makeBoolAnd(problem,-a,-b));
    int out4 = makeBoolAnd(problem,a, makeBoolAnd(problem,b,carryin));
    *out = makeBoolOr(problem,out1, makeBoolOr(problem,out2, makeBoolOr(problem,out3, out4)));
    *carryout = makeBoolOr(problem,makeBoolAnd(problem,a,b), makeBoolOr(problem,makeBoolAnd(problem,a,carryin), makeBoolAnd(problem,carryin, b)));
}

// -------------- numbers -------------

/* Determines a list of literals 
   Numbers are defined with most significant bit first; in two's complement for negative numbers.
   Note: postive numbers have at least two digits and start with 0:  00, 01, 010, etc
   Note: negative numbers also have at least two digits, and start with 1
   
*/
List* makeNumber (Problem* problem, int number) {
    List* literals = NewList();
    if (number == 0) {   
	    // special case: we want at least two bits for makeNumNot to work
	    ListPrepend(literals, problem->false);
	    ListPrepend(literals, problem->false);
    }
    else if (number>0) { 
        // starting with least significant bit
        while (number != 0) {
            // if number is odd, add true to list
            if ((number & 0x1) == 1)
                ListPrepend(literals, problem->true);
            else
                ListPrepend(literals, problem->false);
            // shift out least significant bit
            number >>= 1;
        }
        // add sign bit 
        ListPrepend(literals, problem->false);
    }
    else // negative number
    {
        // two's complement: negative is invert bits and add one
        number = abs(number)-1;
        // starting with least significant bit
        while (number != 0) {
            // if number is odd, add FALSE to list -> invert bits 
            if ((number & 0x1) == 1)
                ListPrepend(literals, problem->false);
            else
                ListPrepend(literals, problem->true);
            // shift out least significant bit
            number >>= 1;
        }
        // add sign bit
        ListPrepend(literals, problem->true);
    }
    return literals;
}

// substitute bps.c's GetNumber with a signed version 
int getNumber (Solver* s, int* literals) {
    int number = 0;
    int index = 0;
    // get sign bit
    int lit = literals[index++];
    int negative = (s->gMarkers[Literal2Index(lit)]!=0);
    // get value
    if (negative) {
        while ((lit = literals[index++])!=0) {
            // shift right
            number <<= 1;
            // if literal NOT in solution, add 1    - two's complement, invert bits
            if (s->gMarkers[Literal2Index(lit)]==0) ++number;
        } 
        return -(number+1);    // two's complement - add 1
    }
    else    // positive
    {
        while ((lit = literals[index++])!=0) {
            // shift right
            number <<= 1;
            // if literal in solution, add 1
            if (s->gMarkers[Literal2Index(lit)]!=0) ++number;
        } 
        return number;
    }
}



// return a new number variable 
List* makeNumVar(Problem* problem) { 
    List* literals = NewList();
    int i;
    for (i=0;i<NUMBER_WIDTH;i++) {
        int lit = newLit(problem);
        ListAppend(literals,lit);
    }
    return literals;
} 


// boolean not; each bit is inverted
// for 2s complement: !0=-1, !1=-2
List* makeNumInvert (Problem* problem, List* left) {
    // check input
    if (problem==NULL||left==NULL) return NULL;
    // invert number
    Node* l = left->first;
    List* literals = NewList();
    while (l != NULL) { 
        ListAppend(literals, -l->value);
        l = l->next;
    }
    // result variable
    return literals;
}


// increment number; does not extend it
List* makeNumIncr (Problem* problem, List* left) {
    // check input
    if (problem==NULL||left==NULL) return NULL;
    // NOTE: does not extend number!
    Node* l = left->last;
    List* literals = NewList();
    int carry = problem->true;      // start with carry = 1
    while (l != NULL) {
        int out;
        makeBoolAdd(problem, l->value, problem->false, carry, &out, &carry);
        ListPrepend(literals, out);
        l = l->prev;
    }
    // result variable
    return literals;
}


// negative of a 2's complement
List* makeNumNeg (Problem* problem, List* left) {
    // check input
    if (problem==NULL||left==NULL) return NULL;
    // two's complement: invert and add one
    List* invert = makeNumInvert(problem, left);
    List* literals = makeNumIncr(problem, invert);
    DeleteList(invert); 
    // result variable
    return literals;
}

// extend number to given size for 2's compilement (extend same as sign)
List* extendNumber(Problem* problem, List* literals, int size) {
    // check input
    if (problem==NULL||literals==NULL) return NULL;
    // first copy value except sign bit
    List* newLiterals = NewList();
    int count = 0;
    Node* l = literals->last;
    while (l && l!=literals->first) {
        ListPrepend(newLiterals,l->value);
        count++;
        l = l->prev;
    }
    // filler: same as sign bit!
    l = literals->first;
    int sign = problem->false;
    if (l!=NULL) 
        sign = l->value; 
    while (count < size) {
        ListPrepend(newLiterals, sign);
        count++;
    }
    // done
    return newLiterals;
}

// extend natural binary number to given size (preprend 0's)
List* extendBinary(Problem* problem, List* literals, int size) {
    // check input
    if (problem==NULL||literals==NULL) return NULL;
    // first copy value 
    List* newLiterals = NewList();
    int count = 0;
    Node* l = literals->last;
    while (l != NULL) { 
        ListPrepend(newLiterals,l->value);
        count++;
        l = l->prev;
    }
    // filler
    while (count < size) {
        ListPrepend(newLiterals, problem->false);
        count++;
    }
    // done
    return newLiterals;
}

// Note: result is a two bit 00 or 01!
List* makeNumEq (Problem* problem, List* left, List* right) {
    // check input, make left and right same length
    if (problem==NULL||left==NULL||right==NULL) return NULL;
    int el = 0;
    int er = 0;
    if (left->count > right->count) {
        right = extendNumber(problem, right, left->count);
        er = 1;
    }
    if (left->count < right->count) {
        left = extendNumber(problem, left, right->count);
        el = 1;
    }
    
    Node* l = left->first;
    Node* r = right->first;
    int and = problem->true;
    while (l != NULL && r !=NULL) {
        int eq = makeBoolEq(problem, l->value, r->value);
        and = makeBoolAnd(problem, eq, and);
        l = l->next;
        r = r->next;
    }

    // clean up extendNumbers
    if (el) DeleteList(left);
    if (er) DeleteList(right);

    // result variable
    List* literals = NewList();
    ListAppend(literals, problem->false);   // sign bit 
    ListAppend(literals, and);              // value
    return literals;
}

// A constant number constraint

void constraintNumConst(Problem* problem, List* literals, int value) {
    List* num = makeNumber(problem, value);
    List* eq = makeNumEq(problem, literals, num);
    // result of eq must be 000001
    Node* eq_i = eq->first;
    while (eq_i->next) {
        constrainBoolFalse(problem, eq_i->value);
        eq_i = eq_i->next;
    }
    constrainBoolTrue(problem, eq_i->value);
    DeleteList(eq);
    DeleteList(num);
}

List* makeNumAnd (Problem* problem, List* left, List* right) {
    // check input, make left and right same length
    if (problem==NULL||left==NULL||right==NULL) return NULL;
    int el = 0;
    int er = 0;
    if (left->count > right->count) {
        right = extendNumber(problem, right, left->count);
        er = 1;
    }
    if (left->count < right->count) {
        left = extendNumber(problem, left, right->count);
        el = 1;
    }
    
    Node* l = left->first;
    Node* r = right->first;
    List* literals = NewList();
    while (l != NULL && r !=NULL) {
        int eq = makeBoolAnd(problem, l->value, r->value);
        ListAppend(literals, eq);
        l = l->next;
        r = r->next;
    }

    // clean up extendNumbers
    if (el) DeleteList(left);
    if (er) DeleteList(right);

    // result variable
    return literals;
}

//Note: returns a two bit 00 or 01
List* makeNumImpl (Problem* problem, List* left, List* right) {
    // check input, make left and right same length
    if (problem==NULL||left==NULL||right==NULL) return NULL;
    int el = 0;
    int er = 0;
    if (left->count > right->count) {
        right = extendNumber(problem, right, left->count);
        er = 1;
    }
    if (left->count < right->count) {
        left = extendNumber(problem, left, right->count);
        el = 1;
    }
    
    Node* l = left->first;
    Node* r = right->first;
    int and = problem->true;
    while (l != NULL && r !=NULL) {
        int eq = makeBoolImpl(problem, l->value, r->value);
        and = makeBoolAnd(problem, eq, and);
        l = l->next;
        r = r->next;
    }

    // clean up extendNumbers
    if (el) DeleteList(left);
    if (er) DeleteList(right);

    // result variable
    List* literals = NewList();
    ListAppend(literals, problem->false);   // sign bit 
    ListAppend(literals, and);              // value
    return literals;

}


// numercial not; invert all but first bit (sign bit) 
// for 2s complement: !0=1, !1=0
List* makeNumNot(Problem* problem, List* left) {
    // check input
    if (problem==NULL||left==NULL) return NULL;
    List* literals = NewList();
    // copy first bit
    Node* l = left->first;
    if (l!= NULL) { 
        ListAppend(literals, l->value);
        l = l->next;
    }
    // invert number
    while (l != NULL) { 
        ListAppend(literals, -l->value);
        l = l->next;
    }
    // result variable
    return literals;
}


List* makeNumOr (Problem* problem, List* left, List* right) {
    // check input, make left and right same length
    if (problem==NULL||left==NULL||right==NULL) return NULL;
    int el = 0;
    int er = 0;
    if (left->count > right->count) {
        right = extendNumber(problem, right, left->count);
        er = 1;
    }
    if (left->count < right->count) {
        left = extendNumber(problem, left, right->count);
        el = 1;
    }
    
    Node* l = left->first;
    Node* r = right->first;
    List* literals = NewList();
    while (l != NULL && r !=NULL) {
        int eq = makeBoolOr(problem, l->value, r->value);
        ListAppend(literals, eq);
        l = l->next;
        r = r->next;
    }

    // clean up extendNumbers
    if (el) DeleteList(left);
    if (er) DeleteList(right);

    // result variable
    return literals;
}

List* makeNumXor (Problem* problem, List* left, List* right) {
    // check input, make left and right same length
    if (problem==NULL||left==NULL||right==NULL) return NULL;
    int el = 0;
    int er = 0;
    if (left->count > right->count) {
        right = extendNumber(problem, right, left->count);
        er = 1;
    }
    if (left->count < right->count) {
        left = extendNumber(problem, left, right->count);
        el = 1;
    }
    
    Node* l = left->first;
    Node* r = right->first;
    List* literals = NewList();
    while (l != NULL && r !=NULL) {
        int eq = makeBoolXor(problem, l->value, r->value);
        ListAppend(literals, eq);
        l = l->next;
        r = r->next;
    }

    // clean up extendNumbers
    if (el) DeleteList(left);
    if (er) DeleteList(right);

    // result variable
    return literals;
}


List* makeNumAdd (Problem* problem, List* left, List* right) {
    // check input, make left and right same length
    if (problem==NULL||left==NULL||right==NULL) return NULL;
    int el = 0;
    int er = 0;
    // size = max (left->count,right->count)+1;
    int size = left->count+1;
    if (right->count + 1 >size) size = right->count+1;
    // extend to size, always add one bit for overflow
    if (right->count<size) {
        right = extendNumber(problem, right, size);
        er = 1;
    }
    if (left->count<size) {
        left = extendNumber(problem, left, size);
        el = 1;
    }
    
    Node* l = left->last;
    Node* r = right->last;
    int carry = problem->false;
    List* literals = NewList();
    // for all value bits (all execpt first, sign bit)
    while (l != NULL && r !=NULL) {
        int out;
        makeBoolAdd(problem, l->value, r->value, carry, &out, &carry);
        ListPrepend(literals, out);
        l = l->prev;
        r = r->prev;
    }
    
    // add carry 
    // ListPrepend(literals, carry);
    
    // clean up extendNumbers
    if (el) DeleteList(left);
    if (er) DeleteList(right);
    // result variable
    return literals;
}


List* makeNumIf (Problem* problem, int selector, List* left, List* right) {
    // check input, make left and right same length
    if (problem==NULL||left==NULL||right==NULL) return NULL;
    int el = 0;
    int er = 0;
    if (left->count > right->count) {
        right = extendNumber(problem, right, left->count);
        er = 1;
    }
    if (left->count < right->count) {
        left = extendNumber(problem, left, right->count);
        el = 1;
    }
    
    Node* l = left->first;
    Node* r = right->first;
    List* literals = NewList();
    while (l != NULL && r !=NULL) {
        int first = makeBoolAnd(problem, selector, l->value); 
        int second = makeBoolAnd(problem, -selector, r->value); 
        int val = makeBoolOr(problem, first, second);
        ListAppend(literals, val);
        l = l->next;
        r = r->next;
    }

    // clean up
    if (el) DeleteList(left);
    if (er) DeleteList(right);

    // result variable
    return literals;
}


List* makeNumAbs(Problem* problem, List* number) {
    // check input
    if (problem==NULL||number==NULL) return NULL;
    // if sign bit set then return negative
    int sign = number->first->value;
    List* neg = makeNumNeg(problem, number);
    List* result = makeNumIf(problem, sign, neg, number);
    // cleanup
    DeleteList(neg);
    return result;
}

List* makeNumSign(Problem* problem, int signbitliteral, List* number) {
    // check input
    if (problem==NULL||number==NULL) return NULL;
    // if sign bit set then return negative
    int numbersignliteral = number->first->value;
    int changesignliteral = makeBoolXor(problem, signbitliteral, numbersignliteral);
    List* neg = makeNumNeg(problem, number);
    List* result = makeNumIf(problem, changesignliteral, neg, number);
    // cleanup
    DeleteList(neg);
    return result;
}


List* makeNumSub (Problem* p, List* left, List* right) {
    List* neg = makeNumNeg(p, right);    
    List* literals = makeNumAdd(p, left, neg);
    DeleteList(neg);
    return literals;
}

List* makeNumShiftL(Problem* problem, List* list) {
    List* new = NewList();
    // add 0 on the right
    // thats the shift: * 2
    ListPrepend(new, problem->false);      
    // copy rest
    Node* node = list->last;
    while (node!=NULL) {
        ListPrepend(new, node->value);
        node = node->prev;
    }
    return new;
}

List* makeNumMulPos (Problem* problem, List* left, List* right) {
    if (left == NULL || right==NULL) return NULL;
    Node* r = right->last;
    List* result = NewList();
    List* shift = copyList(left);
    while (r !=NULL) {
        // add shifted value to total, IF current right bit is true 
        List *add = makeNumAdd(problem, result, shift);
        List *newresult = makeNumIf(problem, r->value, add, result);
        DeleteList(add); 
        DeleteList(result);
        result = newresult;
    
        // iterative shift the left side (to the list) 
        List* newshift = makeNumShiftL(problem, shift);
        DeleteList(shift); 
        shift = newshift;
    
        // next
        r = r->prev;        
    }

    // result variable
    return result;
}

List* makeNumMul (Problem* problem, List* left, List* right) {
/*
    // if right is negative then compute multiplication with -right and negate total
    List* rneg = makeNumNeg(problem, right);
    List* mulneg = makeNumMulPos(problem, left, rneg);
    List* mulnegneg = makeNumNeg(problem, mulneg);
    List* mulpos = makeNumMulPos(problem, left, right);
    int sign = right->first->value;
    List* literals = makeNumIf(problem, sign, mulnegneg, mulpos);
    DeleteList(rneg);
    DeleteList(mulneg);
    DeleteList(mulnegneg);
    DeleteList(mulpos);
    // add shortcut for sign bit - to help performance      // BUG: 0 * -1 -> no solutions!
    // int signl = left->first->value;
    // int signr = right->first->value;
    // int signm = literals->first->value;
    // constrainBoolTrue(problem, makeBoolEq(problem, signm, makeBoolXor(problem, signl, signr)));
    // result
    return literals;
*/
    List* absl = makeNumAbs(problem,left);
    List* absr = makeNumAbs(problem,right);
    List* prod = makeNumMulPos(problem, absl,absr);
    int signl = left->first->value;
    int signr = right->first->value;
    int signp = makeBoolXor(problem,signl, signr);
    List* literals = makeNumSign(problem, signp, prod);
    DeleteList(absl);
    DeleteList(absr);
    DeleteList(prod);
    return literals;
}


List* makeNumLte (Problem* p, List* left, List* right)
{
        // carry bit should be 0
        List* sub = makeNumSub(p, left, right);
        int carry = sub->first->value;
        DeleteList(sub);
 
        // create variable    
        List* literals = NewList();
        ListAppend(literals, p->false);   // sign bit = 0
        ListAppend(literals, -carry);
        return literals;
}

List* makeNumLt (Problem* p, List* left, List* right)
{
    List* lte = makeNumLte(p, right, left);
    List* eq = makeNumEq(p, right, left);
    List* neq = makeNumNot(p, eq);
    List* literals = makeNumAnd(p, lte, neq);
    DeleteList(lte);
    DeleteList(eq);
    DeleteList(neq);
    return literals;
}


// only works correctly for positive numbers
List* makeNumDivPos (Problem* p, List* left, List* right) {
    
    /* Basic binary long division - http://en.wikipedia.org/wiki/Division_algorithm 
    
    if D == 0 then throw DivisionByZeroException end
    Q := 0                 initialize quotient and remainder to zero
    R := 0                     
    for i = n-1...0 do     where n is number of bits
      R := R << 1          left-shift R by 1 bit    
      R(0) := N(i)         set the least-significant bit of R equal to bit i of the numerator
      if R >= D then
        R = R - D               
        Q(i) := 1
      end
    end  
    */

    List* n = left;                 // numerator
    List* d = right;                // divisor
    // q=0 and r = 0
    // ensure q and r as long as n
    List* zero = makeNumber(p,0);   
    List* q = extendNumber(p, zero, n->count);        // quotient 
    List* r = extendNumber(p, zero, n->count);        // remaineder 
    DeleteList(zero);
    // init walking elements
    Node* ni = n->first->next;              // n(i); skip sign bit; starting with most significant bit
    Node* qi = q->first->next;              // q(i); skip sign bit; starting with most significant bit
    
    // no solution if d == 0
    List* d0 = makeNumEq(p, d, zero);
    constrainBoolFalse(p, d0->last->value);
    
    //  for each digit of n and q, starting with most significant bit
    while (ni != NULL && qi != NULL) {       
        // r = r << 1
        List* r2 = makeNumShiftL(p, r);
        DeleteList(r);
        r = r2;
        // r(k)=n(i) where k is least significant bit
        r->last->value = ni->value;        
        // if r>= d then r = r - d; else r = r
        List* same = r;
        List* subtract = makeNumSub(p, r, d);    
        List* lt = makeNumLt(p, r, d);      
        List* gte = makeNumNot(p, lt);
        int greater = gte->last->value; 
        List* r3 = makeNumIf(p, greater, subtract, same);
        DeleteList(r);
        r = r3;
        // if r>=d then q(i) = true else false, i.e. q(i)=(r>=d)
        qi->value = greater;
        // next (less significant) bit
        ni = ni->next;
        qi = qi->next;
    }

    DeleteList(r);

    return q;
}

List* makeNumDiv (Problem* p, List* left, List* right) {

    // get sign bits
    int signl = left->first->value;
    int signr = right->first->value;

    // absolute value 
    List* n = makeNumAbs(p, left);
    List* d = makeNumAbs(p, right);

    // compute interger division for positive values
    List* absresult = makeNumDivPos(p, n, d); 

    // set result sign bit 
    int sign = makeBoolXor(p, signl, signr);    
    List* result = makeNumSign(p, sign, absresult);   
 
    // remove unused lists
    DeleteList(absresult);    
    DeleteList(n);    
    DeleteList(d);    

    return result;

}

// same as makeNumDivPos, except returns remainder instead of quotient
List* makeNumModPos (Problem* p, List* left, List* right) {
    
    /* Basic binary long division - http://en.wikipedia.org/wiki/Division_algorithm 
    
    if D == 0 then throw DivisionByZeroException end
    Q := 0                 initialize quotient and remainder to zero
    R := 0                     
    for i = n-1...0 do     where n is number of bits
      R := R << 1          left-shift R by 1 bit    
      R(0) := N(i)         set the least-significant bit of R equal to bit i of the numerator
      if R >= D then
        R = R - D               
        Q(i) := 1
      end
    end  
    */

    List* n = left;                 // numerator
    List* d = right;                // divisor
    // q=0 and r = 0
    // ensure q and r as long as n
    List* zero = makeNumber(p,0);   
    List* q = extendNumber(p, zero, n->count);        // quotient 
    List* r = extendNumber(p, zero, n->count);        // remaineder 
    DeleteList(zero);
    // init walking elements
    Node* ni = n->first->next;              // n(i); skip sign bit; starting with most significant bit
    Node* qi = q->first->next;              // q(i); skip sign bit; starting with most significant bit
    
    // no solution if d == 0
    List* d0 = makeNumEq(p, d, zero);
    constrainBoolFalse(p, d0->last->value);
    
    //  for each digit of n and q, starting with most significant bit
    while (ni != NULL && qi != NULL) {       
        // r = r << 1
        List* r2 = makeNumShiftL(p, r);
        DeleteList(r);
        r = r2;
        // r(k)=n(i) where k is least significant bit
        r->last->value = ni->value;        
        // if r>= d then r = r - d; else r = r
        List* same = r;
        List* subtract = makeNumSub(p, r, d);    
        List* lt = makeNumLt(p, r, d);      
        List* gte = makeNumNot(p, lt);
        int greater = gte->last->value; 
        List* r3 = makeNumIf(p, greater, subtract, same);
        DeleteList(r);
        r = r3;
        // if r>=d then q(i) = true else false, i.e. q(i)=(r>=d)
        qi->value = greater;
        // next (less significant) bit
        ni = ni->next;
        qi = qi->next;
    }

    DeleteList(q);

    return r;
}


List* makeNumMod_old (Problem* p, List* left, List* right) {

    // get sign bits
    int signl = left->first->value;
    int signr = right->first->value;

    // absolute value 
    List* numerator = makeNumAbs(p, left);
    List* divisor = makeNumAbs(p, right);

    // compute interger modulus for positive values
    List* remainder = makeNumModPos(p, numerator, divisor); 

    // determine modulus from remainder and input signs
    // A:if n > 0 & d > 0, then remainder          (floor modulus)
    // B:if n < 0 & d > 0, then d-1-remainder      (floor modulus)  TODO: doesn't work right; need to shift before modulus!
    // C:if n > 0 & d < 0, then remainder-(d-1)    (?)
    // D:if n < 0 & d < 0, then -remainder         (?)
    List* minone = makeNumber(p, -1);
    List* shift = makeNumAdd(p, divisor, minone);
    List* a = remainder;
    List* b = makeNumSub(p, divisor, remainder);
    List* c = makeNumSub(p,remainder, shift);
    List* d = makeNumNeg(p,remainder); 
    List* ab = makeNumIf(p, -signl, a, b);
    List *cd = makeNumIf(p, -signl, c, d);
    List* modulus = makeNumIf(p, -signr, ab, cd);

    // remove unused lists
    DeleteList(numerator);    
    DeleteList(divisor);    
    DeleteList(remainder);    
    DeleteList(minone);    
    DeleteList(shift);    
    // DeleteList(a);   = remainder   
    DeleteList(b);    
    DeleteList(c);    
    DeleteList(d);    
    DeleteList(ab);    
    DeleteList(cd);    
    return modulus;
}

// note: if division is floor (vs integer) division then this is the floor (vs integer) modulus
//  
List* makeNumMod (Problem* p, List* left, List* right) {
    List* div = makeNumDiv(p, left, right);
    List* muldiv = makeNumMul(p, div, right);
    List* floormod = makeNumSub(p, left, muldiv);
    DeleteList(div);
    DeleteList(muldiv);
    return floormod;
}

// ---------- solving expressions  ----------

List* convertExpr(Expr* expr, Problem* p);  // needs to be forward declare due recursion

List* makeEval(Problem* p, Expr* expr) {
    printf("Solving subproblem for EVAL...");
    if (p == NULL) return NULL;
    if (expr == NULL) return NULL;
    if (expr->left == NULL) return NULL;
    if (expr->right == NULL) return NULL;
 
    // evalutate in a new context
    Problem* problem = newProblem();

    // convert subexpressions in new context
    List* left = convertExpr(expr->left, problem);
    List* right = convertExpr(expr->right, problem);
    if (left == NULL || right == NULL)
    {
	    return NULL;
    } 
           
    // implicitly constrain right hand to 1
    constraintNumConst(problem, right, 1);
    
    // solve
    // add a rule to ensure all variables are include
    addRule2(problem, 0,0, problem->maxlit, 0);
    // convert problem rules to array
    int* rules = List2Array(problem->rules);
    // create solver
    Solver* solver = NewSolver(rules);
    free(rules);
    
    // we dont need this anymore
    // the new expression is created in the in old context!
    deleteProblem(problem);
    
    // result is a new (anonymous) variable
    List* varlits = makeNumVar(p);    
    // if no solutions, no solution for new variable
    List* num0 = makeNumber(p, 0);  
    List* num1 = makeNumber(p, 1);  
    List* eq0 = makeNumEq(p, varlits, num0);
    List* eq1 = makeNumEq(p, varlits, num1);
    List* or = makeNumAnd(p, eq1, eq0);
 
    // evaluate left left side in all solutions
    // assign to literals Or(Eq(var,num1), Eq(var,num2),...)
    int numSol= 0;
    while (NextSolution(solver)) {
        ++numSol;
        if (numSol > MAX_SOLUTIONS) {
            printf("\nWARNING: Subproblem has more than %d solutions. EVAL truncated.\n", MAX_SOLUTIONS);
            break;
        }
        int* array = List2Array(left);
        if (array != NULL) { 
            int num = getNumber(solver, array);
            List* numlits = makeNumber(p, num);  // should use same true/false literals  
            List* eqlits = makeNumEq(p, varlits, numlits);
            List* newor = makeNumOr(p, or, eqlits);
            DeleteList(eqlits);
            DeleteList(numlits);
            DeleteList(or);
            or = newor; 
            free(array);
        }
    }
   
    // exhaused 
    DeleteSolver(solver);

    // constrain the result 
    constraintNumConst(p, or, 1);   

    printf("done.\n");

    return varlits;
}

/* basically copies expr, but all pointers to variables are replaced by a new variable
   determined by the value of the variable in the given solutions 
*/
Expr* replaceIndirections(Expr* expr, Problem* problem, Solver* solver) {
    if (expr == NULL)    
        return expr;
    else {
        Operator op = expr->op;
        Expr* left = expr->left;
        Expr* right = expr->right;
        char* term = expr->term;
        if (op == PNTR) {
            List* varlits = NULL;
            varlits = getVariable(problem, left->term);
            if (varlits != NULL) {
                int* array = List2Array(varlits);
                if (array != NULL) { 
                    int num = getNumber(solver, array);
                    char* numstr = malloc(10);
                    snprintf(numstr,10,"%d",num); 
                    return newExpr(NUM, numstr, NULL, NULL);
                    free(array);
                }
                DeleteList(varlits);
            }
         } 
         else if (op == VAR) {
            List* varlits = NULL;
            varlits = getVariable(problem, term);
            if (varlits != NULL) {
                int* array = List2Array(varlits);
                if (array != NULL) { 
                    int num = getNumber(solver, array);
                    char* varname = makeVarName(num); 
                    return newExpr(VAR, varname, NULL, NULL);
                    free(array);
                }
                DeleteList(varlits);
            }
        } 
        
        // NOTE: if variable not in problem, or not a variable, then just copy  
        Expr* newleft = replaceIndirections(left, problem, solver);
        Expr* newright = replaceIndirections(right, problem, solver);
        return newExpr(op, term, newleft, newright);
    }
}  

/* Indirection solver */
List* makeInd(Problem* problem, Expr* expr) {
    printf("Solving subproblem for IND...");

    if (problem == NULL) return NULL;
    if (expr == NULL) return NULL;
    if (expr->left == NULL) return NULL;
    if (expr->right == NULL) return NULL;
 
    // convert right side sub-problem in new context
    Problem* new = newProblem();
    List* right = convertExpr(expr->right, new);
    if (right == NULL)
    {
	return NULL;
    } 

    // implicitly constrain right hand to 1 (in new context)
    constraintNumConst(new, right, 1);
    
    // solve
    // add a rule to ensure all variables are include
    addRule2(new, 0,0, new->maxlit, 0);
    // convert new rules to array
    int* rules = List2Array(new->rules);
    // create solver
    Solver* solver = NewSolver(rules);
    // we dont need this anymore
    free(rules);

    // replace all variables in left hand by new variables determined by
    // values of those variables in right hand solutions 
    // result is AND of new expressions
    Expr* newexpr = newExpr(NUM, "1", NULL,NULL);
    int numSol = 0;
    while (NextSolution(solver)) {
         ++numSol;
        if (numSol > MAX_SOLUTIONS) {
            printf("\nWARNING: Subproblem has more than %d solutions. IND truncated.\n", MAX_SOLUTIONS);
            break;
        }
        Expr* replacexpr = replaceIndirections(expr->left, new, solver);    // recursive
        newexpr = newExpr(AND, "&(indirection)", newexpr, replacexpr);
    } 
    
    // done solving
    deleteProblem(new);
    DeleteSolver(solver);
   
    printf("New problem:\n");
    printExpr(newexpr,1); 

    // convert LHS expression in input context
    List* newlits = convertExpr(newexpr, problem);
           
    // delete new expression tree 
    // TODO: deleteExpr(newexpr);   NOTE: problem is that Expr.term is only sometimes allocated 
 
    printf("done.\n");

    return newlits;
}


List* makeCount(Problem* p, Expr* expr) {

    printf("Solving subproblem for COUNT...");
    
    if (p == NULL) return NULL;
    if (expr == NULL) return NULL;
    if (expr->left == NULL) return NULL;
    // if (expr->right == NULL) return NULL;
 
    // evalutate in a new context
    Problem* problem = newProblem();

    // convert subexpressions in new context
    List* left = convertExpr(expr->left, problem);
    // List* right = convertExpr(expr->right, problem);
    if (left == NULL)
    {
	return NULL;
    } 


    // implicitly constrain left hand to 1
    constraintNumConst(problem, left, 1);
    
    // solve
    // add a rule to ensure all variables are include
    addRule2(problem, 0,0, problem->maxlit, 0);
    // convert problem rules to array
    int* rules = List2Array(problem->rules);
    // create solver
    Solver* solver = NewSolver(rules);
    free(rules);
    
   
    // count solutions
    int numSolutions = 0;
    while (NextSolution(solver)) {
        numSolutions+=1;
        if (numSolutions > MAX_SOLUTIONS) {
            printf("\nWARNING: Subproblem has more than %d solutions. COUNT truncated.\n", MAX_SOLUTIONS);
            break;
        }
    }

    // exhaused 
    DeleteSolver(solver);

    // we dont need this anymore
    // the new expression is created in the in old context (p)
    deleteProblem(problem);
 
    // constrain the result 
    List* num = makeNumber(p, numSolutions);

    printf("done.\n");
    return num;
}

List* makeMin(Problem* p, Expr* expr) {
    printf("Solving subproblem for MIN...");
    if (p == NULL) return NULL;
    if (expr == NULL) return NULL;
    if (expr->left == NULL) return NULL;
    if (expr->right == NULL) return NULL;
 
    // evalutate in a new context
    Problem* problem = newProblem();

    // convert subexpressions in new context
    List* left = convertExpr(expr->left, problem);
    List* right = convertExpr(expr->right, problem);
    if (left == NULL || right == NULL)
    {
	return NULL;
    } 

    // implicitly constrain right hand to 1
    constraintNumConst(problem, right, 1);
    
    // create order variable list
    List* ordered = NewList();
    // convert to postive value, by adding maximum integer of same length.
    // needed because 2's complement negetive value not in sort order
    List* maxint = NewList();
    ListAppend(maxint, problem->false);       // maxint sign bit false
    Node* node = left->first;
    while (node) {
        ListAppend(maxint, problem->true);        // maxint all other bits true
        node = node -> next;
    } 
    List* positive = makeNumAdd(problem, left, maxint);
    // add variables(not literals!)to ordered vars list
    node = positive->first;
    while (node) {
        ListAppend(ordered, literal2var(problem, node->value));
        node = node -> next;
    }

    // add a rule to ensure all variables are include
    addRule2(problem, 0,0, problem->maxlit, 0);
    // convert problem rules to array
    int* rules = List2Array(problem->rules);

    // create solver with ordered variables
    Solver* solver = NewSolver2(rules,ordered);
    
    // we dont need this anymore (local context)
    free(rules);
    DeleteList(ordered);
    DeleteList(maxint);
    DeleteList(positive); 
    deleteProblem(problem);
    
    // solve and evaluate left left side in first solution!
    int minValue = 0;
    int numSolutions = 0;
    if (NextSolution(solver)) {
        int* array = List2Array(left);
        if (array != NULL) { 
            minValue = getNumber(solver, array);
            free(array);
            numSolutions++;
        }
    }
    
    // not needed anymore 
    DeleteSolver(solver);

    // construct result variables, in global context 
    List* literals;
    if (numSolutions == 0) {
        // create a conflict
        literals = NewList();
        int lit = newLit(p);  
        constrainBoolTrue(p,lit);
        constrainBoolFalse(p,lit);
        ListAppend(literals, lit);
    } 
    else
    {
        literals = makeNumber(p, minValue);
    }
    
    printf("done.\n");
    
    return literals;
}

List* makeMax(Problem* p, Expr* expr) {
    printf("Solving subproblem for MAX...");
    if (p == NULL) return NULL;
    if (expr == NULL) return NULL;
    if (expr->left == NULL) return NULL;
    if (expr->right == NULL) return NULL;
 
    // evalutate in a new context
    Problem* problem = newProblem();

    // convert subexpressions in new context
    List* left = convertExpr(expr->left, problem);
    List* right = convertExpr(expr->right, problem);
    if (left == NULL || right == NULL)
    {
	return NULL;
    } 



    // implcitlyy constrain right hand to 1
    constraintNumConst(problem, right, 1);
    
    // create order variable list
    List* ordered = NewList();
    // convert to postive value, by subtracting value from maximum integer of same length.
    // needed because 2's complement negetive value not in sort order
    List* maxint = NewList();
    ListAppend(maxint, problem->false);       // maxint sign bit false
    ListAppend(maxint, problem->true);       // extra bit, because neg range bigger than pos range for 2's comp
    Node* node = left->first;
    while (node) {
        ListAppend(maxint, problem->true);        // maxint all other bits true
        node = node -> next;
    } 
    List* positive = makeNumSub(problem, maxint, left);
    // add variables(not literals!)to ordered vars list
    node = positive->first;
    while (node) {
        ListAppend(ordered, literal2var(problem, node->value));
        node = node -> next;
    }

    // add a rule to ensure all variables are include
    addRule2(problem, 0,0, problem->maxlit, 0);
    // convert problem rules to array
    int* rules = List2Array(problem->rules);

    // create solver with ordered variables
    Solver* solver = NewSolver2(rules,ordered);
    
    // we dont need this anymore (local context)
    free(rules);
    DeleteList(ordered);
    DeleteList(maxint);
    DeleteList(positive); 
    deleteProblem(problem);
    
    // solve and evaluate left left side in first solution!
    int maxValue = 0;
    int numSolutions = 0;
    if (NextSolution(solver)) {
        int* array = List2Array(left);
        if (array != NULL) { 
            maxValue = getNumber(solver, array);
            numSolutions++;
            free(array);
        }
    }
    
    // not needed anymore 
    DeleteSolver(solver);

    // construct result variables, in global context 
    List* literals;
    if (numSolutions == 0) {
        // create a conflict
        literals = NewList();
        int lit = newLit(p);  
        constrainBoolTrue(p,lit);
        constrainBoolFalse(p,lit);
        ListAppend(literals, lit);
    } 
    else
    {
        literals = makeNumber(p, maxValue);
    }
    
    printf("done.\n");
    
    return literals;
}



// ------------ convert expression - 
// adds constraints to problem and return a list of variables 

List* convertExpr(Expr* expr, Problem* p) {
    if (expr == NULL) return NULL;
    List* left = NULL; 
    List* right = NULL; 
    List* literals = NULL;

    // for PNTR 
    Expr* lhs;
    int varno; 
        
    switch (expr->op) {
        // var and num have no subexpressions
        case VAR: 
            literals = addOrGetVariable(p, expr->term);
            break;
        case NUM:
            literals = makeNumber(p,atoi(expr->term));
            break;

        // unary math expressions
        case NOT:
            left = convertExpr(expr->left, p);
	        if (left == NULL)
	        {
		        return NULL;
	        } 
            literals = makeNumNot(p,left);
            break; 
        
        case NEG:
            left = convertExpr(expr->left, p);
            if (left == NULL)
	        {
		        return NULL;
	        } 
	        literals = makeNumNeg(p,left);
            break; 
        
        // the only constraint: True 
        // CON is unary 
        case CON:
            left = convertExpr(expr->left, p);
	        if (left == NULL)
	        {
		        return NULL;
	        } 
            constraintNumConst(p, left, 1);
            literals = copyList(left);
            break;
	
	    // pointer is just a variable
        // PNTR is unary - lhs is not evaluated but must be a number 
	    case PNTR:
	        lhs = (Expr*)expr->left;
	        varno = atoi(lhs->term);
	        literals = addOrGetVariable(p, makeVarName(varno));
            break;

        // the evaluation operators convert subexpressions as needed
        // possibly in a different context (new Problem instance)
        case EVAL:
            literals = makeEval(p, expr);
            break;
        case IND:
            literals = makeInd(p, expr);
            break;
        case MIN: 
            literals = makeMin(p, expr);
            break;
        case MAX: 
            literals = makeMax(p, expr);
            break;
        case COUNT: 
            literals = makeCount(p, expr);
            break;
        default:
            break;
    }

    // the mathematical operators all have two subexpressions, evaluated in the same context
    switch (expr->op) {
        case EQ:
        case IMPL:
        case AND:
        case OR:
        case XOR:
        case ADD:
        case SUB:
        case MUL:
        case DIV:
        case MOD:
        case LT:
        case GT:
        left = convertExpr(expr->left, p);
        right = convertExpr(expr->right, p);
    	if (left == NULL || right ==NULL) 
	    {
	        return NULL;
	    }
        default:
            break;    
    }   
 
    switch (expr->op) {
       case EQ:
            literals = makeNumEq(p, left, right);
            break;
        case IMPL:
            literals = makeNumImpl(p,left, right);
            break;
        case AND:
            literals = makeNumAnd(p,left, right);
            break;
        case OR:
            literals = makeNumOr(p,left, right);
            break;
        case XOR:
            literals = makeNumXor(p,left, right);
            break;
        case ADD:
            literals = makeNumAdd(p,left, right);
            break;
        case SUB:       // depricated!
            literals = makeNumSub(p,left, right);
            break;
        case MUL:
            literals = makeNumMul(p,left, right);
            break;
        case DIV:
            literals = makeNumDiv(p,left,right);
            break;
        case MOD:
            literals = makeNumMod(p,left,right);
            break;
        case LT:
            literals = makeNumLt(p,left,right);
            break;
        case GT:
            literals = makeNumLt(p,right,left);
            break;
        default:
            break;
    }

    // cleanup
    if (left != NULL) DeleteList(left);
    if (right != NULL) DeleteList(right);
    
    return literals;
}

// experimental
Problem* Simplify(Problem* problem, List* expression) {

    // convert problem rules to array
    int* rules = List2Array(problem->rules);
    // create solver
    Solver* s = NewSolver(rules);
    free(rules);

    printf("Simplify...\n");
    
     // print some statistics
    printf("%d variables\n", s->gNumberOfVariables);
    printf("%d rules\n", s->gNumberOfRules);
    
    // mark time
    struct timeval tv1;
    gettimeofday(&tv1,NULL);

    // ---- snip taken from NextSolution ----
    // basically perform one forward propagation step, but make no choice.
    
    // propagate all literals for which the antagonist cannot be propagated
    int lastcount = -1;
    int i = 0;
    int backtrack = 0;
    while (s->gFreeVars->count != lastcount) { 
        lastcount = s->gFreeVars->count;
        i++;
        DEBUGPRINTF("Iter %d, color=%d freevars=%d\n", i, s->gColor,s->gFreeVars->count);
    
        // unfortunately need to copy because order of s->gFreeVars can change during (Un)Propagate
        // NOTE: A simply linked list stack or queue would be suffient too!
        // Or maybe we can change the order in which variables are pushed onto s->gFreeVars?

        Set* freevarscopy = NewSet(s->gNumberOfVariables+1);
        Node* freevar = s->gFreeVars->first;
        while (freevar) {
            AddSet(freevarscopy, freevar->value, FALSE);
            freevar = freevar -> next;
        }

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

        // conflict - no solutions, what should we return?    
        if (backtrack == 1) break;
    }  // while 

    // ------------- end snippit -----------
    
    // -------- create new problem
    Problem* newpro = newProblem();
    
    // no solution  -- return empty problem
    if (backtrack == 1) {
        // add conflict to problem
        addRule2(newpro, newpro->false, 0, newpro->true, 0);
        addRule2(newpro, newpro->true, 0, newpro->false, 0);
        return newpro;
     };   


    // allocate space for variable mapping
    int* map = (int*)malloc((problem->maxlit+1) * sizeof(int));
    // clear it
    int var;
    for (var=0;var<=problem->maxlit;var++) {
        map[var] = 0;
    }

   // ------- keep variables in new problem
    int numbervar;
    for (numbervar=0;numbervar<MAX_VARS;numbervar++) {
        int lit = problem->var2lit[numbervar];
        if (lit != 0) {
            int i = 0;
            for (i=0;i<NUMBER_WIDTH;i++) {
                map[lit+i] = newLit(newpro);
                //printf("map %d -> var %d\n", lit+i,map[lit+i]);
            }
            newpro->var2lit[numbervar] = map[lit];
        }
    }
    
    // for each unmapped var, if not free, then map to fixed true/false
    for (var=1;var<=problem->maxlit;var++) {
        if (map[var] == 0) {
            int istrue = s->gMarkers[Literal2Index(var)]!=0;
            int isfalse = s->gMarkers[Literal2Index(-var)]!=0;
            if (istrue && isfalse) {
                printf("error: variable %d is true and false!\n", var);
            }
            else if (istrue) {
                map[var] = newpro->true;
            }
            else if (isfalse) {
                map[var] = newpro->false;
            }
            else {
                map[var] = newLit(newpro);
            }
        }
    }

    // --------- map expression (in place) ------
    Node* node = expression->first;
    while (node != NULL) {
        int lit = node->value;
        if (lit >= 0)
            lit = map[lit];
        else
            lit = -map[-lit];
        ASSERT(node->value == 0 || lit != 0);
        node->value = lit;
        node = node->next;
    }

     // -------- map rules of old problem to new problem ---- 
    node = problem->rules->first;
    List* newrules = NewList();
    while (node != NULL) {
        int lit = node->value;
        if (lit >= 0)
            lit = map[lit];
        else
            lit = -map[-lit];
        ASSERT(node->value == 0 || lit != 0);
        ListAppend(newrules, lit);
        //ListAppend(newpro->rules, lit);
        node = node->next;
    }

    // remove rules that are redundant 
    // and remove duplicate literals in lhs (and rhs) of rules (unsupported by solver, rule counters will be wrong!)

    node = newrules->first;
    Set* lhs = NewSet(Literal2Index(newpro->maxlit+1));
    Set* rhs = NewSet(Literal2Index(newpro->maxlit+1));
    while (node != NULL) {
        // parse lhs
        while (node != NULL) {
            int value = node->value;
            node = node -> next;
            if (value == 0) break;          //. end of lhs
            if (value != problem->true)      // ignore TRUE
                AddSet(lhs, Literal2Index(value), 0);   
        }
        // parse rhs
        while (node != NULL) {
            int value = node->value;
            node = node -> next;
            if (value == 0) break;          // end of rhs
            if (value != problem->true)     // ignore TRUE  
                AddSet(rhs, Literal2Index(value), 0);   
        }

        // simplify rules: skip rule if lhs contains false
        int skip = 0;
        if (GetSetNode(lhs, Literal2Index(newpro->false)) != NULL) skip = 1;
        if (lhs->count == 0) skip = 1;
        if (rhs->count == 0) skip = 1;
        //if (rhs->count == 0 && lhs->count == 0) skip = 1;

        // rule to newproblem if not skipped
        if (skip == 0) {
            // write lhs to newproblem
            Node* setnode = lhs->first;
            while (setnode != NULL) {
                ListAppend(newpro->rules, Index2Literal(setnode->value));
                setnode = setnode->next;
            }
            ListAppend(newpro->rules, 0);      // end lhs
            // write rhs to newproblem
            setnode = rhs->first;
            while (setnode != NULL) {
                ListAppend(newpro->rules, Index2Literal(setnode->value));
                setnode = setnode->next;
            }
            ListAppend(newpro->rules, 0);      // end rhs
        }
        else {
        }
        // clear lhs and rhs sets 
        Node* setnode = lhs->first;
        while (setnode != NULL) {
            RemSet(lhs, setnode->value); 
            setnode = lhs->first;
        }
        ASSERT(lhs->count==0);
        setnode = rhs->first;
        while (setnode != NULL) {
            RemSet(rhs, setnode->value); 
            setnode = rhs->first;
        }
        ASSERT(rhs->count==0);
    } 
    
    // remove temp rules
    DeleteSet(lhs);
    DeleteSet(rhs);
    DeleteList(newrules);
    free(map);
    
     // mark time
    struct timeval tv2;
    gettimeofday(&tv2,NULL);

    // compute time in seconds
    double t1 = (double)tv1.tv_sec + ((double)tv1.tv_usec) * 1e-6;
    double t2 = (double)tv2.tv_sec + ((double)tv2.tv_usec) * 1e-6;

    // more statistics
    printf("%d propagations\n", s->gNumProp);
    printf("%f seconds\n", t2-t1);

    return newpro;
    
} // Simplify

    
// -------------------------- main -------------------

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

    while (!feof(input)) { 
    
        Problem* problem = newProblem();
        
        // parse expression
        Expr* expr = parseExpr(input);
        if (expr == NULL) {
            printf("End of Input.\n");
            deleteProblem(problem);
            continue;
        }
    
        // if expr is numerical, impicityly prepend CON=?1
        // else if expr is boolean, impicityly prepend CON
            switch (expr->op) {
                case VAR: 
                case PNTR: 
                case NUM:
                case EVAL:
                case MIN: 
                case MAX: 
                case COUNT: 
                case NEG:
                case ADD:
                case SUB:
                case MUL:
                case DIV:
                case MOD:
                    expr = newExpr(CON, "@", newExpr(EQ, "=", newExpr(VAR,"?1",NULL,NULL),expr),NULL);        
                    break;

                case NOT:
                case IND:
                case EQ:
                case IMPL:
                case AND:
                case OR:
                case XOR:
                case LT:
                case GT:
                    expr = newExpr(CON, "@",expr,NULL);        
                    break;
                
                case CON:
                    break;
        }   

        // inform user of interpretation
        printExpr(expr,1);
     
         // create boolean problem
        List* literals = convertExpr(expr, problem);
        if (literals == NULL) {
            deleteProblem(problem);
            printf("Error: Invalid expression\n");
            continue;
        }
     
        // ---- simplify problem
         
        // mark time
        struct timeval tv0;
        gettimeofday(&tv0,NULL);

       
        // add a rule to ensure all variables are included
        addRule2(problem, 0,0, problem->maxlit, 0);

        // simplify and substitute old problem
        // note: litrerals is changed in place!
        Problem* newProblem = Simplify(problem, literals);
        deleteProblem(problem);
        problem = newProblem;
            
        // ------- solve problem  -----
 
        // mark time
        struct timeval tv1;
        gettimeofday(&tv1,NULL);

       
        // add a rule to ensure all variables are include
        addRule2(problem, 0,0, problem->maxlit, 0);

        // convert problem rules to array
        int* rules = List2Array(problem->rules);
        // create solver
        Solver* solver = NewSolver(rules);
        free(rules);

        // print some statistics
        printf("Solving...\n");
        printf("%d variables\n", solver->gNumberOfVariables);
        printf("%d rules\n", solver->gNumberOfRules);
        
        // print solutions
        int numSol = 0;
        while (NextSolution(solver)) {
            ++numSol;
            if (numSol > MAX_SOLUTIONS) {
                printf("WARNING: Problem has more than %d solutions. Result truncated.\n", MAX_SOLUTIONS);
                break;
            }
            printf("Solution #%d:\n",numSol);
            // print all variables
            int var;
            for (var=1;var<MAX_VARS;var++) {
                char* varname = makeVarName(var); 
                List* varlits = getVariable(problem, varname); 
                if (varlits != NULL) {
                    int* array = List2Array(varlits);
                    if (array != NULL) { 
                        int num = getNumber(solver, array);
                        printf(" %s=%d\n", varname, num);
                        free(array);
                    }
                    DeleteList(varlits);
                }
                free(varname);
            }
            /*
            // answer to expresion
            int* array = List2Array(literals);
            if (array != NULL) { 
                int num = getNumber(solver, array);
                printf(" %s=%d\n", "ans", num);
                free(array);
            }
            */
        }
        

      

        // mark time
        struct timeval tv2;
        gettimeofday(&tv2,NULL);

        // compute time in seconds
        double t0 = (double)tv0.tv_sec + ((double)tv0.tv_usec) * 1e-6;
        double t1 = (double)tv1.tv_sec + ((double)tv1.tv_usec) * 1e-6;
        double t2 = (double)tv2.tv_sec + ((double)tv2.tv_usec) * 1e-6;

        // more statistics
        printf("%d solutions\n",numSol);
        printf("%d propagations\n", solver->gNumProp);
        printf("%d choices\n", solver->gNumChoice);
        printf("simplify %f seconds\n", t1-t0);
        printf("solve %f seconds\n", t2-t1);
        printf("total %f seconds\n", t2-t0);
   
        // clean up
        DeleteSolver(solver);
        DeleteList(literals); 
        deleteProblem(problem);
    }   // while !eof
    return 0;
} // main

