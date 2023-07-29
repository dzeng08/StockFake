#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include "def.h"
#include "position.h"

struct PolyglotEntry {
	U64 key;
	U16 move;
	U16 count;

	PolyglotEntry(U64 Key, U16 Move, U16 Count) {
		key = Key;
		move = Move;
		count = Count;
	}
};

extern FILE* bookFile;
extern int bookSize;

void openBook(std::string filePath);
U64 getPolyglotZobrist(Position* p);
int findPos(U64 key);
U16 bookMove(U64 key);
Move getBookMove(Position* p);

U64 readInteger(FILE* file, int size);
void readEntry(PolyglotEntry* entry, int n);