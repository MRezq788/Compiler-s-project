#include "LexicalAnalyzer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cctype>
#include "NFASimulator.h"

static inline void rtrim(std::string &s);
static void handleLexicalError(char errPosition);

LexicalAnalyzer::LexicalAnalyzer(const std::string& filePath, const DFA& dfa)
    : dfa(dfa), errorOccurred(false), pos(0)
{
    std::ifstream input(filePath);
    if (!input) {
        throw std::runtime_error("Could not open file: " + filePath);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    inputString = buffer.str();
    rtrim(inputString);
    inputString += '\n';
}


bool LexicalAnalyzer::hasNext() const {
    return pos < inputString.size();
}


Token LexicalAnalyzer::getNextToken() {
    if (pos >= inputString.size()) {
        return {"$", ""}; 
    }

    int currentState = dfa.startState;
    int lastAcceptingState = -1;
    size_t lastAcceptingIndex = pos;

    std::string lexeme;
    size_t i = pos;

    while (i < inputString.size()) {
        char c = inputString[i];
        if (isspace(c) && lexeme.empty()) {
            i++;
            pos++;
            continue;
        }

        auto it = dfa.states[currentState].transitions.find(c);
        if (it == dfa.states[currentState].transitions.end()) {
            break; 
        }

        currentState = it->second;
        lexeme += c;

        if (dfa.states[currentState].isAccepting) {
            lastAcceptingState = currentState;
            lastAcceptingIndex = i + 1;
        }

        i++;
    }

    if (lastAcceptingState != -1) {
        pos = lastAcceptingIndex;
        std::string tokenClass = dfa.states[lastAcceptingState].tokenClass;

        // If identifier, add to symbol table
        if (tokenClass == "id") {
            symbolTable.addId(lexeme, IdInfo());
        }

        return {tokenClass, lexeme};
    }

    // Panic mode: no accepting state reached
    errorOccurred = true;

    if (pos >= inputString.size()) {
         return {"$", ""};
    }

    handleLexicalError(inputString[pos]);
    pos++; // skip bad character
    return getNextToken();
}


const SymbolTable& LexicalAnalyzer::getSymbolTable() const {
    return symbolTable;
}

bool LexicalAnalyzer::errorOccured() const {
    return errorOccurred;
}

// helper functions
// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); }).base(),
            s.end());
}

static void handleLexicalError(char errPositionChar) {
    if (!isspace(errPositionChar)) {
        std::cerr << "Lexical error at character " << errPositionChar << std::endl;
    }
}