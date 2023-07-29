#include "Book.h"
#include "UCI.h"

#pragma warning( disable: 4996 )

FILE* bookFile;
int bookSize;

void openBook(std::string filePath) {
	bookFile = fopen(filePath.c_str(), "rb");

	if (bookFile == NULL) {
		std::cout << "ERROR: Could not open " << filePath << std::endl;
		exit(-1);
	}

	fseek(bookFile, 0, SEEK_END);
	bookSize = ftell(bookFile) / 16;
}

U64 getPolyglotZobrist(Position* p) {
	U64 finalHash = 0ULL;

	// all pieces
	for (int square = 0; square < 64; square++) {
		int piece = polyglotPiece[p->at((Square)square)];

		if (piece != -1) finalHash ^= Random64[piece * 64 + square];
	}

	// en passant
	MoveList moves(*p);
	for (Move m : moves) {
		if (m.is_enPassant()) {
			finalHash ^= Random64[772 + m.to() % 8];
		}
	}

	if (!(p->history[p->ply()].entry & WHITE_OO_MASK)) finalHash ^= Random64[768];
	if (!(p->history[p->ply()].entry & WHITE_OOO_MASK)) finalHash ^= Random64[769];
	if (!(p->history[p->ply()].entry & BLACK_OO_MASK)) finalHash ^= Random64[770];
	if (!(p->history[p->ply()].entry & BLACK_OOO_MASK)) finalHash ^= Random64[771];


	if (p->turn() == WHITE) finalHash ^= Random64[780];

	return finalHash;
}

int findPos(U64 key) {
	int left, right, mid;

	PolyglotEntry polyglotEntry(0, 0, 0);

	left = 0;
	right = bookSize - 1;

	while (left < right) {
		mid = (left + right) / 2;

		readEntry(&polyglotEntry, mid);

		if (key <= polyglotEntry.key) {
			right = mid;
		}
		else {
			left = mid + 1;
		}
	}

	readEntry(&polyglotEntry, left);

	return (polyglotEntry.key == key) ? left : bookSize;
}

U16 bookMove(U64 key) {
	int firstPos;
	PolyglotEntry polyglotEntry(0, 0, 0);

	firstPos = findPos(key);
	if (firstPos == bookSize) return -1;

	int bestMove = -1;
	int bestScore = 0;

	for (int pos = firstPos; pos < bookSize; pos++) {
		readEntry(&polyglotEntry, pos);

		if (polyglotEntry.key != key) break;

		if (polyglotEntry.count > bestScore) {
			bestScore = polyglotEntry.count;
			bestMove = polyglotEntry.move;
		}
	}

	return bestMove;
}


Move getBookMove(Position* p) {
	U16 move = bookMove(getPolyglotZobrist(p));

	if (move == 65535) return Move();

	int endFile = (move & 0b000000000000111) >> 0;
	int endRank = (move & 0b000000000111000) >> 3;
	int startFile = (move & 0b000000111000000) >> 6;
	int startRank = (move & 0b000111000000000) >> 9;
	int promotionPiece = (move & 0b111000000000000) >> 12;

	int startSquare = startRank * 8 + startFile;
	int endSquare = endRank * 8 + endFile;

	if (startSquare == e1 && endSquare == h1) {
		if (p->at((Square)startSquare) == WHITE_KING) {
			endSquare = g1;
		}
	}

	if (startSquare == e1 && endSquare == a1) {
		if (p->at((Square)startSquare) == WHITE_KING) {
			endSquare = c1;
		}
	}

	if (startSquare == e8 && endSquare == h8) {
		if (p->at((Square)startSquare) == BLACK_KING) {
			endSquare = g8;
		}
	}

	if (startSquare == e8 && endSquare == a8) {
		if (p->at((Square)startSquare) == BLACK_KING) {
			endSquare = c8;
		}
	}

	std::string UCIMove = squareToCoordinates[startSquare] + squareToCoordinates[endSquare] + piecePromotion[promotionPiece];

	return UCIToMove(p, UCIMove);
}

U64 readInteger(FILE* file, int size) {
	U64 n;
	int b;

	n = 0;

	for (int i = 0; i < size; i++) {

		b = fgetc(file);

		n = (n << 8) | b;
	}

	return n;
}

void readEntry(PolyglotEntry* entry, int n) {
	fseek(bookFile, n * 16, SEEK_SET);

	entry->key = readInteger(bookFile, 8);
	entry->move = (U16)readInteger(bookFile, 2);
	entry->count = (U16)readInteger(bookFile, 2);
}