#ifndef LEXICALANALYZER_H
#define LEXICALANALYZER_H

#include <string>
#include "SymbolTable.h"
#include "DFA.h"
#include "NFASimulator.h"

class LexicalAnalyzer {
public:
    // Constructor: takes input string and DFA
    LexicalAnalyzer(const std::string& filePath, const DFA& dfa);

    // Check if more tokens are available
    bool hasNext() const;

    // Get next token from input
    Token getNextToken();

    // Access the symbol table
    const SymbolTable& getSymbolTable() const;

    // Check if any error occurred
    bool errorOccured() const;

private:
    DFA dfa;                  // DFA for token recognition
    bool errorOccurred;       // flag for error detection
    std::string inputString;  // full input source
    size_t pos;               // current position in input
    SymbolTable symbolTable;  // symbol table for identifiers
};

#endif // LEXICALANALYZER_H
