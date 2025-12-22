#include "Parser.h"
#include <iostream>
#include <algorithm>

using namespace std;

Parser::Parser(LexicalAnalyzer& lex, const ParsingTable& tab, const std::string& startSymbol) 
    : lexer(lex), table(tab) {
    
    // Initialize Stack: Push End Marker ($) then Start Symbol
    parseStack.push(END_MARKER);
    parseStack.push(startSymbol);
}

void Parser::parse() {
    cout << "\n--- STARTING PARSE ---\n";

    // Prime the pump: Get the first token
    Token currentToken = lexer.getNextToken();

    while (!parseStack.empty()) {
        string top = parseStack.top();

        // Check for End of Parsing
        if (top == END_MARKER) {
            if (currentToken.type == "EOF" || currentToken.value == "" || currentToken.type == "ERROR") {
                cout << "SUCCESS: Stack empty and Input finished.\n";
                break;
            } else {
                cerr << "ERROR: Stack is empty but input still has tokens: " << currentToken.value << endl;
                return;
            }
        }

        // CASE 1: Top of Stack is a Terminal
        // (We check this by seeing if it's NOT in the table's NonTerminal keys, 
        //  OR strictly if it is a known terminal. For simplicity, if it's not a NonTerm, it's a Term)
        // NOTE: In your TableGen, we didn't expose a simple isNonTerminal check, 
        // so we assume if it's in the table keys (LHS), it's a NonTerminal.
        
        bool isNonTerminal = false;
        // Check if 'top' exists as a LHS in any entry of the table
        // This is a bit slow; better to pass the Grammar set, but this works for now.
        for(auto const& [key, val] : table) {
            if (key.first == top) {
                isNonTerminal = true;
                break;
            }
        }

        if (!isNonTerminal) {
            // Match Logic
            if (top == currentToken.type || top == EPSILON_SYMBOL) {
                if (top != EPSILON_SYMBOL) {
                    // Consumed a real terminal
                    // cout << "Matched Terminal: " << top << endl;
                    parseStack.pop();
                    
                    // Advance input only if we matched a real token
                    if (lexer.hasNext()) {
                        currentToken = lexer.getNextToken();
                    } else {
                        // Create a dummy EOF token if we run out
                        currentToken = { "EOF", "" }; 
                    }
                } else {
                    // EPSILON_SYMBOL match (pop only, do not consume input)
                    parseStack.pop();
                }
            } else {
                cerr << "SYNTAX ERROR: Expected '" << top << "' but found '" << currentToken.type << "' (" << currentToken.value << ")" << endl;
                return; // Simple panic: Stop parsing
            }
        }
        // CASE 2: Top of Stack is a Non-Terminal
        else {
            // Look up M[top, currentToken.type]
            pair<string, string> key = { top, currentToken.type };
            
            if (table.find(key) != table.end()) {
                // Found a production!
                vector<string> production = table[key];
                
                printProduction(top, production); // Output step
                
                parseStack.pop(); // Remove LHS
                
                // Push RHS in REVERSE order
                // (Unless RHS is EPSILON_SYMBOL/Lambda, then push nothing)
                if (!(production.size() == 1 && production[0] == EPSILON_SYMBOL)) {
                    for (int i = production.size() - 1; i >= 0; --i) {
                        parseStack.push(production[i]);
                    }
                }
            } else {
                // Empty table cell = Error
                cerr << "SYNTAX ERROR: Unexpected token '" << currentToken.type << "' (" << currentToken.value << ") while parsing " << top << endl;
                return;
            }
        }
    }
}

void Parser::printProduction(const std::string& lhs, const std::vector<std::string>& rhs) {
    // This matches the "Leftmost Derivation" output requirement roughly
    cout << lhs << " -> ";
    for (const string& s : rhs) cout << s << " ";
    cout << endl;
}