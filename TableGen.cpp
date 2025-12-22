#include "TableGen.h"
#include <iostream>
#include <algorithm>

using namespace std;

// To run
// g++ -std=c++17 CFG.cpp TableGen.cpp -o follow.exe
// ./follow

map<string, set<string>> FOLLOW;

// Get FIRST set of a sequence of symbols (e.g., A B C)
// needs this to calculate FOLLOW. If A -> a B beta, FOLLOW(B) needs FIRST(beta).

set<string> getFirstOfSequence(const vector<string>& sequence) {
    set<string> result;
    bool allNullable = true;

    for (const string& sym : sequence) {
        if (sym == EPSILON_SYMBOL) {
            continue; 
        }

        // If it's a terminal, add it and stop
        if (FIRST.find(sym) == FIRST.end()) {
            // It's a terminal (not in FIRST keys) or a terminal defined in FIRST
            // Based on Member 1's code, terminals are in FIRST map too.
            if (FIRST.count(sym)) {
                for(const string& f : FIRST[sym]) {
                    if(f != EPSILON_SYMBOL) result.insert(f);
                }
                if(FIRST[sym].count(EPSILON_SYMBOL) == 0) {
                    allNullable = false;
                    break;
                }
            } else {
                // Should be a literal terminal not explicitly in FIRST map (fallback)
                result.insert(sym);
                allNullable = false;
                break;
            }
        } 
        else {
            // It's a non-terminal
            const set<string>& firstSet = FIRST[sym];
            for (const string& val : firstSet) {
                if (val != EPSILON_SYMBOL) {
                    result.insert(val);
                }
            }

            // If this symbol doesn't produce EPSILON_SYMBOL, we stop looking ahead
            if (firstSet.find(EPSILON_SYMBOL) == firstSet.end()) {
                allNullable = false;
                break;
            }
        }
    }

    if (allNullable) {
        result.insert(EPSILON_SYMBOL);
    }
    return result;
}

void computeFollow(Grammar& grammar) {
    // Initialize FOLLOW sets
    for (const auto& nt : grammar.nonTerminals) {
        FOLLOW[nt]; // create empty set
    }

    // Rule 1: Place $ in FOLLOW of Start Symbol
    if (!grammar.startSymbol.empty()) {
        FOLLOW[grammar.startSymbol].insert(END_MARKER);
    }

    // Iteratively apply Rule 2 and Rule 3 until no changes
    bool changed = true;
    while (changed) {
        changed = false;

        for (const auto& prod : grammar.productions) {
            const string& A = prod.first; // LHS

            for (const auto& rhs : prod.second) { // RHS Alternatives
                
                // Iterate through the RHS symbols
                for (size_t i = 0; i < rhs.size(); ++i) {
                    const string& B = rhs[i];

                    // We only compute FOLLOW for Non-Terminals
                    if (grammar.nonTerminals.find(B) != grammar.nonTerminals.end()) {
                        
                        // Create the suffix beta (symbols following B)
                        vector<string> beta;
                        if (i + 1 < rhs.size()) {
                            beta.assign(rhs.begin() + i + 1, rhs.end());
                        }

                        // Rule 2: FOLLOW(B) += FIRST(beta) - {EPSILON_SYMBOL}
                        set<string> firstOfBeta = getFirstOfSequence(beta);
                        
                        // Logic: If beta is empty, firstOfBeta contains EPSILON_SYMBOL by default logic 
                        // or is just empty if we treat empty vec as EPSILON_SYMBOL. 
                        // Actually getFirstOfSequence returns EPSILON_SYMBOL for empty input.

                        for (const string& f : firstOfBeta) {
                            if (f != EPSILON_SYMBOL) {
                                if (FOLLOW[B].insert(f).second) {
                                    changed = true;
                                }
                            }
                        }

                        // Rule 3: If EPSILON_SYMBOL is in FIRST(beta) OR beta is empty,
                        // then FOLLOW(B) += FOLLOW(A)
                        if (firstOfBeta.find(EPSILON_SYMBOL) != firstOfBeta.end()) {
                            for (const string& f : FOLLOW[A]) {
                                if (FOLLOW[B].insert(f).second) {
                                    changed = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

ParsingTable buildParsingTable(Grammar& grammar) {
    ParsingTable table;

    for (const auto& prod : grammar.productions) {
        const string& A = prod.first; // LHS

        for (const auto& rhs : prod.second) { // RHS
            // Calculate FIRST(alpha) for this production A -> alpha
            set<string> firstAlpha = getFirstOfSequence(rhs);

            // For each terminal 'a' in FIRST(alpha), add A->alpha to Table[A, a]
            for (const string& a : firstAlpha) {
                if (a != EPSILON_SYMBOL) {
                    if (table.count({A, a})) {
                        cerr << "ERROR: Grammar is NOT LL(1). Conflict at [" << A << ", " << a << "]" << endl;
                    }
                    table[{A, a}] = rhs;
                }
            }

            // 3. If EPSILON_SYMBOL is in FIRST(alpha), add A->alpha to Table[A, b] for each b in FOLLOW(A)
            if (firstAlpha.find(EPSILON_SYMBOL) != firstAlpha.end()) {
                for (const string& b : FOLLOW[A]) {
                    if (table.count({A, b})) {
                        cerr << "ERROR: Grammar is NOT LL(1). Conflict at [" << A << ", " << b << "]" << endl;
                    }
                    // For table entry, we store the production that derived EPSILON_SYMBOL.
                    // If A -> \L, rhs is {\L}. If A -> X Y and X,Y -> \L, rhs is {X, Y}.
                    table[{A, b}] = rhs;
                }
            }
        }
    }
    return table;
}

void printFollow() {
    cout << "\nFOLLOW SETS :-\n";
    for (auto& entry : FOLLOW) {
        cout << "FOLLOW(" << entry.first << ") = { ";
        for (auto& s : entry.second)
            cout << s << " ";
        cout << "}\n";
    }
}

void printParsingTable(const ParsingTable& table) {
    cout << "\n--- PREDICTIVE PARSING TABLE ---\n";
    for (const auto& entry : table) {
        cout << "M[" << entry.first.first << ", " << entry.first.second << "] = " << entry.first.first << " -> ";
        for (const string& s : entry.second) {
            cout << s << " ";
        }
        cout << endl;
    }
}

// int main() {
//     Grammar grammar = readGrammar("grammar.txt");
//     computeFirst(grammar);
//     computeFollow(grammar);
//     printFirst();
//     printFollow();

//     ParsingTable parsingTable = buildParsingTable(grammar);
//     printParsingTable(parsingTable);

//     return 0;
// }