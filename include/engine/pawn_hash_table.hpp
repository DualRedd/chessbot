#pragma once

#include <cstdint>
#include <vector>

struct PawnTableEntry { // 16 bytes
    uint64_t key;
    int32_t eval;
};

/**
 * Pawn hash table for caching pawn structure evaluations. Always replace scheme.
 */
class PawnHashTable {
public:
    /**
     * @param megabytes size of the table in megabytes
     */
    PawnHashTable(size_t megabytes = 4);

    /**
     * Clear all entries in the table.
     */
    void clear();

    /**
     * Try to find an entry with the given key
     * @param key the key (non-zero)
     * @return PawnTableEntry pointer if found, nullptr if not found
     */
    const PawnTableEntry* find(uint64_t key) const;

    /**
     * Store an entry in the table
     * @param key the key
     * @param eval the pawn evaluation
     */
    void store(uint64_t key, int32_t eval);

private:        
    std::vector<PawnTableEntry> m_table;
    size_t m_mask = 0;
};
