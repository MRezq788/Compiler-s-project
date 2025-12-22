#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <stack>
#include <vector>
#include "LexicalAnalyzer.h"
#include "TableGen.h" // For ParsingTable definition

class Parser {
public:
    Parser(LexicalAnalyzer& lexer, const ParsingTable& table, const std::string& startSymbol);
    
    // The main loop
    void parse();

private:
    LexicalAnalyzer& lexer;
    ParsingTable table;
    std::stack<std::string> parseStack;
    
    // Helper to print the stack state (for debugging or tracing)
    void printStack();
    
    // Helper to print the production rule being applied
    void printProduction(const std::string& lhs, const std::vector<std::string>& rhs);
};

#endif