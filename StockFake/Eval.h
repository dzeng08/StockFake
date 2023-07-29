#pragma once

#include "position.h"
#include "NNUE.h"
#include <iostream>
#include <vector>

extern int eg_value[6];
extern int mg_value[6];

int eval(Position* pos, NNUE* net);
int SEE(Position* pos, Move m);
void loadParams(std::vector<int> params);