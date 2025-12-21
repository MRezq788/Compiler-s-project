#ifndef PARSERGEN_H
#define PARSERGEN_H

#include "CFG.h"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility> // for pair

using namespace std;

// The End-of-Input marker
const string END_MARKER = "$";

// Global FOLLOW map (NonTerminal -> Set of Terminals)
extern map<string, set<string>> FOLLOW;

// Parsing Table Type
// Key: {NonTerminal, Terminal}
// Value: The RHS of the production rule (e.g., {"int", "id", ";"})
typedef map<pair<string, string>, vector<string>> ParsingTable;

// Function Prototypes
void computeFollow(Grammar& grammar);
ParsingTable buildParsingTable(Grammar& grammar);

// Visualization Helpers
void printFollow();
void printParsingTable(const ParsingTable& table);

#endif