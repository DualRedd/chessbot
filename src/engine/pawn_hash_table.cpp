#include "engine/pawn_hash_table.hpp"

PawnHashTable::PawnHashTable(size_t megabytes) {
    size_t bytes = megabytes * 1024ULL * 1024ULL;
    size_t n = bytes / sizeof(PawnTableEntry);

    // power of two size
    size_t pow2 = 16ULL;
    while (pow2 * 2 <= n) pow2 *= 2;
    m_table.resize(pow2);
    m_mask = pow2 - 1;
}

void PawnHashTable::clear() {
    for (auto& entry : m_table)
        entry.key = 0;
}

const PawnTableEntry* PawnHashTable::find(uint64_t key) const {
    size_t idx = key & m_mask;
    if (m_table[idx].key == key)
        return &m_table[idx];
    return nullptr;
}

void PawnHashTable::store(uint64_t key, int32_t eval) {
    size_t idx = key & m_mask;
    m_table[idx].key = key;
    m_table[idx].eval = eval;
}
