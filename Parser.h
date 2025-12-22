#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <stack>
#include <vector>
#include <fstream> 
#include "LexicalAnalyzer.h"
#include "TableGen.h"

class Parser {
public:
    Parser(LexicalAnalyzer& lexer, const ParsingTable& table, const std::string& startSymbol, const std::string& outFilename);
    ~Parser(); // Destructor to close file

    void parse();

private:
    LexicalAnalyzer& lexer;
    ParsingTable table;
    std::stack<std::string> parseStack;
    std::ofstream outFile;
    std::vector<std::string> sententialForm;

    // Helper to print the full sentential form to file
    void printDerivation();
    
    // Helper to update the sentential form string
    void updateDerivation(const std::string& lhs, const std::vector<std::string>& rhs);
    void recover(const std::string& top, Token& currentToken, bool isNonTerminal);
};

#endif