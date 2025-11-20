#include "SymbolTable.h"
#include <iostream>

SymbolTable::SymbolTable() {}

void SymbolTable::addId(const std::string& id, const IdInfo& info) {
    // Insert only if not already present
    table.insert({id, info});
}

const IdInfo* SymbolTable::getIdInfo(const std::string& id) const {
    auto it = table.find(id);
    if (it != table.end()) {
        return &(it->second);
    }
    return nullptr; 
}

void SymbolTable::printAllIds() const {
    std::cout << "=== Symbol Table ===" << std::endl;
    for (const auto& entry : table) {
        std::cout << "Identifier: " << entry.first << std::endl;
    }
}
