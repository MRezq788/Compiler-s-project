#ifndef RULESPARSER_H
#define RULESPARSER_H

#include <string>
#include <map>
#include <stack>
#include <vector>
#include <iostream>
#include "NFA.h"

using namespace std;

class RulesParser {
private:
    map<string, string> definitions; 
    
    // CHANGED: Use a non-printable char (ASCII 8 / Backspace) for internal concatenation.
    // This guarantees NO CONFLICT with any character ( like '&' or '.' ) in the input file.
    const char CONCAT_OP = '\x08'; 

    int precedence(char op);
    string toPostfix(string regex);
    NFAFragment* evaluatePostfix(string postfix);
    string preprocessRegex(string regex);
    string expandRanges(string regex);
    
    // Helpers for file parsing
    string replaceDefinedVars(string regex);
    string removeSpaces(string regex);
    void parseKeywords(string line, vector<NFAFragment*>& tokens);
    void parsePunctuation(string line, vector<NFAFragment*>& tokens);
    string trim(const string& str);

public:
    State* parseFile(string filename);
};

#endif