#include <iostream>
#include "RulesParser.h"
#include "NFAConstruction.h"
#include "NFAVisualizer.h"
#include "NFASimulator.h"

using namespace std;

// g++ main.cpp NFAConstruction.cpp RulesParser.cpp -o generator

int main() {
    cout << "Phase 1: Lexical Generator Front-End" << endl;

    RulesParser parser;

    
    State* startState = parser.parseFile("rules.txt");

    if (!startState || startState->id == 0) {
        cerr << "FATAL ERROR: NFA generation failed. Check 'rules.txt'." << endl;
        return 0;
    }

    cout << "NFA Constructed Successfully!" << endl;
    cout << "Start State ID: " << startState->id << endl;
    cout << "----------------------------------------" << endl;
    cout << "Type 'exit' to quit." << endl;
    cout << "----------------------------------------" << endl;

    string input;
    while (true) {
        cout << "\nInput > ";
        if (!getline(cin, input)) break;
        if (input == "exit") break;
        if (input.empty()) continue;

        cout << "Tokens Found:" << endl;
        vector<Token> results = NFASimulator::tokenize(startState, input);
        
        for (const auto& t : results) {
            cout << t.type << " : " << t.value << endl;
        }
    }

    return 0;
}
