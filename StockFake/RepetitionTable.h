#pragma once

#include <cstdint>
#include <string>
#include "def.h"

class RepetitionTable
{
public:
	// dedicated hash table for repetition checking, last 16 bits of Zobrist key are key
	U8 repetitionTable[65536];

	// we still need to store the past zobrists because of collisions
	U64 pastZobrists[1000];
	int zobristIndex = 0;

	// add a position to the hash table
	void addPosition(U64 key);

	// remove a position from the hash table
	void removePosition(U64 key);

	// check if a position has been repeated twice before
	bool positionRepeated(U64 key);

	// clear the table
	void clearRepetitionTable();
};

