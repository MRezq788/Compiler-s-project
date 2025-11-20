#ifndef SYMBOLTABLE_H
#define SYMBOLTABLE_H

#include <string>
#include <unordered_map>

// To be edited with needed info
struct IdInfo {
};

class SymbolTable {
public:
    SymbolTable();  

    // Add identifier with its info
    void addId(const std::string& id, const IdInfo& info);

    // Get identifier info (returns nullptr if not found)
    const IdInfo* getIdInfo(const std::string& id) const;

    // print all identifiers
    void printAllIds() const;

private:
    std::unordered_map<std::string, IdInfo> table; 
};

#endif
