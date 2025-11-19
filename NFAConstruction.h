#ifndef NFACONSTRUCTION_H
#define NFACONSTRUCTION_H

#include "NFA.h"

class NFAConstruction {
public:
    static int stateCounter;

public:
    // Basic Building Blocks
    static NFAFragment* createBasic(char c);
    static NFAFragment* createEpsilon(); // For \L
    
    // Operators
    static NFAFragment* concatenate(NFAFragment* first, NFAFragment* second);
    static NFAFragment* join(NFAFragment* top, NFAFragment* bottom); // Union / OR (|)
    static NFAFragment* kleeneStar(NFAFragment* nfa); // Zero or more (*)
    static NFAFragment* positiveClosure(NFAFragment* nfa); // One or more (+)
    
    // Helpers
    static int getNextStateID() { return stateCounter++; }
};

#endif