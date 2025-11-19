#ifndef DFA_H
#define DFA_H

#include <set>
#include <map>
#include <climits>
#include <vector>
#include <string>
#include "NFA.h"

using namespace std;

struct DFAState {
    int id;
    std::set<State*> nfaSubset;
    bool isAccepting = false;
    std::string tokenClass;
    int priority = INT_MAX;
    std::map<char,int> transitions;
};

struct DFA {
    std::vector<DFAState> states;
    int startState = -1;
};
#endif