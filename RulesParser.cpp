#include "RulesParser.h"
#include "NFAConstruction.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <regex>

// --- Utils ---
std::string RulesParser::trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (std::string::npos == first) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::string RulesParser::removeSpaces(std::string str) {
    std::string result = "";
    for (size_t i = 0; i < str.length(); i++) {
        // Keep space if it is escaped (though rare in this format, good practice)
        if (str[i] == '\\' && i + 1 < str.length() && str[i+1] == ' ') {
            result += ' '; // Add the literal space
            i++;
        }
        else if (str[i] != ' ' && str[i] != '\t') {
            result += str[i];
        }
    }
    return result;
}

// --- Core Logic ---

std::string RulesParser::replaceDefinedVars(std::string regex) {
    // Sort keys by length descending to avoid partial replacements
    std::vector<std::string> keys;
    for(auto const& imap: definitions) keys.push_back(imap.first);
    std::sort(keys.begin(), keys.end(), [](const std::string& a, const std::string& b) {
        return a.length() > b.length(); 
    });

    for (const auto& key : keys) {
        // Enclose in parens to ensure precedence safety
        std::string val = "(" + definitions[key] + ")";
        
        size_t index = 0;
        while (true) {
            // Find variable name
            index = regex.find(key, index);
            if (index == std::string::npos) break;
            
            // Simple boundary check could be added here if necessary
            // For this assignment, simple replacement is usually sufficient
            regex.replace(index, key.length(), val);
            index += val.length();
        }
    }
    return regex;
}

State* RulesParser::parseFile(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "CRITICAL ERROR: Could not open file '" << filename << "'" << std::endl;
        return nullptr;
    }

    std::string line;
    std::vector<NFAFragment*> tokenNFAs;
    int priorityCounter = 1;
    int lineNum = 0;

    std::cout << "--- Parsing " << filename << " ---" << std::endl;

    while (std::getline(file, line)) {
        lineNum++;
        line = trim(line);
        if (line.empty()) continue;

        // 1. KEYWORDS: Enclosed in { }
        if (line.front() == '{') {
            parseKeywords(line, tokenNFAs);
        }
        // 2. PUNCTUATION: Enclosed in [ ]
        else if (line.front() == '[') {
            parsePunctuation(line, tokenNFAs);
        }
        // 3. REGULAR DEFINITIONS: LHS = RHS
        else if (line.find('=') != std::string::npos && line.find(':') == std::string::npos) {
            std::stringstream ss(line);
            std::string lhs, rhs;
            std::getline(ss, lhs, '=');
            std::getline(ss, rhs);
            
            lhs = trim(lhs);
            rhs = trim(rhs);
            rhs = removeSpaces(rhs);
            rhs = replaceDefinedVars(rhs); 
            
            definitions[lhs] = rhs;
            std::cout << "   Defined: " << lhs << std::endl;
        }
        // 4. REGULAR EXPRESSIONS: LHS : RHS
        else if (line.find(':') != std::string::npos) {
            size_t colonPos = line.find(':');
            std::string lhs = trim(line.substr(0, colonPos));
            std::string rhs = trim(line.substr(colonPos + 1));

            rhs = removeSpaces(rhs); // Remove formatting spaces
            rhs = replaceDefinedVars(rhs); // Expand {digit}, {letter}, etc.
            rhs = preprocessRegex(rhs); // Insert implicit concat and handle ranges
            std::string postfix = toPostfix(rhs);
            
            NFAFragment* nfa = evaluatePostfix(postfix);
            
            if (nfa) {
                nfa->end->isAccepting = true;
                
                if (lhs == "assign") {
                    nfa->end->tokenClass = "=";
                } 
                else {
                    nfa->end->tokenClass = lhs;
                }
                
                nfa->end->priority = priorityCounter++;
                tokenNFAs.push_back(nfa);
                std::cout << "   Parsed Token: " << lhs << std::endl;
            } else {
                std::cerr << "   ERROR: Invalid Regex for " << lhs << std::endl;
            }
        }
    }

    // Combine into one big NFA
    State* superStart = new State(NFAConstruction::getNextStateID());
    for (NFAFragment* frag : tokenNFAs) {
        superStart->addTransition(EPSILON, frag->start);
    }

    return superStart;
}

void RulesParser::parseKeywords(std::string line, std::vector<NFAFragment*>& tokens) {
    // Format: { int float while }
    if (line.length() < 2) return;
    std::string content = line.substr(1, line.length() - 2);
    std::stringstream ss(content);
    std::string word;
    
    while (ss >> word) {
        NFAFragment* current = nullptr;
        for (char c : word) {
            NFAFragment* ch = NFAConstruction::createBasic(c);
            if (current == nullptr) current = ch;
            else current = NFAConstruction::concatenate(current, ch);
        }
        if (current) {
            current->end->isAccepting = true;
            current->end->tokenClass = word;
            current->end->priority = 0; // Keywords have highest priority
            tokens.push_back(current);
            std::cout << "   Keyword: " << word << std::endl;
        }
    }
}

void RulesParser::parsePunctuation(std::string line, std::vector<NFAFragment*>& tokens) {
    // Robust parsing: Find the content between [ and ]
    size_t openBracket = line.find('[');
    size_t closeBracket = line.find_last_of(']');

    if (openBracket == std::string::npos || closeBracket == std::string::npos || closeBracket <= openBracket) {
        std::cerr << "WARNING: Malformed punctuation line: " << line << std::endl;
        return;
    }

    std::string content = line.substr(openBracket + 1, closeBracket - openBracket - 1);
    
    for (size_t i = 0; i < content.length(); i++) {
        char c = content[i];
        if (c == ' ' || c == '\t') continue;

        char target = c;
        // Handle escapes inside punctuation brackets e.g. \[ or \(
        if (c == '\\' && i + 1 < content.length()) {
            target = content[++i];
        }

        NFAFragment* nfa = NFAConstruction::createBasic(target);
        nfa->end->isAccepting = true;
        std::string tokenName(1, target);
        nfa->end->tokenClass = tokenName; // The class name is the punctuation itself (e.g., ";")
        nfa->end->priority = 0; 
        tokens.push_back(nfa);
        
        // DEBUG PRINT: Confirm we added it
        std::cout << "   Parsed Punctuation: " << target << std::endl;
    }
}

// --- Regex Processing ---

int RulesParser::precedence(char op) {
    if (op == '|') return 1;
    if (op == CONCAT_OP) return 2;
    if (op == '*' || op == '+') return 3;
    return 0;
}

std::string RulesParser::expandRanges(std::string regex) {
    std::string result = "";
    for (size_t i = 0; i < regex.length(); i++) {
        // Check for a-z or 0-9 pattern
        bool isRange = false;
        if (i + 2 < regex.length() && regex[i+1] == '-') {
            // Ensure previous char wasn't escaped (e.g., \a-z is not a range)
            bool prevEscaped = (i > 0 && regex[i-1] == '\\');
            if (!prevEscaped) {
                char start = regex[i];
                char end = regex[i+2];
                // Simple alphanumeric range check
                if (isalnum(start) && isalnum(end) && start <= end) {
                    result += "(";
                    for (char c = start; c <= end; c++) {
                        result += c;
                        if (c < end) result += "|";
                    }
                    result += ")";
                    i += 2; // Skip 'start-end'
                    isRange = true;
                }
            }
        }
        
        if (!isRange) {
            result += regex[i];
        }
    }
    return result;
}

std::string RulesParser::preprocessRegex(std::string regex) {
    regex = expandRanges(regex);
    std::string result = "";
    
    for (size_t i = 0; i < regex.length(); i++) {
        char c1 = regex[i];

        // Add current char to result
        if (c1 == '\\') {
            result += c1;
            if (i + 1 < regex.length()) {
                result += regex[++i]; // Add the escaped char
            }
        } else {
            result += c1;
        }

        // Decide if we need to insert Concatenation Operator after this char
        if (i + 1 < regex.length()) {
            char c2 = regex[i+1];
            
            // Logic: Insert '.' (CONCAT_OP) if:
            // c1 is literal, ')', '*', or '+'
            // c2 is literal, '(', or '\'
            
            bool c1_is_operand_end = (c1 != '(' && c1 != '|' && c1 != CONCAT_OP);
            // If c1 was the second char of an escape sequence (handled by loop increment), it is a literal
            
            bool c2_is_operand_start = (c2 != ')' && c2 != '|' && c2 != '*' && c2 != '+' && c2 != CONCAT_OP);
            
            if (c1_is_operand_end && c2_is_operand_start) {
                result += CONCAT_OP;
            }
        }
    }
    return result;
}

std::string RulesParser::toPostfix(std::string regex) {
    std::string postfix = "";
    std::stack<char> opStack;
    
    for (size_t i = 0; i < regex.length(); i++) {
        char c = regex[i];

        if (c == '\\') {
             // Escaped character: Treat as literal
             postfix += regex[++i]; 
             postfix += '\\'; // Mark as literal
        } 
        else if (c == '(') {
            opStack.push(c);
        } 
        else if (c == ')') {
            while (!opStack.empty() && opStack.top() != '(') {
                postfix += opStack.top();
                opStack.pop();
            }
            if(!opStack.empty()) opStack.pop();
        } 
        else if (c == '*' || c == '+' || c == '|' || c == CONCAT_OP) { 
            while (!opStack.empty() && precedence(opStack.top()) >= precedence(c)) {
                postfix += opStack.top();
                opStack.pop();
            }
            opStack.push(c);
        }
        else {
            // Normal characters, literals (including '.')
            postfix += c;
        }
    }
    while (!opStack.empty()) {
        postfix += opStack.top();
        opStack.pop();
    }
    return postfix;
}

NFAFragment* RulesParser::evaluatePostfix(std::string postfix) {
    std::stack<NFAFragment*> stack;

    for (size_t i = 0; i < postfix.length(); i++) {
        char c = postfix[i];

        if (i + 1 < postfix.length() && postfix[i+1] == '\\') {
             // Handle Escaped Chars
             // Check for Lambda \L
             if (c == 'L') {
                 stack.push(NFAConstruction::createEpsilon());
             } else {
                 stack.push(NFAConstruction::createBasic(c));
             }
             i++; // Skip the backslash marker
        }
        else if (c == CONCAT_OP) {
            if(stack.size() < 2) continue;
            NFAFragment* op2 = stack.top(); stack.pop();
            NFAFragment* op1 = stack.top(); stack.pop();
            stack.push(NFAConstruction::concatenate(op1, op2));
        } 
        else if (c == '|') {
             if(stack.size() < 2) continue;
            NFAFragment* op2 = stack.top(); stack.pop();
            NFAFragment* op1 = stack.top(); stack.pop();
            stack.push(NFAConstruction::join(op1, op2));
        } 
        else if (c == '*') {
            if(stack.empty()) continue;
            NFAFragment* op = stack.top(); stack.pop();
            stack.push(NFAConstruction::kleeneStar(op));
        } 
        else if (c == '+') {
            if(stack.empty()) continue;
            NFAFragment* op = stack.top(); stack.pop();
            stack.push(NFAConstruction::positiveClosure(op));
        }
        else {
            // Literal
            stack.push(NFAConstruction::createBasic(c));
        }
    }
    return stack.empty() ? nullptr : stack.top();
}