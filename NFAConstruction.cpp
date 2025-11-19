#include "NFAConstruction.h"

int NFAConstruction::stateCounter = 0;

NFAFragment* NFAConstruction::createBasic(char c) {
    State* s1 = new State(getNextStateID());
    State* s2 = new State(getNextStateID());
    s1->addTransition(c, s2);
    return new NFAFragment(s1, s2);
}

NFAFragment* NFAConstruction::createEpsilon() {
    return createBasic(EPSILON);
}

NFAFragment* NFAConstruction::concatenate(NFAFragment* first, NFAFragment* second) {
    // Connect End of First -> Start of Second via Epsilon
    first->end->addTransition(EPSILON, second->start);
    // Result is Start of First -> End of Second
    NFAFragment* result = new NFAFragment(first->start, second->end);
    
    // Clean up the old fragment pointers (not the states themselves)
    delete first;
    delete second;
    return result;
}

NFAFragment* NFAConstruction::join(NFAFragment* top, NFAFragment* bottom) {
    State* newStart = new State(getNextStateID());
    State* newEnd = new State(getNextStateID());

    // Split from new start
    newStart->addTransition(EPSILON, top->start);
    newStart->addTransition(EPSILON, bottom->start);

    // Join to new end
    top->end->addTransition(EPSILON, newEnd);
    bottom->end->addTransition(EPSILON, newEnd);

    NFAFragment* result = new NFAFragment(newStart, newEnd);
    
    delete top;
    delete bottom;
    return result;
}

NFAFragment* NFAConstruction::kleeneStar(NFAFragment* nfa) {
    State* newStart = new State(getNextStateID());
    State* newEnd = new State(getNextStateID());

    newStart->addTransition(EPSILON, nfa->start); // Enter
    newStart->addTransition(EPSILON, newEnd);     // Skip (0 times)
    
    nfa->end->addTransition(EPSILON, nfa->start); // Loop
    nfa->end->addTransition(EPSILON, newEnd);     // Exit

    NFAFragment* result = new NFAFragment(newStart, newEnd);
    delete nfa;
    return result;
}

NFAFragment* NFAConstruction::positiveClosure(NFAFragment* nfa) {
    // a+ is effectively a . a*
    // But typically implemented as:
    State* newStart = new State(getNextStateID());
    State* newEnd = new State(getNextStateID());

    newStart->addTransition(EPSILON, nfa->start);
    nfa->end->addTransition(EPSILON, nfa->start); // Loop
    nfa->end->addTransition(EPSILON, newEnd);     // Exit

    NFAFragment* result = new NFAFragment(newStart, newEnd);
    delete nfa;
    return result;
}