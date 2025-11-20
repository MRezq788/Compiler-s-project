#ifndef LEXICALANALYZERTESTER_H
#define LEXICALANALYZERTESTER_H

#include <string>
#include "LexicalAnalyzer.h"
#include "NFASimulator.h"

class LexicalAnalyzerTester {
public:
    // Constructor: takes file path and DFA
    LexicalAnalyzerTester(const std::string& filePath, const DFA& dfa);

    // Run the test: print all tokens
    void run();

private:
    LexicalAnalyzer lexer;
};

#endif
