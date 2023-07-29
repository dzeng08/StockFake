#pragma once

// Network architecture is heavily inspired by the Cerebrum library

#include "position.h"
#include <stdio.h>
#include <string.h>
#include "def.h"
#include "immintrin.h"

#define NN_SIZE 128

void loadNN(const char* fileName);

class NNUE {
public:
	Position* position;
	alignas(64) float accumulator[2][NN_SIZE];

	NNUE(Position* p) { position = p; }
	NNUE() { }

	void undoMove(Move m);
	void updateMove(Move m);
	void updateAll();

	int evalNN();
};