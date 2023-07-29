#pragma once

#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "Eval.h"
#include "position.h"
#include "NNUE.h"
#include "Search.h"
#include "TranspositionTable.h"
#include <math.h>

extern double K;

void openFens();
double sigmoid(int score);
double evaluationError(std::vector<int> params);
std::vector<int> tune(std::vector<int> initialGuess);
std::vector<int> loadWeights(const char* fileName);