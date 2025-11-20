#include "LexicalAnalyzerTester.h"
#include <iostream>
#include <fstream>
#include "NFASimulator.h"

// Constructor: open file and initialize lexer
LexicalAnalyzerTester::LexicalAnalyzerTester(const std::string& filePath, const DFA& dfa)
    : lexer(filePath, dfa) // construct lexer with file stream
{}

// Run the test
void LexicalAnalyzerTester::run() {
    while (lexer.hasNext()) {
        Token t = lexer.getNextToken();
        if (t.type=="ERROR") continue;
        std::cout << "Token: " << t.type << ", Lexeme: " << t.value << std::endl;
    }

    if (lexer.errorOccured()) {
        std::cout << "Errors were detected during lexical analysis.\n";
    }

    SymbolTable SymbolTable = lexer.getSymbolTable();
    SymbolTable.printAllIds();
}
