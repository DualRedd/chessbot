#pragma once

#include "registry.hpp"
#include "../core/bitboard.hpp"

void registerRandomAI();

class RandomAI : public AIPlayer {
public:
    RandomAI() = default;
    UCI getMove(const FEN& fen) override;
};
