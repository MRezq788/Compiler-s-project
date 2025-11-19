#ifndef NFASIMULATOR_H
#define NFASIMULATOR_H

#include "NFA.h"
#include <string>
#include <vector>
#include <set>
#include <stack>
#include <iostream>
#include <algorithm>

struct Token {
    std::string type;
    std::string value;
};

class NFASimulator {
public:
    // NEW: Tokenize a full string based on "Maximal Munch" (Longest Match)
    static std::vector<Token> tokenize(State* startState, std::string input) {
        std::vector<Token> tokens;
        int currentPos = 0;

        while (currentPos < input.length()) {
            // 1. Skip Whitespace
            if (isspace(input[currentPos])) {
                currentPos++;
                continue;
            }

            // 2. Try to find the longest match starting from currentPos
            int lastAcceptingPos = -1;
            State* lastAcceptingState = nullptr;
            
            std::set<State*> currentStates;
            currentStates.insert(startState);
            currentStates = computeEpsilonClosure(currentStates);

            int tempPos = currentPos;
            
            // Run NFA forward until it dies
            while (tempPos < input.length()) {
                char c = input[tempPos];
                
                // Check if we are currently in an accepting state
                State* accepting = getBestAccepting(currentStates);
                if (accepting) {
                    lastAcceptingPos = tempPos; // Mark this position as a valid stopping point
                    lastAcceptingState = accepting;
                }

                // Move to next set of states
                std::set<State*> nextStates;
                for (State* s : currentStates) {
                    for (const auto& trans : s->transitions) {
                        if (trans.input == c) {
                            nextStates.insert(trans.nextState);
                        }
                    }
                }

                if (nextStates.empty()) {
                    // Dead end: The NFA cannot consume 'c'
                    break;
                }

                currentStates = computeEpsilonClosure(nextStates);
                tempPos++;
            }
            
            // Check one last time if we ended in an accepting state after loop
            State* accepting = getBestAccepting(currentStates);
            if (accepting) {
                lastAcceptingPos = tempPos;
                lastAcceptingState = accepting;
            }

            // 3. Logic for Longest Match
            if (lastAcceptingState != nullptr) {
                // We found a token!
                // The token text is from currentPos to lastAcceptingPos (exclusive)
                // Note: lastAcceptingPos is the index *after* the last valid char
                int length = lastAcceptingPos - currentPos;
                std::string lexeme = input.substr(currentPos, length);
                
                tokens.push_back({lastAcceptingState->tokenClass, lexeme});
                
                // Advance main pointer
                currentPos = lastAcceptingPos;
            } else {
                // Error: We consumed characters but found no accepting state
                std::cerr << "Error: Unrecognized character '" << input[currentPos] << "' at index " << currentPos << std::endl;
                currentPos++; // Panic mode: skip one char and try again
            }
        }
        
        return tokens;
    }

private:
    // Helper: Get the accepting state with the highest priority (lowest priority number)
    static State* getBestAccepting(const std::set<State*>& states) {
        State* best = nullptr;
        int bestPriority = 2147483647; // Max Int

        for (State* s : states) {
            if (s->isAccepting) {
                if (s->priority < bestPriority) {
                    bestPriority = s->priority;
                    best = s;
                }
            }
        }
        return best;
    }

    // Helper: Find all states reachable via Epsilon
    static std::set<State*> computeEpsilonClosure(std::set<State*> states) {
        std::set<State*> closure = states;
        std::stack<State*> stack;
        
        for (State* s : states) stack.push(s);

        while (!stack.empty()) {
            State* current = stack.top();
            stack.pop();

            for (const auto& trans : current->transitions) {
                if (trans.input == EPSILON) {
                    if (closure.find(trans.nextState) == closure.end()) {
                        closure.insert(trans.nextState);
                        stack.push(trans.nextState);
                    }
                }
            }
        }
        return closure;
    }
};

#endif