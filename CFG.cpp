#include "CFG.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// g++ -std=c++17 CFG.cpp -o first.exe
// then run .\first

using namespace std;
map<string, set<string>> FIRST;


// Helper function to trim whitespace
string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    if (start == string::npos) return "";
    return s.substr(start, end - start + 1);
}

// Helper function to split a string by a delimiter
vector<string> split(const string& s, char delim) {
    vector<string> result;
    string temp;
    stringstream ss(s);
    while (getline(ss, temp, delim)) {
        result.push_back(trim(temp));
    }
    return result;
}

void normalizeQuotes(std::string& line) {
    const std::vector<std::pair<std::string, std::string>> replacements = {
        {"‘", "'"}, {"’", "'"},
        {"“", "'"}, {"”", "'"},
        {"–", "-"}, {"—", "-"}
    };

    for (const auto& rep : replacements) {
        size_t pos = 0;
        while ((pos = line.find(rep.first, pos)) != std::string::npos) {
            line.replace(pos, rep.first.length(), rep.second);
            pos += rep.second.length();
        }
    }

    // Remove any remaining non-ASCII characters
    line.erase(
        std::remove_if(line.begin(), line.end(),
            [](unsigned char c) { return c > 127; }),
        line.end()
    );
}


// Reading grammar from file
Grammar readGrammar(const std::string& filename) {
    Grammar grammar;
    std::ifstream file(filename);
    std::string line;
    std::string currentLHS;

    if (!file.is_open()) {
        std::cerr << "Error: Cannot open grammar file\n";
        return grammar;
    }

    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty())
            continue;

        // Remove leading #
        if (line[0] == '#')
            line = trim(line.substr(1));

        // Normalize UTF-8 smart quotes
        normalizeQuotes(line);

        // New production rule
        size_t eqPos = line.find('=');
        if (eqPos != std::string::npos && line.find('|') != 0) {
            currentLHS = trim(line.substr(0, eqPos));
            grammar.nonTerminals.insert(currentLHS);

            if (grammar.startSymbol.empty())
                grammar.startSymbol = currentLHS;

            std::string rhs = trim(line.substr(eqPos + 1));
            std::vector<std::string> alternatives = split(rhs, '|');

            for (const std::string& alt : alternatives) {
                std::vector<std::string> symbols;
                std::stringstream ss(alt);
                std::string sym;

                while (ss >> sym) {
                    if (sym == EPSILON_SYMBOL) {
                        // Keep \L as a symbol in RHS; no need to add to terminals
                        symbols.push_back(EPSILON_SYMBOL);
                    } 
                    else if (sym.front() == '\'' && sym.back() == '\'') {
                        std::string terminal = sym.substr(1, sym.size() - 2);
                        grammar.terminals.insert(terminal);
                        symbols.push_back(terminal);
                    } else {
                        symbols.push_back(sym);
                    }
                }

                grammar.productions[currentLHS].push_back(symbols);
            }
        }
        // Continuation rule starting with |
        else if (!currentLHS.empty() && line[0] == '|') {
            std::string rhs = trim(line.substr(1));
            std::vector<std::string> symbols;
            std::stringstream ss(rhs);
            std::string sym;

            while (ss >> sym) {
                if (sym.front() == '\'' && sym.back() == '\'') {
                    std::string terminal = sym.substr(1, sym.size() - 2);
                    grammar.terminals.insert(terminal);
                    symbols.push_back(terminal);
                } else {
                    symbols.push_back(sym);
                }
            }

            grammar.productions[currentLHS].push_back(symbols);
        }
    }

    return grammar;
}





void computeFirst(Grammar& grammar) {
    // Initialize FIRST sets for terminals
    for (const auto& t : grammar.terminals)
        FIRST[t].insert(t);

    // Initialize empty FIRST sets for non-terminals
    for (const auto& nt : grammar.nonTerminals)
        FIRST[nt];

    bool changed = true;
    while (changed) {
        changed = false;

        for (const auto& prod : grammar.productions) {
            const std::string& A = prod.first;

            for (const auto& rhs : prod.second) {
                bool allNullable = true; // tracks if all symbols in RHS can produce EPSILON_SYMBOL

                for (size_t i = 0; i < rhs.size(); ++i) {
                    const std::string& Y = rhs[i];

                    if (Y == EPSILON_SYMBOL) {
                        // RHS is EPSILON_SYMBOL itself
                        if (FIRST[A].insert(EPSILON_SYMBOL).second)
                            changed = true;
                        allNullable = true;
                        break; // nothing else to check
                    }

                    // Add FIRST(Y) \ {EPSILON_SYMBOL} to FIRST(A)
                    for (const auto& terminal : FIRST[Y]) {
                        if (terminal != EPSILON_SYMBOL) {
                            if (FIRST[A].insert(terminal).second)
                                changed = true;
                        }
                    }

                    // If Y cannot produce EPSILON_SYMBOL, stop here
                    if (FIRST[Y].find(EPSILON_SYMBOL) == FIRST[Y].end()) {
                        allNullable = false;
                        break;
                    }
                }

                // If all symbols can produce EPSILON_SYMBOL, add EPSILON_SYMBOL to FIRST(A)
                if (allNullable) {
                    if (FIRST[A].insert(EPSILON_SYMBOL).second)
                        changed = true;
                }
            }
        }
    }
}


void printFirst() {
    for (auto& entry : FIRST) {
        cout << "FIRST(" << entry.first << ") = { ";
        for (auto& s : entry.second)
            cout << s << " ";
        cout << "}\n";
    }
}

// int main() {
//     Grammar grammar = readGrammar("grammar.txt");

//     computeFirst(grammar);
//     printFirst();

//     return 0;
// }