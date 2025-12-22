#include "GrammarTransformer.h"
#include <unordered_map>
#include <sstream>
#include <algorithm>


void GrammarTransformer::eliminateImmediateLeftRecursion(
    const std::string& A,
    std::vector<std::vector<std::string>>& rules,
    Grammar& result
) {
    std::vector<std::vector<std::string>> alpha;
    std::vector<std::vector<std::string>> beta;

    for (const auto& rule : rules) {
        if (!rule.empty() && rule[0] == A) {
            alpha.push_back(std::vector<std::string>(rule.begin() + 1, rule.end()));
        } else {
            beta.push_back(rule);
        }
    }

    if (alpha.empty()) {
        result.productions[A] = rules;
        return;
    }

    std::string A_dash = A + "'";
    result.nonTerminals.insert(A_dash);

    for (auto& b : beta) {
        b.push_back(A_dash);
        result.productions[A].push_back(b);
    }

    for (auto& a : alpha) {
        a.push_back(A_dash);
        result.productions[A_dash].push_back(a);
    }

    result.productions[A_dash].push_back({EPSILON});
}

Grammar GrammarTransformer::eliminateLeftRecursion(const Grammar& g) {
    Grammar result;
    result.terminals = g.terminals;
    result.startSymbol = g.startSymbol;

    std::vector<std::string> orderedNonTerminals(g.nonTerminals.begin(), g.nonTerminals.end());

    for (size_t i = 0; i < orderedNonTerminals.size(); ++i) {
        std::string Ai = orderedNonTerminals[i];
        std::vector<std::vector<std::string>> newRules;

        for (auto& rule : g.productions.at(Ai)) {
            if (!rule.empty() && std::find(orderedNonTerminals.begin(), orderedNonTerminals.begin() + i, rule[0]) != orderedNonTerminals.begin() + i) {
                std::string Aj = rule[0];
                for (const auto& delta : result.productions[Aj]) {
                    std::vector<std::string> newRule = delta;
                    newRule.insert(newRule.end(), rule.begin() + 1, rule.end());
                    newRules.push_back(newRule);
                }
            } else {
                newRules.push_back(rule);
            }
        }

        eliminateImmediateLeftRecursion(Ai, newRules, result);
        result.nonTerminals.insert(Ai);
    }

    return result;
}
