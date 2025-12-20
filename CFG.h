#ifndef CFG_H
#define CFG_H

#include <string>
#include <vector>
#include <map>
#include <set>

const std::string EPSILON = "\\L";

struct Grammar {
    std::string startSymbol;
    std::set<std::string> nonTerminals;
    std::set<std::string> terminals;
    std::map<std::string, std::vector<std::vector<std::string>>> productions;
};

// Reading grammar
Grammar readGrammar(const std::string& filename);

// FIRST sets
extern std::map<std::string, std::set<std::string>> FIRST;
void computeFirst(Grammar& grammar);

#endif
