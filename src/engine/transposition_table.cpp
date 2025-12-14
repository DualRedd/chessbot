#include "engine/transposition_table.hpp"
#include <limits>

TranspositionTable::TranspositionTable(size_t megabytes) {
    size_t bytes = megabytes * 1024ULL * 1024ULL;
    size_t n = bytes / sizeof(TTEntry);

    // power of two size
    size_t pow2 = 16ULL;
    while (pow2 * 2 <= n) pow2 *= 2;
    m_table.resize(pow2);
    m_mask = pow2 - 1;
}

void TranspositionTable::clear() {
    for (auto& entry : m_table)
        entry.key = 0;
}

const TTEntry* TranspositionTable::find(uint64_t key) const {
    size_t idx = key & m_mask;
    for (size_t i = 0; i < m_probe_window; ++i) {
        if (m_table[idx].key == key) return &m_table[idx];
        if (m_table[idx].key == 0) return nullptr;
        idx = (idx + 1) & m_mask;
    }
    return nullptr;
}

void TranspositionTable::store(uint64_t key, int32_t score, int16_t depth,
                                Bound bound, Move best_move) {
    size_t idx = key & m_mask;
    size_t replace_idx = idx;

    int best_keep_score = std::numeric_limits<int>::max();
    for (size_t i = 0; i < m_probe_window; ++i) {
        TTEntry &entry = m_table[idx];
        if (entry.key == 0) { replace_idx = idx; break; } // empty slot: free to use
        if (entry.key == key) { replace_idx = idx; break; } // same key: overwrite

        // prefer entries with same age and deeper depth
        int keep_score = entry.depth + ((entry.age == m_age) ? 100000 : 0);

        // replace the entry with the smallest keep_score
        if (keep_score < best_keep_score) {
            best_keep_score = keep_score;
            replace_idx = idx;
        }

        idx = (idx + 1) & m_mask;
    }

    auto &dst = m_table[replace_idx];
    dst.key = key;
    dst.score = score;
    dst.depth = depth;
    dst.bound = bound;
    dst.best_move = best_move;
    dst.age = m_age;
}

void TranspositionTable::new_search_iteration() { m_age += 1; }
