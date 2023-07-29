#include "TranspositionTable.h"

#define EXTRACT_GENERATION(gen) ((GENERATION_CYCLE + generation - gen) & GENERATION_MASK)

TranspositionTable tt;

TTEntry* TranspositionTable::probe(U64 hashKey, bool& found) {
	TTEntry* tte = &(table[getHashIndex(hashKey)].entry[0]);
	TTEntry* toReplace = tte;

	for (int i = 0; i < CLUSTER_SIZE; i++) {
		bool emptyEntry = tte[i].move.to_from() == 0 && tte[i].eval == 0 && tte[i].value == 0;
		if (tte[i].key == hashKey >> 48 || emptyEntry) {
			// refresh the generation of the entry
			tte[i].genBound = U8(generation | (tte[i].genBound & (GENERATION_DELTA - 1)));

			found = !emptyEntry;

			return &tte[i];
		}

		if (i == 0) continue;

		// We replace based on (depth searched - generation * 4)
		// Stockfish has (depth searched - generation * 8) but I'd wager that my engine
		// searches to less than half the depth of Stockfish... so we scale it down
		if (toReplace->depth - EXTRACT_GENERATION(toReplace->genBound) > tte[i].depth - EXTRACT_GENERATION(tte[i].genBound)) {
			toReplace = &tte[i];
		}
	}

	found = false;
	return toReplace;
}

void TTEntry::save(U64 hashKey, Move m, S16 searchValue, S16 evaluation, U8 bound, U8 depth) {
	// Only overwrite move if we have a new move or a new position
	if (m != Move() || (hashKey >> 48) != key) {
		move = m;
	}

	// Write new entry if:
	// Exact bound
	// New position
	// Same position, but greater depth
	if (bound == hashFlagExact || (hashKey >> 48) != key || depth > this->depth) {
		key = U16(hashKey >> 48);
		value = scoreStorage(searchValue);
		eval = evaluation;
		genBound = U8(tt.generation | bound);
		this->depth = depth;
	}
}

//S16 TranspositionTable::readHashEntry(int alpha, int beta, int depth, U64 hashKey, int ply) {
//	TTEntry* tte = &(table[getHashIndex(hashKey)].entry[0]);
//	
//	// cluster size is 4
//	for (int i = 0; i < CLUSTER_SIZE; i++) {
//		if (tte[i].key != (hashKey >> 48)) continue;
//
//		if (tte[i].depth >= depth) {
//			tte[i].genBound = U8(generation | (tte[i].genBound & (GENERATION_DELTA - 1))); // Refresh
//			S16 score = scoreRetrieval(tte[i].value, ply);
//			int flag = tte[i].getHashFlag();
//
//			if (flag == hashFlagExact) {
//				return score;
//			}
//
//			if (flag == hashFlagAlpha && score <= alpha) {
//				return score;
//			}
//
//			if (flag == hashFlagBeta && score >= beta) {
//				return score;
//			}
//		}
//	}
//
//	return noHashEntry;
//}
//
//void TranspositionTable::writeHashEntry(int score, int depth, int hashFlag, Move best, U64 hashKey, int ply) {
//	TTEntry* tte = &(table[getHashIndex(hashKey)].entry[0]);
//	
//	int c1, c2, c3;
//	int toReplace = 0;
//
//	for (int i = 0; i < CLUSTER_SIZE; i++) {
//		if (tte[i].key == 0 || tte[i].key == hashKey >> 48) {
//			TTEntry newEntry;
//			newEntry.depth = (U8)depth;
//			newEntry.value = scoreStorage((S16)score);
//			newEntry.genBound = generation | hashFlag;
//			newEntry.key = (U16)(hashKey >> 48);
//			newEntry.move = best;
//			
//			tte[i] = newEntry;
//
//			return;
//		}
//
//		// we want to replace this entry if:
//		// 1. the previous candidate for a replacement was from this generation
//		// 2. the pervious candidate for a replacement had a deeper depth
//		
//		// we don't want to replace this entry if:
//		// 1. this candidate was from this generation, or it has an exact bound
//
//		// so we replace an entry if:
//		// #2 for replace
//		// #1 and #2 for replace
//		// #1 and #2 for replace, and #1 for don't replace
//		if (tte[toReplace].depth - ((GENERATION_CYCLE + generation - tte[toReplace].genBound) & GENERATION_MASK)
//		> tte[i].depth - ((GENERATION_CYCLE + generation - tte[i].genBound) & GENERATION_MASK))
//			toReplace = i;
//		//c1 = (((GENERATION_CYCLE + generation - tte[toReplace].genBound) & GENERATION_MASK) ? 0 : 2);
//		//c2 = (!((GENERATION_CYCLE + generation - tte[i].genBound) & GENERATION_MASK) || tte[i].getHashFlag() == hashFlagExact) ? -2 : 0;
//		//c3 = (tte[i].depth < tte[toReplace].depth) ? 1 : 0;
//
//		//if (c1 + c2 + c3 > 0) {
//		//	toReplace = i;
//		//}
//	}
//
//	if (hashFlag == hashFlagExact || hashKey >> 48 != tte[toReplace].key || depth > tte[toReplace].depth - 2) {
//		TTEntry newEntry;
//		newEntry.depth = (U8)depth;
//		newEntry.value = scoreStorage((S16)score);
//		newEntry.genBound = generation | hashFlag;
//		newEntry.key = (U16)(hashKey >> 48);
//		newEntry.move = best;
//
//		tte[toReplace] = newEntry;
//	}
//
//	return;
//}

//Move TranspositionTable::getStoredMove(U64 hashKey) {
//	TTEntry* tte = &(table[getHashIndex(hashKey)].entry[0]);
//	
//	for (int i = 0; i < CLUSTER_SIZE; i++) {
//		if (tte[i].key == hashKey >> 48) {
//			return tte[i].move;
//		}
//	}
//	
//	return Move();
//}