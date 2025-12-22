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

    std::string A_dash = A + "_";
    result.nonTerminals.insert(A_dash);

    for (auto& b : beta) {
        b.push_back(A_dash);
        result.productions[A].push_back(b);
    }

    for (auto& a : alpha) {
        a.push_back(A_dash);
        result.productions[A_dash].push_back(a);
    }

    result.productions[A_dash].push_back({EPSILON_SYMBOL});
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

void GrammarTransformer::leftFactorProduction(const std::string& A, const std::vector<std::vector<std::string>>& alternatives, Grammar& result) {
    std::map<std::string, std::vector<std::vector<std::string>>> prefixGroups;

    // Group alternatives by their first symbol
    for (const auto& alt : alternatives) {
        std::string prefix = alt.empty() ? "" : alt[0];
        prefixGroups[prefix].push_back(alt);
    }

    bool factored = false;

    for (const auto& [prefix, group] : prefixGroups) {
        if (group.size() == 1 || prefix == EPSILON_SYMBOL) {
            result.productions[A].push_back(group[0]);
            continue;
        }

        // Find longest common prefix
        std::vector<std::string> commonPrefix = group[0];
        for (size_t i = 1; i < group.size(); ++i) {
            size_t j = 0;
            while (j < commonPrefix.size() && j < group[i].size() && commonPrefix[j] == group[i][j])
                j++;
            commonPrefix.resize(j);
        }

        if (commonPrefix.empty()) {
            for (const auto& alt : group)
                result.productions[A].push_back(alt);
            continue;
        }

        std::string A_dash = A + "'";
        result.nonTerminals.insert(A_dash);

        std::vector<std::string> newAlt = commonPrefix;
        newAlt.push_back(A_dash);
        result.productions[A].push_back(newAlt);

        std::vector<std::vector<std::string>> A_dashNewRules;
        for (const auto& alt : group) {
            std::vector<std::string> suffix(alt.begin() + commonPrefix.size(), alt.end());
            if (suffix.empty()) suffix.push_back(EPSILON_SYMBOL);
            A_dashNewRules.push_back(suffix);
        }

        leftFactorProduction(A_dash, A_dashNewRules, result);

        factored = true;
    }

    if (!factored && result.productions[A].empty())
        result.productions[A] = alternatives;
}

Grammar GrammarTransformer::leftFactor(const Grammar& g) {
    Grammar result;
    result.startSymbol = g.startSymbol;
    result.terminals = g.terminals;
    result.nonTerminals = g.nonTerminals;

    for (const auto& [A, alternatives] : g.productions) {
        leftFactorProduction(A, alternatives, result);
    }

    return result;
}