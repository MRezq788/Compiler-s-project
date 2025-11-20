#include <iostream>
#include "RulesParser.h"
#include "NFAConstruction.h"
#include "NFAVisualizer.h"
#include "NFASimulator.h"
#include "DFA.h"
#include "part2.cpp"
#include "LexicalAnalyzerTester.h"

using namespace std;

// g++ main.cpp NFAConstruction.cpp RulesParser.cpp LexicalAnalyzerTester.cpp SymbolTable.cpp LexicalAnalyzer.cpp -o generator

vector<char> getAlphabet(State* startState) {
    set<char> inputs;
    set<int> visited;
    queue<State*> q;

    q.push(startState);
    visited.insert(startState->id);

    // BFS traversal to find all unique transition characters
    while (!q.empty()) {
        State* current = q.front();
        q.pop();

        for (const auto& trans : current->transitions) {
            // Filter out Epsilon and the internal Backspace char used for concatenation
            if (trans.input != EPSILON && trans.input != '\x08') {
                inputs.insert(trans.input);
            }

            if (visited.find(trans.nextState->id) == visited.end()) {
                visited.insert(trans.nextState->id);
                q.push(trans.nextState);
            }
        }
    }

    // Convert set to vector for sorting and printing
    vector<char> alphabet(inputs.begin(), inputs.end());
    return alphabet;
}

int main() {
    cout << "Phase 1: Lexical Generator Front-End" << endl;

    RulesParser parser;

    
    State* startState = parser.parseFile("rules.txt");

    if (!startState || startState->id == 0) {
        cerr << "FATAL ERROR: NFA generation failed. Check 'rules.txt'." << endl;
        return 0;
    }

    cout << "----------------------------------------" << endl;
    cout << "Accepted Input Characters (Alphabet):" << endl;
    vector<char> alphabet = getAlphabet(startState);

    DFA dfa = convertNFAtoDFA(startState, alphabet);
    std::cout << "DFA states: " << dfa.states.size() << "\n";

    std::cout << "Minimizing DFA...\n";
    DFA md = minimizeDFA(dfa, alphabet);
    std::cout << "Minimized states: " << md.states.size() << "\n\n";

    printTransitionTable(md, alphabet);

    cout<< endl << "Lexical Analyzer Test Output:" << endl;
    LexicalAnalyzerTester tester = LexicalAnalyzerTester("test.txt", md);
    tester.run();
    
    // cout << "{ ";
    // for (size_t i = 0; i < alphabet.size(); i++) {
    //     cout << alphabet[i];
    //     if (i < alphabet.size() - 1) cout << ", ";
    // }
    // cout << " }" << endl;
    // cout << "Total unique characters: " << alphabet.size() << endl;
    // cout << "----------------------------------------" << endl;
    

    // cout << "NFA Constructed Successfully!" << endl;
    // cout << "Start State ID: " << startState->id << endl;
    // cout << "----------------------------------------" << endl;
    // cout << "Type 'exit' to quit." << endl;
    // cout << "----------------------------------------" << endl;

    // string input;
    // while (true) {
    //     cout << "\nInput > ";
    //     if (!getline(cin, input)) break;
    //     if (input == "exit") break;
    //     if (input.empty()) continue;

    //     cout << "Tokens Found:" << endl;
    //     vector<Token> results = NFASimulator::tokenize(startState, input);
        
    //     for (const auto& t : results) {
    //         cout << t.type << endl;
    //     }
    // }

    return 0;
}
