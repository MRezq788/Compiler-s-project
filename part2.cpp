#include <vector>
#include <string>
#include <unordered_map>
#include <set>
#include <map>
#include <queue>
#include <stack>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include <climits>
#include "NFA.h"
#include "DFA.h"

using StatePtr = State*;
using StateSet = std::set<StatePtr>;

// Produce stable key for a set of NFA states by their integer ids
static std::string stateSetKey(const StateSet& sset) {
    std::vector<int> ids;
    ids.reserve(sset.size());
    for (auto p : sset)
        ids.push_back(p->id);
    std::sort(ids.begin(), ids.end());
    std::ostringstream oss;
    bool first = true;
    for (int x : ids) {
        if (!first) oss << ",";
        first = false;
        oss << x;
    }
    return oss.str();
}


/*Start from every state in the input set, follow every EPSILON transition recursively,
return the set containing the original states plus any reachable by only epsilon steps*/
StateSet epsilonClosure(const StateSet& states) {
    StateSet closure = states;
    std::stack<StatePtr> st;
    for (StatePtr s : states) 
        st.push(s);
    while (!st.empty()) {
        StatePtr cur = st.top(); 
        st.pop();
        for (const Transition &t : cur->transitions) {
            if (t.input == EPSILON) {
                StatePtr nxt = t.nextState;
                if (closure.insert(nxt).second) {
                    st.push(nxt);
                }
            }
        }
    }
    return closure;
}

/*Returns the set of NFA states that are directly reachable from any state in states via
a transition labeled with the character ch*/
StateSet moveOnChar(const StateSet& states, char ch) {
    StateSet res;
    for (StatePtr s : states) {
        for (const Transition &t : s->transitions) {
            if (t.input == ch)
                res.insert(t.nextState);
        }
    }
    return res;
}

// Convert the given NFA to an equivalent DFA using the subset construction algorithm
DFA convertNFAtoDFA(State* nfaStart, const std::vector<char>& alphabet) {
    DFA dfa;
    std::map<std::string, int> subsetToDfaId;

    StateSet startSet; 
    startSet.insert(nfaStart);
    StateSet startClosure = epsilonClosure(startSet);

    auto createDfaStateFromSubset = [&](const StateSet& subset) -> int {
        std::string key = stateSetKey(subset);
        auto it = subsetToDfaId.find(key);
        if (it != subsetToDfaId.end())
            return it->second;
        
        int newId = (int)dfa.states.size();
        DFAState ds;
        ds.id = newId;
        ds.nfaSubset = subset;

        ds.isAccepting = false;
        ds.priority = INT_MAX;
        for (StatePtr s : subset) {
            if (s->isAccepting) {
                ds.isAccepting = true;
                if (s->priority >= 0 && s->priority < ds.priority) {
                    ds.priority = s->priority;
                    ds.tokenClass = s->tokenClass;
                } else if (ds.priority == INT_MAX && s->priority < 0) {
                    ds.tokenClass = s->tokenClass;
                }
            }
        }

        dfa.states.push_back(ds);
        subsetToDfaId[key] = newId;
        return newId;
    };

    int startId = createDfaStateFromSubset(startClosure);
    dfa.startState = startId;
    std::queue<int> work;
    work.push(startId);

    while (!work.empty()) {
        int curId = work.front();
        work.pop();

        StateSet currentNfaSubset = dfa.states[curId].nfaSubset;
        for (char c : alphabet) {
            if (c == EPSILON) 
                continue;
            StateSet moved = moveOnChar(currentNfaSubset, c);
            if (moved.empty())
                continue;
            StateSet cl = epsilonClosure(moved);
            
            // Check if state already exists
            std::string key = stateSetKey(cl);
            bool isNewState = (subsetToDfaId.find(key) == subsetToDfaId.end());            
            int tgtId = createDfaStateFromSubset(cl);
            dfa.states[curId].transitions[c] = tgtId;
            
            if (isNewState) {
                work.push(tgtId);
            }
        }
    }

    return dfa;
}

struct Partition {
    std::set<int> members;
};

DFA minimizeDFA(const DFA& dfa, const std::vector<char>& alphabet) {
    // Initial partitioning:
    // - Create one partition for non-accepting states (if any)
    // - Create one partition per accepting tokenClass (so different token types won't be merged)
    std::vector<Partition> partitions;
    std::map<std::string,int> acceptClassToPart;

    Partition nonAcceptPart;
    for (const DFAState &s : dfa.states) {
        if (!s.isAccepting) nonAcceptPart.members.insert(s.id);
    }
    if (!nonAcceptPart.members.empty()) partitions.push_back(nonAcceptPart);

    for (const DFAState &s : dfa.states) {
        if (s.isAccepting) {
            const std::string &cls = s.tokenClass;
            auto it = acceptClassToPart.find(cls);
            if (it == acceptClassToPart.end()) {
                Partition p;
                p.members.insert(s.id);
                int idx = (int)partitions.size();
                partitions.push_back(p);
                acceptClassToPart[cls] = idx;
            } else {
                partitions[it->second].members.insert(s.id);
            }
        }
    }

    // Build state -> partition mapping
    auto buildStateToPart = [&](const std::vector<Partition>& parts) {
        std::vector<int> stateToPart(dfa.states.size(), -1);
        for (size_t pi = 0; pi < parts.size(); ++pi) {
            for (int s : parts[pi].members) stateToPart[s] = (int)pi;
        }
        return stateToPart;
    };

    // Iteratively refine partitions until stable
    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<Partition> newPartitions;

        // current mapping to speed signature calculation
        std::vector<int> stateToPart = buildStateToPart(partitions);

        // For each existing partition, split it into groups of same transition-signature
        for (const Partition &part : partitions) {
            if (part.members.empty()) continue;

            // Map from signature -> set of states
            // Signature will be a string built from destination partition indices for each alphabet symbol
            std::map<std::string, std::set<int>> groups;

            for (int s : part.members) {
                std::ostringstream sig;
                // For every symbol in alphabet (skip EPSILON)
                for (char c : alphabet) {
                    if (c == EPSILON) continue;
                    auto it = dfa.states[s].transitions.find(c);
                    if (it == dfa.states[s].transitions.end()) {
                        sig << "#" << ","; // -1 marker for no transition
                    } else {
                        int dest = it->second;
                        int destPart = (dest >= 0 && dest < (int)stateToPart.size()) ? stateToPart[dest] : -1;
                        sig << destPart << ",";
                    }
                }
                // Also include whether state itself is accepting and its accept tokenClass
                // so that states with same transition behavior but different acceptance types/symbols do not merge
                sig << (dfa.states[s].isAccepting ? "A" : "N") << ",";
                if (dfa.states[s].isAccepting) sig << dfa.states[s].tokenClass << ",";
                std::string key = sig.str();
                groups[key].insert(s);
            }

            // If partition splits into more than one group, we mark change
            if (groups.size() == 1) {
                // unchanged, keep the original partition
                newPartitions.push_back(part);
            } else {
                changed = true;
                for (auto &entry : groups) {
                    Partition p;
                    p.members = std::move(entry.second);
                    newPartitions.push_back(std::move(p));
                }
            }
        }

        partitions.swap(newPartitions);
    }

    // Build final mapping from old DFA state -> partition id
    std::vector<int> stateToPart = buildStateToPart(partitions);

    // Construct minimized DFA
    DFA md;
    md.startState = (dfa.startState >= 0 && dfa.startState < (int)stateToPart.size()) ? stateToPart[dfa.startState] : -1;
    int P = (int)partitions.size();
    md.states.resize(P);

    for (int p = 0; p < P; ++p) {
        md.states[p].id = p;
        // pick a representative: choose the accepting state with smallest priority if any, otherwise any state
        int rep = *partitions[p].members.begin();
        // prefer an accepting state with smallest priority
        int bestRep = rep;
        int bestPriority = INT_MAX;
        for (int s : partitions[p].members) {
            if (dfa.states[s].isAccepting) {
                if (dfa.states[s].priority < bestPriority) {
                    bestPriority = dfa.states[s].priority;
                    bestRep = s;
                }
            } else if (bestPriority == INT_MAX) {
                // pick a non-accepting candidate only if no accepting chosen yet
                bestRep = s;
            }
        }
        rep = bestRep;

        const DFAState &repOld = dfa.states[rep];
        md.states[p].isAccepting = repOld.isAccepting;
        md.states[p].tokenClass = repOld.tokenClass;
        md.states[p].priority = repOld.priority;
    }

    // Fill transitions for minimized states: use any member's transitions and remap destinations via stateToPart
    for (int p = 0; p < P; ++p) {
        // choose a representative state from the partition to copy transitions
        int rep = *partitions[p].members.begin();
        const DFAState &repOld = dfa.states[rep];
        for (auto &tr : repOld.transitions) {
            char c = tr.first;
            if (c == EPSILON) continue;
            int destOld = tr.second;
            int destPart = (destOld >= 0 && destOld < (int)stateToPart.size()) ? stateToPart[destOld] : -1;
            if (destPart >= 0) md.states[p].transitions[c] = destPart;
        }
    }

    return md;
}

void printTransitionTable(const DFA& md, const std::vector<char>& alphabet) {
    std::cout << "Minimized DFA Transition Table\n";
    std::cout << "States: " << md.states.size() << "\n";
    std::cout << "Start state: " << md.startState << "\n\n";

    std::cout << "State";
    for (char c : alphabet) {
        if (c == EPSILON) continue;
        std::cout << "\t'" << ( (c=='\t') ? "\\t" : std::string(1,c) ) << "'";
    }
    std::cout << "\tAccepting\tTokenClass\n";

    for (const DFAState &s : md.states) {
        std::cout << s.id;
        for (char c : alphabet) {
            if (c == EPSILON) continue;
            auto it = s.transitions.find(c);
            if (it != s.transitions.end()) std::cout << "\t" << it->second;
            else std::cout << "\t-";
        }
        std::cout << "\t" << (s.isAccepting ? "Yes" : "No");
        std::cout << "\t" << (s.isAccepting ? s.tokenClass : "-") << "\n";
    }
    std::cout << std::flush;
}

// Replace your existing main() with this Option 1 main.
void addRangeTransitions(State* from, State* to, char a, char b) {
    for (char c = a; c <= b; ++c) from->addTransition(c, to);
}

// int main() {
//     std::cout << "Option 1: Building FULL realistic NFA (letters a-z A-Z, digits 0-9)...\n";

//     // Master start (combined NFA start)
//     State* masterStart = new State(10000); // big id to avoid conflicts with example states in file
//     std::vector<State*> allocated;

//     auto makeState = [&](int id)->State* {
//         State* s = new State(id);
//         allocated.push_back(s);
//         return s;
//     };

//     int idCounter = 0;
//     auto newId = [&](){ return idCounter++; };

//     // helper to create simple accepting state with tokenClass and priority
//     auto makeAccept = [&](const std::string &tok, int pr)->State* {
//         State* s = makeState(newId());
//         s->isAccepting = true;
//         s->tokenClass = tok;
//         s->priority = pr;
//         return s;
//     };

//     // ------------------ Build character class helper states ------------------
//     // We'll build small fragments for each token and then connect via epsilon from masterStart.

//     // 1) id: le (le|di)+   where le = a-z | A-Z, di = 0-9
//     {
//         State* s0 = makeState(newId()); // start for id fragment
//         masterStart->addTransition(EPSILON, s0);
//         // first char must be letter
//         State* s_letter = makeState(newId());
//         addRangeTransitions(s0, s_letter, 'a', 'z');
//         addRangeTransitions(s0, s_letter, 'A', 'Z');
//         // then at least one (letter|digit), then loop on letter|digit (so use chain one then loop)
//         State* s_mid = makeState(newId());
//         addRangeTransitions(s_letter, s_mid, 'a', 'z');
//         addRangeTransitions(s_letter, s_mid, 'A', 'Z');
//         addRangeTransitions(s_letter, s_mid, '0', '9');

//         // loop back from s_mid to itself on letters and digits
//         addRangeTransitions(s_mid, s_mid, 'a', 'z');
//         addRangeTransitions(s_mid, s_mid, 'A', 'Z');
//         addRangeTransitions(s_mid, s_mid, '0', '9');

//         State* accept = makeAccept("id", 10);
//         // s_mid -> accept via epsilon (we treat accept as end of token)
//         s_mid->addTransition(EPSILON, accept);
//     }

//     // 2) intconst: di+
//     {
//         State* s0 = makeState(newId());
//         masterStart->addTransition(EPSILON, s0);
//         State* s1 = makeState(newId());
//         addRangeTransitions(s0, s1, '0', '9'); // one digit
//         addRangeTransitions(s1, s1, '0', '9'); // loop digits
//         State* accept = makeAccept("intconst", 20);
//         s1->addTransition(EPSILON, accept);
//     }

//     // 3) bin : b (0|1)+
//     {
//         State* s0 = makeState(newId()); masterStart->addTransition(EPSILON, s0);
//         State* s1 = makeState(newId());
//         s0->addTransition('b', s1);
//         State* s2 = makeState(newId());
//         s1->addTransition('0', s2);
//         s1->addTransition('1', s2);
//         s2->addTransition('0', s2);
//         s2->addTransition('1', s2);
//         State* accept = makeAccept("bin", 5);
//         s2->addTransition(EPSILON, accept);
//     }

//     // 4) oct : o (0..7)+
//     {
//         State* s0 = makeState(newId()); masterStart->addTransition(EPSILON, s0);
//         State* s1 = makeState(newId());
//         s0->addTransition('o', s1);
//         State* s2 = makeState(newId());
//         for (char c='0'; c<='7'; ++c) s1->addTransition(c, s2);
//         for (char c='0'; c<='7'; ++c) s2->addTransition(c, s2);
//         State* accept = makeAccept("oct", 6);
//         s2->addTransition(EPSILON, accept);
//     }

//     // 5) hex : h (di | A-F)+
//     {
//         State* s0 = makeState(newId()); masterStart->addTransition(EPSILON, s0);
//         State* s1 = makeState(newId());
//         s0->addTransition('h', s1);
//         State* s2 = makeState(newId());
//         addRangeTransitions(s1, s2, '0', '9');
//         addRangeTransitions(s1, s2, 'A', 'F');
//         addRangeTransitions(s2, s2, '0', '9');
//         addRangeTransitions(s2, s2, 'A', 'F');
//         State* accept = makeAccept("hex", 7);
//         s2->addTransition(EPSILON, accept);
//     }

//     // Keywords: boolean int float if else while
//     // We build each keyword path and mark accepting with appropriate priorities (lower = higher priority)
//     {
//         std::vector<std::pair<std::string,int>> keywords = {
//             {"boolean", 1}, {"int", 2}, {"float", 3}, {"if", 4}, {"else", 5}, {"while", 6}
//         };
//         for (auto &kp : keywords) {
//             State* cur = makeState(newId());
//             masterStart->addTransition(EPSILON, cur);
//             for (char ch : kp.first) {
//                 State* nxt = makeState(newId());
//                 cur->addTransition(ch, nxt);
//                 cur = nxt;
//             }
//             State* acc = makeAccept(kp.first, kp.second);
//             cur->addTransition(EPSILON, acc);
//         }
//     }

//     // Operators and punctuations: == != >= <= > < = + - * / ; , ( ) { }
//     {
//         // multi-char operators: ==, !=, >=, <=
//         auto makeTwoChar = [&](char a, char b, const std::string &tok, int pr) {
//             State* s = makeState(newId()); masterStart->addTransition(EPSILON, s);
//             State* s1 = makeState(newId()); s->addTransition(a, s1);
//             State* s2 = makeState(newId()); s1->addTransition(b, s2);
//             State* acc = makeAccept(tok, pr);
//             s2->addTransition(EPSILON, acc);
//         };
//         makeTwoChar('=', '=', "EQ", 30);
//         makeTwoChar('!', '=', "NEQ", 31);
//         makeTwoChar('>', '=', "GE", 32);
//         makeTwoChar('<', '=', "LE", 33);

//         // single-char comparisons and assign
//         auto makeSingle = [&](char a, const std::string &tok, int pr) {
//             State* s = makeState(newId()); masterStart->addTransition(EPSILON, s);
//             State* s1 = makeState(newId()); s->addTransition(a, s1);
//             State* acc = makeAccept(tok, pr);
//             s1->addTransition(EPSILON, acc);
//         };
//         makeSingle('>', "GT", 34);
//         makeSingle('<', "LT", 35);
//         makeSingle('=', "ASSIGN", 36);
//         makeSingle('+', "PLUS", 37);
//         makeSingle('-', "MINUS", 38);
//         makeSingle('*', "MUL", 39);
//         makeSingle('/', "DIV", 40);
//         makeSingle(';', "SEMI", 41);
//         makeSingle(',', "COMMA", 42);
//         makeSingle('(', "LPAREN", 43);
//         makeSingle(')', "RPAREN", 44);
//         makeSingle('{', "LBRACE", 45);
//         makeSingle('}', "RBRACE", 46);
//     }

//     // Now we have a master NFA start (masterStart). Convert to DFA then minimize and print.

//     // Convert NFA -> DFA
//     std::vector<char> alphabet;
//     // Build alphabet: letters, digits, operator/punct chars used above
//     for (char c='a'; c<='z'; ++c) alphabet.push_back(c);
//     for (char c='A'; c<='Z'; ++c) alphabet.push_back(c);
//     for (char c='0'; c<='9'; ++c) alphabet.push_back(c);
//     std::string ops = "=!><+-*/;,(){}";
//     for (char c : ops) alphabet.push_back(c);

//     std::cout << "Converting NFA to DFA...\n";
//     DFA dfa = convertNFAtoDFA(masterStart, alphabet);
//     std::cout << "DFA states: " << dfa.states.size() << "\n";

//     std::cout << "Minimizing DFA...\n";
//     DFA md = minimizeDFA(dfa, alphabet);
//     std::cout << "Minimized states: " << md.states.size() << "\n\n";

//     printTransitionTable(md, alphabet);

//     // cleanup
//     for (State* s : allocated) delete s;
//     delete masterStart;

//     std::cout << "Done Option 1.\n";
//     return 0;
// }
