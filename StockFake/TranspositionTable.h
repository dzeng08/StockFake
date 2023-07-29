#pragma once

#include "def.h"
#include "types.h"
#include <malloc.h>

#define CLUSTER_SIZE 3

// 10 byte TTEntry
// Similar to stockfish
// but we have 16 bits for generation, no bits for value, and 8 bits for flags
// and 32 bits for the key

struct TTEntry {
	U8 getHashFlag() { return genBound & 0b11; }
	void save(U64 hashKey, Move m, S16 searchValue, S16 evaluation, U8 bound, U8 depth);

	U16 key;
	Move move;
	S16 value;
	S16 eval;
	U8 genBound;
	U8 depth;
};

struct Cluster {
	TTEntry entry[CLUSTER_SIZE];
	char padding[2];
};

class TranspositionTable
{
public:
	// Constants used to refresh the hash table periodically
	static constexpr unsigned GENERATION_BITS = 2;                                // nb of bits reserved for other things
	static constexpr int      GENERATION_DELTA = (1 << GENERATION_BITS);           // increment for generation field
	static constexpr int      GENERATION_CYCLE = 255 + (1 << GENERATION_BITS);     // cycle length
	static constexpr int      GENERATION_MASK = (0xFF << GENERATION_BITS) & 0xFF; // mask to pull out generation number

	U8 generation;
	Cluster* table;
	U32 clusterCount;
	U64 indexMask;


	TranspositionTable() {
		clusterCount = (((U32)(128)) << 20) / sizeof(Cluster);
		table = static_cast<Cluster*>(_aligned_malloc(clusterCount * sizeof(Cluster), 2 * 1024 * 1024));

		if (table == NULL) std::cout << "Memory Allocation failed";

		generation = 0;
		indexMask = clusterCount - 1;
	}

	void newSearch() { generation += GENERATION_DELTA; }

	TTEntry* probe(U64 hashKey, bool& found);

	//S16 readHashEntry(int alpha, int beta, int depth, U64 hashKey, int ply);
	//void writeHashEntry(int score, int depth, int hashFlag, Move best, U64 hashKey, int ply);
	//Move getStoredMove(U64 hashKey);
	U64 getHashIndex(U64 hashKey) { return hashKey & indexMask; }

	void resetTable() { memset(table, 0, clusterCount * sizeof(Cluster)); generation = 0; }

	void resize(int megabytes) {
		generation = 0;

		int index;
		while (megabytes) {
			index = getLS1BIndex(megabytes);
			megabytes &= ~(1ULL << index);
		}

		clusterCount = (1ULL << (20 + index)) / sizeof(Cluster);
		indexMask = clusterCount - 1;

		_aligned_free(table);
		table = static_cast<Cluster*>(_aligned_malloc(clusterCount * sizeof(Cluster), 2 * 1024 * 1024));
		if (table == NULL) std::cout << "Memory Allocation failed";
	}
};


inline S16 scoreStorage(S16 score) {
	if (score == VALUE_NONE) return VALUE_NONE;

	if (score >= MATE - 100) {
		return MATE;
	}
	else if (score <= -MATE + 100) {
		return -MATE;
	}

	return score;
}

inline S16 scoreRetrieval(S16 score, int ply) {
	if (score == VALUE_NONE) return VALUE_NONE;

	if (score == MATE) {
		return MATE - ply;
	}
	else if (score == -MATE) {
		return -(MATE - ply);
	}

	return score;
}

extern TranspositionTable tt;