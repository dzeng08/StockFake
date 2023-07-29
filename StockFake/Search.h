#pragma once
#include "position.h"
#include "Eval.h"
#include <chrono>
#include <algorithm>
#include <iterator>
#include "TranspositionTable.h"
#include "Book.h"
#include "NNUE.h"

inline U64 getTimeMillis() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

class SearchInfo {
public:
	bool scorePV;
	bool followPV;

	Position* p;
	NNUE* net;
	Move excludedMove = Move();

	int nodes = 0;
	int transpositions = 0;
	Move killers[2][MAX_PLY];
	int history[14][64];
	int pvLength[MAX_PLY];
	Move pvTable[MAX_PLY][MAX_PLY];
	int ply = 0;
};

class Search
{
public:
	Position* position;
	NNUE* net;
	NNUE notPointerNet;

	Search(Position* pos) {
		position = pos;
		notPointerNet = NNUE(pos);
		net = &notPointerNet;
	}

	Move startSearch();
	int negamax(int alpha, int beta, int depth, SearchInfo* info);
	int quiescence(int alpha, int beta, SearchInfo* info);

private:
	void enablePVScoring(MoveList& moveList, SearchInfo* info);
	int scoreMove(Move move, Move ttMove, SearchInfo* info);
	void assignScores(MoveList& moveList, int* moveScores, int alpha, int beta, int depth, Move ttMove, SearchInfo* info);
	Move pickMove(MoveList& moveList, int* scores);
};

extern int timeLimit;
extern int maxDepth;