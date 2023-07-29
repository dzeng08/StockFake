#include "Eval.h"

#define FLIP(sq) ((sq)^56)
#define OTHER(side) ((side)^ 1)
#define PCOLOR(p) (p & 1)

int mg_value[6] = { 100, 300, 325, 500, 900, 0 };
int eg_value[6] = { 100, 300, 325, 500, 900, 0 };

int mopUpEval(int whiteMS, int blackMS, int whiteKP, int blackKP) {
    float multiplier = (float)(whiteMS - blackMS) / (whiteMS + blackMS + 100) * 10;

    if (multiplier > 5) multiplier = 5;
    else if (multiplier < -5) multiplier = -5;

    int friendlyKing, enemyKing;

    if (multiplier > 0) {
        friendlyKing = whiteKP;
        enemyKing = blackKP;
    }
    else {
        friendlyKing = blackKP;
        enemyKing = whiteKP;
    }

    return (manhattanCenterDistance(enemyKing) * 10 + (14 - manhattanDistance(friendlyKing, enemyKing)) * 4) * multiplier;
}

int eval(Position* pos, NNUE* net)
{
    int whiteMaterialCount = 0;
    int blackMaterialCount = 0;

    // get material counts
    for (int sq = 0; sq < 64; ++sq) {
        int pc = pos->at((Square)sq);
        if (pc == NO_PIECE) continue;
        if (pc & 0b1000) blackMaterialCount += mg_value[pc & ~0b1000];
        else whiteMaterialCount += mg_value[pc];
    }

    int side = pos->turn();

    int finalScore = (whiteMaterialCount - blackMaterialCount) * (1 - (side << 1));

    const int whiteKingSquare = getLS1BIndex(pos->bitboard_of((Piece)(WHITE_KING)));
    const int blackKingSquare = getLS1BIndex(pos->bitboard_of((Piece)(BLACK_KING)));

    float nnueProp = 1;

    if (abs(finalScore) < 800) nnueProp = 1;
    else if (abs(finalScore) < 1300) nnueProp = 1 - (((float)abs(finalScore) - 800) / 1000);
    else nnueProp = 0.5;

    // PSQT evaluation * (1 - nnue proportion)
    // NNUE evaluation * (nnue proportion)
    // mop up eval scaled to side to move
    // all scaled by fifty move rule
    return (finalScore * (1 - nnueProp) + net->evalNN() * nnueProp + mopUpEval(whiteMaterialCount, blackMaterialCount, whiteKingSquare, blackKingSquare) * (1 - (side << 1))) * (100 - pos->getMoveRule()) / 100;
}

// Static Exchange Evaluation
// Literally not used at all
// Any pruning attempts only lost elo
int SEE(Position* pos, Move m) {
    int from = m.from();
    int to = m.to();

    int target = pos->at((Square)to);
    int source = pos->at((Square)from);

    int gain[32];
    int d = 0;

    U64 bishops = pos->bitboard_of(WHITE_BISHOP) | pos->bitboard_of(BLACK_BISHOP);
    U64 rooks = pos->bitboard_of(WHITE_ROOK) | pos->bitboard_of(BLACK_ROOK);
    U64 queens = pos->bitboard_of(WHITE_QUEEN) | pos->bitboard_of(BLACK_QUEEN);
    U64 pawns = pos->bitboard_of(WHITE_PAWN) | pos->bitboard_of(WHITE_PAWN);

    U64 diagonalsXRay = bishops | queens | pawns;
    U64 diagonals = bishops | queens;
    U64 orthogonals = rooks | queens;

    U64 fromSet = 1ULL << from;
    U64 occ = pos->all_pieces<WHITE>() | pos->all_pieces<BLACK>();
    U64 attadef = pos->attackers_from<WHITE>((Square)to, occ) | pos->attackers_from<BLACK>((Square)to, occ);
    gain[d] = eg_value[target & ~0b1000];

    do {
        d++;
        gain[d] = eg_value[source & ~0b1000] - gain[d - 1];
        if (std::max(-gain[d - 1], gain[d]) < 0) break;
        attadef ^= fromSet;
        occ ^= fromSet;

        if (fromSet & diagonalsXRay) {
            diagonals ^= fromSet;
            attadef |= attacks<BISHOP>((Square)target, occ) & diagonals;
        }
        else if (fromSet & orthogonals) {
            orthogonals ^= fromSet;
            attadef |= attacks<ROOK>((Square)target, occ) & orthogonals;
        }

        fromSet = 0;
        for (target = WHITE_PAWN + ((d & 1) << 3); target <= WHITE_KING + ((d & 1) << 3); target++) {
            U64 subset = attadef & pos->bitboard_of((Piece)target);
            if (subset) {
                fromSet = 1ULL << (getLS1BIndex(subset));
                break;
            }
        }
    } while (fromSet);

    while (--d) {
        gain[d - 1] = -std::max(-gain[d - 1], gain[d]);
    }

    return gain[0];
}

void loadParams(std::vector<int> params) {
    //int index = 0;
    //for (int i = 0; i < 64; i++, index++) {
    //    mg_pawn_table   [i] = params[index];
    //    eg_pawn_table   [i] = params[index + 64];
    //    mg_knight_table [i] = params[index + 64 * 2];
    //    eg_knight_table [i] = params[index + 64 * 3];
    //    mg_bishop_table [i] = params[index + 64 * 4];
    //    eg_bishop_table [i] = params[index + 64 * 5];
    //    mg_rook_table   [i] = params[index + 64 * 6];
    //    eg_rook_table   [i] = params[index + 64 * 7];
    //    mg_queen_table  [i] = params[index + 64 * 8];
    //    eg_queen_table  [i] = params[index + 64 * 9];
    //    mg_king_table   [i] = params[index + 64 * 10];
    //    eg_king_table   [i] = params[index + 64 * 11];
    //}

    //index = 64 * 12;

    //// what are loops?
    //mg_value[0] = index++;
    //mg_value[1] = index++;
    //mg_value[2] = index++;
    //mg_value[3] = index++;
    //mg_value[4] = index++;
    //
    //eg_value[0] = index++;
    //eg_value[1] = index++;
    //eg_value[2] = index++;
    //eg_value[3] = index++;
    //eg_value[4] = index++;
}