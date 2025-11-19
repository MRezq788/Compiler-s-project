#ifndef NFAVISUALIZER_H
#define NFAVISUALIZER_H

#include "NFA.h"
#include <iostream>
#include <fstream>
#include <set>
#include <queue>
#include <string>

class NFAVisualizer {
public:
    static void generateDOT(State* startState, std::string filename) {
        std::ofstream out(filename);
        if (!out.is_open()) {
            std::cerr << "Error: Could not create visualization file." << std::endl;
            return;
        }

        out << "digraph NFA {" << std::endl;
        out << "  rankdir=LR;" << std::endl; // Left to Right layout
        out << "  node [shape=circle];" << std::endl;

        std::set<int> visited;
        std::queue<State*> q;
        
        q.push(startState);
        visited.insert(startState->id);

        // Visual marker for the purely start state
        out << "  secret_node [style=invis];" << std::endl;
        out << "  secret_node -> " << startState->id << " [label=\"start\"];" << std::endl;

        while(!q.empty()) {
            State* current = q.front();
            q.pop();

            // Special formatting for Accepting States (Double Circle)
            if (current->isAccepting) {
                out << "  " << current->id << " [shape=doublecircle, style=bold, label=\"" << current->id << "\\n" << current->tokenClass << "\"];" << std::endl;
            } else {
                out << "  " << current->id << " [label=\"" << current->id << "\"];" << std::endl;
            }

            // Process Transitions
            for (const auto& trans : current->transitions) {
                // Label handling: Check for Epsilon
                std::string label = (trans.input == EPSILON) ? "ε" : std::string(1, trans.input);
                
                // Escape special characters if necessary (simple version)
                if (label == "\"") label = "\\\"";

                out << "  " << current->id << " -> " << trans.nextState->id << " [label=\"" << label << "\"];" << std::endl;

                if (visited.find(trans.nextState->id) == visited.end()) {
                    visited.insert(trans.nextState->id);
                    q.push(trans.nextState);
                }
            }
        }

        out << "}" << std::endl;
        out.close();
        std::cout << "Visualization saved to: " << filename << ". Copy content to https://dreampuf.github.io/GraphvizOnline/" << std::endl;
    }
};

#endif