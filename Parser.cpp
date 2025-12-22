#include "Parser.h"
#include <iostream>
#include <algorithm>

using namespace std;

Parser::Parser(LexicalAnalyzer& lex, const ParsingTable& tab, const std::string& startSymbol, const std::string& outFilename) 
    : lexer(lex), table(tab) {
    
    outFile.open(outFilename);
    if (!outFile.is_open()) {
        cerr << "ERROR: Could not create parser output file: " << outFilename << endl;
    }

    parseStack.push(END_MARKER);
    parseStack.push(startSymbol);

    sententialForm.push_back(startSymbol);
}

Parser::~Parser() {
    if (outFile.is_open()) {
        outFile.close();
    }
}

// Helper: Print current list of symbols to file (e.g. "int id ; STATEMENT")
void Parser::printDerivation() {
    if (!outFile.is_open()) return;

    for (size_t i = 0; i < sententialForm.size(); ++i) {
        outFile << sententialForm[i];
        if (i < sententialForm.size() - 1) outFile << " ";
    }
    outFile << endl;
}

// Helper: Find the leftmost Non-Terminal (LHS) and replace it with RHS
void Parser::updateDerivation(const std::string& lhs, const std::vector<std::string>& rhs) {
    // Find the FIRST occurrence of the LHS in the sentential form (Leftmost derivation)
    auto it = std::find(sententialForm.begin(), sententialForm.end(), lhs);
    
    if (it != sententialForm.end()) {
        // Position of the Non-Terminal we are expanding
        size_t index = std::distance(sententialForm.begin(), it);
        
        // Remove the LHS
        sententialForm.erase(it);

        // Insert the RHS at that position (unless it's Epsilon)
        if (!(rhs.size() == 1 && rhs[0] == EPSILON_SYMBOL)) {
            sententialForm.insert(sententialForm.begin() + index, rhs.begin(), rhs.end());
        }
    }
}

void Parser::parse() {
    cout << "\n--- STARTING PARSE ---\n";
    
    // Print the initial Start Symbol to file
    printDerivation();

    Token currentToken = lexer.getNextToken();

    while (!parseStack.empty()) {
        string top = parseStack.top();

        if (top == END_MARKER) {
            if (currentToken.type == "EOF" || currentToken.value == "" || currentToken.type == "ERROR") {
                cout << "SUCCESS: Parsing complete. Output saved.\n";
                break;
            } else {
                cerr << "ERROR: Stack empty but input remains: " << currentToken.value << endl;
                return;
            }
        }

        // Check if top is Non-Terminal
        bool isNonTerminal = false;
        for(auto const& [key, val] : table) {
            if (key.first == top) {
                isNonTerminal = true;
                break;
            }
        }

        if (!isNonTerminal) {
            // --- MATCH TERMINAL ---
            if (top == currentToken.type || top == EPSILON_SYMBOL) {
                if (top != EPSILON_SYMBOL) {
                    parseStack.pop();
                    if (lexer.hasNext()) currentToken = lexer.getNextToken();
                    else currentToken = { "EOF", "" };
                } else {
                    parseStack.pop();
                }
            } else {
                recover(top, currentToken, false);
            }
        }
        else {
            // --- EXPAND NON-TERMINAL ---
            pair<string, string> key = { top, currentToken.type };
            
            if (table.find(key) != table.end()) {
                vector<string> production = table[key];

                parseStack.pop(); 
                
                updateDerivation(top, production);
                printDerivation();

                if (!(production.size() == 1 && production[0] == EPSILON_SYMBOL)) {
                    for (int i = production.size() - 1; i >= 0; --i) {
                        parseStack.push(production[i]);
                    }
                }
            } else {
                recover(top, currentToken, true);
            }
        }
    }
}

void Parser::recover(const std::string& top, Token& currentToken, bool isNonTerminal) {
    outFile << "ERROR: Syntax Error at " << currentToken.value << std::endl;
    if (isNonTerminal) {
        cerr << "SYNTAX ERROR: No rule for [" << top << ", " << currentToken.type << "] - Recovering..." << std::endl;
    } else {
        cerr << "SYNTAX ERROR: Expected '" << top << "' but found '" << currentToken.type << "' - Recovering..." << std::endl;
    }

    // Panic-mode: Skip input tokens until one in FOLLOW(top) or EOF
    while (lexer.hasNext()) {
        if (currentToken.type == "EOF") {
            break;
        }
        if (FOLLOW[top].count(currentToken.type)) {
            parseStack.pop();
            break;
        }
        
        cerr << "Skipping invalid token: " << currentToken.type << std::endl;
        currentToken = lexer.getNextToken();
    }

    if (!parseStack.empty() && parseStack.top() == top) {
        parseStack.pop();
    }
}