#ifndef GRAMMAR_TRANSFORMER_H
#define GRAMMAR_TRANSFORMER_H

#include <string>
#include <set>
#include <map>
#include <vector>
#include "CFG.h" 

class GrammarTransformer {
public:
    // Eliminate left recursion from the given grammar
    Grammar eliminateLeftRecursion(const Grammar& g);

    // Appliy left factoring to the given grammar
    // Grammar leftFactor(const Grammar& g);
private: 
    void eliminateImmediateLeftRecursion( 
        const std::string& nonTerminal, 
        std::vector<std::vector<std::string>>& rules, Grammar& result 
    );
};

#endif
