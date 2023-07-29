#include "Search.h"

bool keepSearching = true;
U64 startTime = getTimeMillis();
int timeLimit = 2147483647;
int maxDepth = 100;

Move Search::startSearch() {
	Move moveFromBook = getBookMove(position);
	if (moveFromBook != Move()) {
		std::cout << "bestmove " << moveFromBook.toString() << std::endl;
		return moveFromBook;
	}

	// only one legal move? don't waste our time searching
	MoveList checkOneLegalMove(*position);
	if (checkOneLegalMove.size() == 1) {
		std::cout << "bestmove " << (*checkOneLegalMove.begin()).toString() << std::endl;
		return *checkOneLegalMove.begin();
	}

	startTime = getTimeMillis();

	int score = 0;
	keepSearching = true;
	SearchInfo info;
	info.p = position;

	info.net = net;
	net->updateAll();

	Move bestMove;

	for (int currentDepth = 1; currentDepth <= maxDepth; currentDepth++) {
		int oldNodes = info.nodes;

		if (currentDepth == 0) {
			score = negamax(-32000, 32000, currentDepth, &info);
		}
		else {
			//int delta = 15;
			//int alpha = std::max(score - delta, -MATE);
			//int beta = std::min(score + delta, MATE);

			//for (;; delta += delta / 2) {
			//	score = negamax(alpha, beta, currentDepth, &info);

			//	if (score <= alpha) {
			//		beta = (alpha + beta) / 2;
			//		alpha = std::max(alpha - delta, -MATE);
			//	}
			//	else if (score >= beta) {
			//		alpha = (alpha + beta) / 2;
			//		beta = std::min(beta + delta, MATE);
			//	}
			//	else {
			//		break;
			//	}
			//}

			int alpha = score - 50;
			int beta = score + 50;

			score = negamax(alpha, beta, currentDepth, &info);

			if (score <= alpha || score >= beta) {
				score = negamax(-32000, 32000, currentDepth, &info);
			}
		}

		if (keepSearching) {
			info.followPV = true;

			bestMove = info.pvTable[0][0];

			std::cout << "info depth " << currentDepth << " nodes " << info.nodes << " score";

			if (score > MATE - 100) std::cout << " mate " << (int)((MATE - score) / 2 + 1);
			else if (score < -MATE + 100) std::cout << " mate " << (int)((score - MATE) / 2 - 1);
			else std::cout << " cp " << score;

			std::cout << " pv ";

			//std::cout << "Depth: " << currentDepth << "\tNodes: " << info.nodes - oldNodes << "\tEval: " << score << " \t";

			for (int i = 0; i < info.pvLength[0]; i++) {
				std::cout << info.pvTable[0][i].toString() << " ";
			}

			std::cout << std::endl;

			// we probably don't have enough time to search another ply
			// just give up
			// we don't have partial search anyways
			if (getTimeMillis() - startTime >= timeLimit * 0.9) {
				break;
			}
		}
		else {
			break;
		}
	}

	std::cout << "bestmove " << bestMove.toString() << std::endl;
	// std::cout << "Nodes: " << info.nodes << " Transpositions: " << info.transpositions << "\tEval: " << score << std::endl;
	tt.newSearch();

	return info.pvTable[0][0];
}

int Search::negamax(int alpha, int beta, int depth, SearchInfo* info) {
	static const int EVAL_MARGIN[] = { 0, 130, 264, 410, 510, 672, 840 };
	static const int RAZOR_MARGIN[] = { 0, 229, 438, 495, 878, 1094 };

	if (!keepSearching || (((info->nodes & 4096) == 0) && getTimeMillis() - startTime >= timeLimit)) {
		keepSearching = false;
		return 0;
	}

	// set length of pv table
	info->pvLength[info->ply] = info->ply;

	// drawn position
	if (info->p->getMoveRule() >= 100 || info->p->positionRepeated()) return 0;

	// our arrays are overflowing
	if (info->ply >= MAX_PLY - 1) {
		return eval(info->p, info->net);
	}

	// check extension
	bool inCheck = info->p->inCheck(info->p->turn());
	if (inCheck) depth++;

	// for hash
	int hashFlag = hashFlagAlpha;

	// is PV node
	bool isPVNode = beta - alpha > 1;

	// transposition table here
	bool found;
	TTEntry* hashEntry = tt.probe(info->p->get_hash(), found);
	int ttVal = scoreRetrieval(hashEntry->value, info->p->ply());
	Move ttMove = found ? hashEntry->move : Move();

	bool useTT = false;
	if (hashEntry->getHashFlag() == hashFlagExact) useTT = true;
	if (hashEntry->getHashFlag() == hashFlagAlpha && ttVal <= alpha) useTT = true;
	if (hashEntry->getHashFlag() == hashFlagBeta && ttVal >= beta) useTT = true;
	if (ttVal == VALUE_NONE) useTT = false;

	// we have a hit
	if (found
		&& info->excludedMove == Move()
		&& !isPVNode
		&& hashEntry->depth >= depth
		&& ttVal != VALUE_NONE
		&& info->ply != 0) {
		// now check if the score is valid due to cutoffs
		if (useTT) {
			if (info->p->ply() < 90) {
				info->transpositions++;
				return ttVal;
			}
		}
	}

	// quiescence
	if (depth <= 0) {
		return quiescence(alpha, beta, info);
	}

	info->nodes++;

	// static evaluation score
	int staticEval;
	if (found) {
		// use data from transposition table for better static evaluation
		if (useTT) {
			staticEval = ttVal;
		}
		else {
			staticEval = hashEntry->eval;
		}
	}
	else {
		staticEval = eval(info->p, info->net);
		hashEntry->save(info->p->get_hash(), ttMove, VALUE_NONE, staticEval, 0, 0);
	}

	// static null move pruning
	if (depth < 3 && !isPVNode && !inCheck && beta > -MATE - 100) {
		int evalMargin = 120 * depth;

		if (staticEval - evalMargin >= beta) {
			return staticEval - evalMargin;
		}
	}

	// razoring
	if (!isPVNode && !inCheck && depth <= 3) {
		int score = staticEval + 125;
		int newScore = 0;

		if (score < beta) {
			if (depth == 1) {
				newScore = quiescence(alpha, beta, info);

				return (newScore > score) ? newScore : score;
			}

			score += 175;

			if (score < beta) {
				newScore = quiescence(alpha, beta, info);

				if (newScore < beta) {
					return (newScore > score) ? newScore : score;
				}
			}
		}
	}

	// null move pruning
	if (depth >= 3
		&& !inCheck
		&& info->ply != 0
		&& info->excludedMove == Move()
		&& (info->p->bitboard_of(WHITE_KNIGHT) | info->p->bitboard_of(BLACK_KNIGHT) |
			info->p->bitboard_of(WHITE_BISHOP) | info->p->bitboard_of(BLACK_BISHOP) |
			info->p->bitboard_of(WHITE_ROOK) | info->p->bitboard_of(BLACK_ROOK) |
			info->p->bitboard_of(WHITE_QUEEN) | info->p->bitboard_of(BLACK_QUEEN)) != 0) {
		int R = 2;
		int numPieces = 0;
		if (depth > 7) {
			for (int piece = WHITE_KNIGHT + info->p->turn() * 8; piece <= WHITE_QUEEN + info->p->turn() * 8; piece++) {
				numPieces += countBits(info->p->bitboard_of((Piece)piece));
			}
			if (numPieces >= 2) R++;
		}

		info->ply++;

		info->net->updateMove(Move());
		int bound = beta - 10;

		int score = -negamax(-bound, -bound + 1, depth - R - 1, info);

		info->ply--;
		info->net->undoMove(Move());

		if (score >= bound) {
			return beta;
		}

		if (!keepSearching) return 0;
	}

	int score = -1000000;
	Move bestMove;
	MoveList moves(*(info->p));
	if (info->followPV) enablePVScoring(moves, info);
	int scores[218];
	assignScores(moves, scores, alpha, beta, depth, ttMove, info);

	// stalemate / checkmate check
	if (moves.size() == 0) {
		if (inCheck) {
			return -MATE + info->ply;
		}
		else {
			return 0;
		}
	}

	// for LMR
	int movesSearched = 0;

	// for Principal Variation Search
	bool foundPV = false;

	while (movesSearched < moves.size()) {
		Move m = pickMove(moves, scores);
		if (m == info->excludedMove) {
			movesSearched++;
			continue;
		}

		int newDepth = depth - 1;

		// singular extensions
		// I can't get singular extensions to work
		//if (info->ply != 0 // necessary
		// && depth >= 3 
		// && m == ttMove // necessary
		// && info->excludedMove == Move() // necessary
		// && abs(ttVal) < MATE - 100
		// && (hashEntry->getHashFlag() & hashFlagBeta)
		// && hashEntry->depth >= depth - 2) {
		//	int singularBeta = ttVal - 5 * depth;
		//	int singularDepth = (depth - 1) / 2;

		//	info->excludedMove = ttMove;
		//	int result = negamax(singularBeta - 1, singularBeta, singularDepth, info);
		//	info->excludedMove = Move();

		//	if (result < singularBeta) {
		//		newDepth++;
		//	}
		//	else if (singularBeta >= beta) {
		//		return singularBeta;
		//	}
		//}

		info->ply++;
		info->net->updateMove(m);

		bool inCheckAfter = info->p->inCheck(info->p->turn());

		bool doPVS = foundPV;
		bool doLMR = movesSearched >= 4
			&& depth >= 3
			&& !inCheck
			&& !inCheckAfter
			&& !m.is_capture()
			&& !m.is_promotion();
		int R = 1;
		if (movesSearched >= 10 && !isPVNode && depth >= 6) R++;

		int alphaBound = -beta;
		int betaBound = -alpha;

		if (doLMR) {
			newDepth -= R;
			doPVS = true;
		}

		if (doPVS) {
			alphaBound = -alpha - 1;
			betaBound = -alpha;
		}

		score = -negamax(alphaBound, betaBound, newDepth, info);

		bool reSearch = false;
		if (doPVS && score > alpha && score < beta) {
			alphaBound = -beta;
			betaBound = -alpha;
			reSearch = true;
		}

		if (doLMR && score > alpha) {
			newDepth += R;
			reSearch = true;
		}

		if (reSearch) {
			score = -negamax(alphaBound, betaBound, newDepth, info);
		}

		//// principal variation search
		//if (foundPV) {
		//	score = -negamax(-alpha - 1, -alpha, newDepth, info);

		//	if (score > alpha && score < beta) {
		//		score = -negamax(-beta, -alpha, newDepth, info);
		//	}
		//}
		//else {
		//	// no LMR
		//	if (movesSearched == 0) {
		//		score = -negamax(-beta, -alpha, newDepth, info);
		//	}
		//	else {
		//		// LMR
		//		if (movesSearched >= 4
		//			&& depth >= 3
		//			&& !inCheck
		//			&& !inCheckAfter
		//			&& !m.is_capture()
		//			&& !m.is_promotion()) {
		//			int R = 1;
		//			if (movesSearched >= 10 && !isPVNode && depth >= 6) R++;
		//			score = -negamax(-alpha - 1, -alpha, newDepth - R, info);
		//		}
		//		else {
		//			// hack to force a research
		//			score = alpha + 1;
		//		}

		//		// LMR failed, research
		//		if (score > alpha) {
		//			score = -negamax(-alpha - 1, -alpha, newDepth, info);

		//			if (score > alpha && score < beta) {
		//				score = -negamax(-beta, -alpha, newDepth, info);
		//			}
		//		}
		//	}
		//}

		info->net->undoMove(m);
		info->ply--;

		movesSearched++;

		if (!keepSearching) return 0;

		if (score > alpha) {
			bestMove = m;

			alpha = score;
			hashFlag = hashFlagExact;

			foundPV = true;

			// write pv move and copy line
			info->pvTable[info->ply][info->ply] = m;
			for (int next = info->ply + 1; next < info->pvLength[info->ply + 1]; next++) {
				info->pvTable[info->ply][next] = info->pvTable[info->ply + 1][next];
			}
			info->pvLength[info->ply] = info->pvLength[info->ply + 1];

			if (score >= beta) {
				// write hash entry
				hashEntry->save(info->p->get_hash(), m, beta, staticEval, hashFlagBeta, depth);

				if (!m.is_capture()) {
					// history moves heuristic
					info->history[info->p->at(m.from())][m.to()] += depth * depth + depth - 1;

					// killer moves heuristic
					if (m != info->killers[0][info->ply]) {
						info->killers[1][info->ply] = info->killers[0][info->ply];
						info->killers[0][info->ply] = m;
					}
				}

				return beta;
			}
		}
	}

	hashEntry->save(info->p->get_hash(), bestMove, alpha, staticEval, hashFlag, depth);

	return alpha;
}

int Search::quiescence(int alpha, int beta, SearchInfo* info) {
	if (!keepSearching || (((info->nodes & 4096) == 0) && getTimeMillis() - startTime >= timeLimit)) {
		keepSearching = false;
		return 0;
	}

	// transposition table
	bool found;
	TTEntry* hashEntry = tt.probe(info->p->get_hash(), found);
	int ttVal = scoreRetrieval(hashEntry->value, info->p->ply());
	Move ttMove = found ? hashEntry->move : Move();

	bool useTT = false;
	if (hashEntry->getHashFlag() == hashFlagExact) useTT = true;
	if (hashEntry->getHashFlag() == hashFlagAlpha && ttVal <= alpha) useTT = true;
	if (hashEntry->getHashFlag() == hashFlagBeta && ttVal >= beta) useTT = true;
	if (ttVal == VALUE_NONE) useTT = false;

	// we have a hit
	if (found && ttVal != VALUE_NONE) {
		// now check if the score is valid due to cutoffs
		if (useTT) {
			if (info->p->ply() < 90) {
				info->transpositions++;
				return ttVal;
			}
		}
	}

	info->nodes++;

	int evaluation;
	if (found) {
		// use data from transposition table for better static evaluation
		if (useTT) {
			evaluation = ttVal;
		}
		else { // or, save some computation. no need to evaluate NNUE if we don't have to
			evaluation = hashEntry->eval;
		}
	}
	else {
		evaluation = eval(info->p, info->net);
		hashEntry->save(info->p->get_hash(), ttMove, VALUE_NONE, evaluation, 0, 0);
	}

	if (info->ply >= MAX_PLY - 1) {
		return evaluation;
	}

	// beta cutoff
	if (evaluation >= beta) {
		return beta;
	}

	// delta pruning: prune tree if the position is too hopeless
	int bigDelta = 1075;
	if (evaluation < alpha - bigDelta) {
		return alpha;
	}

	if (evaluation > alpha) {
		alpha = evaluation;
	}

	MoveList moves(*(info->p));
	int scores[218];
	assignScores(moves, scores, alpha, beta, 0, ttMove, info);
	int score = 0;

	int hashFlag = hashFlagAlpha;
	Move bestMove;

	for (int index = 0; index < moves.size(); index++) {
		Move m = pickMove(moves, scores);
		if (!m.is_capture() && !m.is_promotion()) continue;

		// Failed pruning attempts
		// if (!m.is_promotion() && evaluation + 200 + eg_value[info->p->at(m.to()) & ~0b1000] <= alpha) continue;

		// SEE pruning
		//int moveSEE = SEE(info->p, m);
		//if (moveSEE < -50) continue;
		//if (evaluation + 200 <= alpha && moveSEE <= 0) continue;

		info->ply++;
		info->net->updateMove(m);
		score = -quiescence(-beta, -alpha, info);
		info->net->undoMove(m);
		info->ply--;

		if (!keepSearching) return 0;

		if (score > alpha) {
			alpha = score;
			bestMove = m;
			hashFlag = hashFlagExact;

			if (score >= beta) {
				hashEntry->save(info->p->get_hash(), m, beta, evaluation, hashFlagBeta, 0);

				return beta;
			}
		}
	}

	hashEntry->save(info->p->get_hash(), bestMove, alpha, evaluation, hashFlag, 0);

	return alpha;
}

void Search::enablePVScoring(MoveList& moveList, SearchInfo* info) {
	info->followPV = false;

	for (Move m : moveList) {
		if (info->pvTable[0][info->ply] == m) {
			info->scorePV = true;
			info->followPV = true;
		}
	}
}

int Search::scoreMove(Move move, Move ttMove, SearchInfo* info) {
	// score principal variation from previous depths very high
	// in theory, should be the same as ttMove, but there are collisions
	if (info->scorePV) {
		if (info->pvTable[0][info->ply] == move) {
			info->scorePV = false;
			return 30000;
		}
	}

	// transposition table move from previous depths / searches
	if (move == ttMove) {
		return 25000;
	}

	// if it's a promotion, it's gotta be good
	if (move.is_promotion()) {
		return 15000 + move.promotion();
	}

	// score all captures based on MVV-LVA
	// no SEE. I could try it but I think it's way too slow
	if (move.is_capture()) {
		int targetPiece = info->p->at(move.to());
		int sourcePiece = info->p->at(move.from());

		targetPiece &= ~0b1000;
		sourcePiece &= ~0b1000;

		// MVV-LVA
		// we want to take valuable pieces with the least valuable pieces
		return 10000 + targetPiece * 100 + (6 - sourcePiece);
	}
	else {
		// Killer 1, then Killer 2, then History
		if (info->killers[0][info->ply] == move) {
			return 9000;
		}
		else if (info->killers[1][info->ply] == move) {
			return 8000;
		}
		else {
			return info->history[info->p->at(move.from())][move.to()];
		}
	}
}

void Search::assignScores(MoveList& moveList, int* moveScores, int alpha, int beta, int depth, Move ttMove, SearchInfo* info) {
	// TODO: implement later?
	// bool useIID = false;

	int i = 0;
	for (Move m : moveList) {
		moveScores[i] = scoreMove(m, ttMove, info);
		// if (moveScores[i] >= 20000) useIID = false;
		i++;
	}
}

Move Search::pickMove(MoveList& moveList, int* scores) {
	// we don't sort because we're not necessarily going to use every move
	// instead, we select and iterate through the whole list each time
	int highest = -1;
	int bestIndex = -1;
	for (int i = 0; i < moveList.size(); i++) {
		if (scores[i] > highest) {
			highest = scores[i];
			bestIndex = i;
		}
	}

	scores[bestIndex] = -2;

	return moveList.begin()[bestIndex];
}