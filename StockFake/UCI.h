#pragma once

#include "def.h"
#include "position.h"
#include "types.h"
#include "Search.h"

Move UCIToMove(Position* p, std::string input);

void UCILoop();