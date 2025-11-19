#ifndef NFA_H
#define NFA_H

#include <vector>
#include <string>
#include <unordered_set>

using namespace std;

// Standard epsilon definitions
const char EPSILON = '\0'; 

// Forward declaration
struct State; 

struct Transition {
    char input;         // The character trigger (or EPSILON)
    State* nextState;   // Pointer to destination

    Transition(char ch, State* next) : input(ch), nextState(next) {}
};

struct State {
    int id;                     // Unique ID for debugging
    bool isAccepting;           // True if this is a final state
    string tokenClass;     // E.g., "id", "num", "while" (Only for accepting states)
    int priority;               // 1 = Highest. Used for tie-breaking rules.
    
    vector<Transition> transitions; 

    State(int id_val) : id(id_val), isAccepting(false), priority(__INT_MAX__) {}

    void addTransition(char input, State* next) {
        transitions.push_back(Transition(input, next));
    }
};

// A wrapper to hold a sub-graph (Start -> ... -> End)
struct NFAFragment {
    State* start;
    State* end;

    NFAFragment(State* s, State* e) : start(s), end(e) {}
};

#endif