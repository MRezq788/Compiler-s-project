#include <iostream>
#include <vector>
#include <set>
#include <queue>

// Phase 1 Headers
#include "RulesParser.h"
#include "NFAConstruction.h"
#include "NFAVisualizer.h"
#include "NFASimulator.h"
#include "DFA.h"
#include "part2.cpp" // Contains convertNFAtoDFA and minimizeDFA
#include "LexicalAnalyzerTester.h"

// Phase 2 Headers 
#include "TableGen.h" 
#include "Parser.h"
#include "GrammarTransformer.h"

using namespace std;

// Compile Command:
// g++ main.cpp NFAConstruction.cpp RulesParser.cpp LexicalAnalyzerTester.cpp SymbolTable.cpp LexicalAnalyzer.cpp CFG.cpp TableGen.cpp Parser.cpp GrammarTransformer.cpp -o compiler

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


void printGrammar(const Grammar& g) {
    std::cout << "Start Symbol: " << g.startSymbol << "\n";

    std::cout << "Non-terminals: ";
    for (const auto& nt : g.nonTerminals)
        std::cout << nt << " ";
    std::cout << "\n";

    std::cout << "Terminals: ";
    for (const auto& t : g.terminals)
        std::cout << t << " ";
    std::cout << "\n";

    std::cout << "Productions:\n";
    for (const auto& p : g.productions) {
        std::cout << "  " << p.first << " -> ";
        for (size_t i = 0; i < p.second.size(); i++) {
            for (const auto& sym : p.second[i])
                std::cout << sym << " ";
            if (i + 1 < p.second.size())
                std::cout << "| ";
        }
        std::cout << "\n";
    }
}

int main() {
    // ==========================================
    // PHASE 1: LEXICAL ANALYZER GENERATION
    // ==========================================
    cout << "=== Phase 1: Lexical Generator Front-End ===" << endl;

    RulesParser parser;
    State* startState = parser.parseFile("rules.txt");

    if (!startState || startState->id == 0) {
        cerr << "FATAL ERROR: NFA generation failed. Check 'rules.txt'." << endl;
        return 0;
    }

    cout << "----------------------------------------" << endl;
    cout << "Accepted Input Characters (Alphabet):" << endl;
    vector<char> alphabet = getAlphabet(startState);

    // 1. Convert NFA -> DFA
    DFA dfa = convertNFAtoDFA(startState, alphabet);
    std::cout << "DFA states: " << dfa.states.size() << "\n";

    // 2. Minimize DFA
    std::cout << "Minimizing DFA...\n";
    DFA md = minimizeDFA(dfa, alphabet);
    std::cout << "Minimized states: " << md.states.size() << "\n\n";

    printTransitionTable(md, alphabet);

    // 3. Test Phase 1 (Optional Debugging Output)
    cout<< endl << "--- Lexical Analyzer Test Output (Tokens) ---" << endl;
    // We create a temporary tester to print tokens to console for verification
    LexicalAnalyzerTester tester = LexicalAnalyzerTester("test.txt", md);
    tester.run();

    // ==========================================
    // PHASE 2: PARSER GENERATION
    // ==========================================
    cout << "\n\n=== Phase 2: Parser Generator ===" << endl;

    cout << "Reading Grammar..." << endl;
    Grammar originalGrammar = readGrammar("grammar.txt");
    GrammarTransformer transformer;
    Grammar grammarWithNoLR = transformer.eliminateLeftRecursion(originalGrammar);
    Grammar grammar = transformer.leftFactor(grammarWithNoLR);
    printGrammar(grammar);

    if (grammar.productions.empty()) {
        cerr << "FATAL ERROR: Grammar is empty. Check 'grammar.txt'." << endl;
        return 0;
    }

    cout << "Computing FIRST sets..." << endl;
    computeFirst(grammar);
    printFirst();

    cout << "Computing FOLLOW sets..." << endl;
    computeFollow(grammar);
    printFollow();

    cout << "Building Parsing Table..." << endl;
    ParsingTable parsingTable = buildParsingTable(grammar);
    printParsingTable(parsingTable);
    
    cout << "\n--- Parsing Source Code ---" << endl;
    LexicalAnalyzer parserLexer("test.txt", md); 

    Parser parserEngine(parserLexer, parsingTable, grammar.startSymbol, "parser_output.txt");
    
    parserEngine.parse();
    
    cout << "Done! Check 'parser_output.txt' for results." << endl;

    return 0;
}