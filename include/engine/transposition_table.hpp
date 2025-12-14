#pragma once

#include <vector>
#include "core/types.hpp"

enum class Bound : uint8_t { Exact=0, Lower=1, Upper=2, None=3 };

struct TTEntry { // 24 bytes aligned
    uint64_t key;
    int32_t score;
    Move best_move;
    int16_t depth;
    Bound bound;
    uint8_t age;
};

/**
 * Open-addressing transposition table with linear probing.
 */
class TranspositionTable {
public:
    /**
     * @param megabytes size of the table in megabytes
     */
    TranspositionTable(size_t megabytes = 256);

    /**
     * Clear all entries in the table.
     */
    void clear();

    /**
     * Try to find an entry with the given key
     * @param key the key (non-zero)
     * @return TTEntry pointer if found, nullptr if not found
     */
    const TTEntry* find(uint64_t key) const;

    /**
     * Store an entry in the table
     * @param key the key
     * @param score the score
     * @param depth the search depth
     * @param bound the bound type
     * @param best_move the best move (32 bit encoded)
     */
    void store(uint64_t key, int32_t score, int16_t depth,
                Bound bound, Move best_move);
    
    /**
     * Increment the age counter for the next search iteration.
     */
    void new_search_iteration();

private:
    std::vector<TTEntry> m_table;
    size_t m_mask = 0;
    const size_t m_probe_window = 4;
    uint8_t m_age = 0;
};