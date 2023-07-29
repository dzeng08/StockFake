#include "RepetitionTable.h"

void RepetitionTable::addPosition(U64 key) {
	repetitionTable[key >> 48]++;
	pastZobrists[zobristIndex] = key;
	zobristIndex++;
}

void RepetitionTable::removePosition(U64 key) {
	repetitionTable[key >> 48]--;
	zobristIndex--;
}

bool RepetitionTable::positionRepeated(U64 key) {
	// check for three-fold in the table
	if (repetitionTable[key >> 48] >= 3) {
		// if we have three-fold in the table, loop over all past zobrists
		// in case of collisions
		int occurrences = 0;

		for (int i = 0; i < zobristIndex; i++) {
			if (pastZobrists[i] == key) {
				occurrences++;
			}
		}

		if (occurrences >= 3) return true;
	}

	return false;
}

void RepetitionTable::clearRepetitionTable() {
	zobristIndex = 0;
	memset(repetitionTable, 0, sizeof(repetitionTable));
}