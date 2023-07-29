#pragma warning( disable: 4996 )

#include "NNUE.h"

float W0[40960 * NN_SIZE];
float B0[NN_SIZE];
float W1[NN_SIZE * 2 * 32];
float B1[32];
float W2[32 * 32];
float B2[32];
float W3[32 * 1];
float B3[1];

#define LOAD_WEIGHTS(arr) fread(arr##, sizeof(arr##), 1, file)

void loadNN(const char* fileName) {
	FILE* file = fopen(fileName, "rb");

	if (file == NULL) {
		std::cout << "ERROR: Could not open " << fileName << std::endl;
		exit(-1);
	}

	// we love macro abuse
	LOAD_WEIGHTS(W0);
	LOAD_WEIGHTS(W1);
	LOAD_WEIGHTS(W2);
	LOAD_WEIGHTS(W3);
	LOAD_WEIGHTS(B0);
	LOAD_WEIGHTS(B1);
	LOAD_WEIGHTS(B2);
	LOAD_WEIGHTS(B3);
	fclose(file);
}

inline float ReLU(float value) {
	if (value < 0.0f) {
		return 0.0f;
	}
	else if (value > 1.0f) {
		return 1.0f;
	}
	return value;
}

// Some helper macroes for CPU intrinsics
#define LOAD_REGISTER(r) __m256 reg##r = _mm256_mul_ps(_mm256_loadu_ps(&I[r * 8]), _mm256_loadu_ps(&W[offset + r * 8]))
#define ADD_REGISTER(r) reg##r = _mm256_fmadd_ps(_mm256_loadu_ps(&I[i + r * 8]), _mm256_loadu_ps(&W[offset + i + r * 8]), reg##r)
#define COMBINE_REGISTER(r1, r2) const __m256 reg##r1##r2 = _mm256_add_ps(reg##r1, reg##r2)

// Computes a layer using 8 256-bit accumulators. Each accumulator holds 8 floats, so 64 floats are processed in each loop
void computeLayer64(float* B, float* I, float* W, float* O, int idim, int odim, bool useReLU) {
	for (int o = 0; o < odim; o++) {
		// CPU intrinsics. I don't really understand them very well
		// see: https://stackoverflow.com/questions/59494745/avx2-computing-dot-product-of-512-float-arrays

		const int offset = o * idim;

		// Load 8 256-bit registers
		LOAD_REGISTER(0);
		LOAD_REGISTER(1);
		LOAD_REGISTER(2);
		LOAD_REGISTER(3);
		LOAD_REGISTER(4);
		LOAD_REGISTER(5);
		LOAD_REGISTER(6);
		LOAD_REGISTER(7);

		// Loop through the weights. Each loop processes 64 floats
		for (int i = 64; i < idim; i += 64) {
			ADD_REGISTER(0);
			ADD_REGISTER(1);
			ADD_REGISTER(2);
			ADD_REGISTER(3);
			ADD_REGISTER(4);
			ADD_REGISTER(5);
			ADD_REGISTER(6);
			ADD_REGISTER(7);
		}

		// Combine the results from 8 registers into one
		COMBINE_REGISTER(0, 1);
		COMBINE_REGISTER(2, 3);
		COMBINE_REGISTER(01, 23);
		COMBINE_REGISTER(4, 5);
		COMBINE_REGISTER(6, 7);
		COMBINE_REGISTER(45, 67);
		COMBINE_REGISTER(0123, 4567);

		// Add the 8 floats in the 256-bit register together
		const __m128 r4 = _mm_add_ps(_mm256_castps256_ps128(reg01234567), _mm256_extractf128_ps(reg01234567, 1));
		const __m128 r2 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));
		const __m128 r1 = _mm_add_ss(r2, _mm_movehdup_ps(r2));

		// We only use computeLayer64 for layer 1, which always uses ReLU!
		// The argument is still in the function so that it looks good
		// But it's not used
		// _mm_cvtss_f32 extracts a 32-bit floating point from the 256-bit register
		O[o] = ReLU(B[o] + _mm_cvtss_f32(r1));
	}
}

// Computes a layer using 4 256-bit accumulators. Each accumulator holds 4 floats, so 32 floats are processed in each loop
void computeLayer32(float* B, float* I, float* W, float* O, int idim, int odim, bool useReLU) {
	for (int o = 0; o < odim; o++) {
		// CPU intrinsics. I don't really understand them very well
		// see: https://stackoverflow.com/questions/59494745/avx2-computing-dot-product-of-512-float-arrays

		const int offset = o * idim;

		// Load 4 256-bit registers
		LOAD_REGISTER(0);
		LOAD_REGISTER(1);
		LOAD_REGISTER(2);
		LOAD_REGISTER(3);

		// Loop through the weights. Each loop processes 32 floats
		for (int i = 32; i < idim; i += 32) {
			ADD_REGISTER(0);
			ADD_REGISTER(1);
			ADD_REGISTER(2);
			ADD_REGISTER(3);
		}

		// Combine the results from 4 registers into one
		COMBINE_REGISTER(0, 1);
		COMBINE_REGISTER(2, 3);
		COMBINE_REGISTER(01, 23);

		// Add the 8 floats in the 256-bit register together
		const __m128 r4 = _mm_add_ps(_mm256_castps256_ps128(reg0123), _mm256_extractf128_ps(reg0123, 1));
		const __m128 r2 = _mm_add_ps(r4, _mm_movehl_ps(r4, r4));
		const __m128 r1 = _mm_add_ss(r2, _mm_movehdup_ps(r2));

		// Layer 2 uses ReLU, but Layer 3 doesn't
		// We do need to check useReLU variable
		// _mm_cvtss_f32 extracts a 32-bit floating point from the 256-bit register
		if (useReLU) {
			O[o] = ReLU(B[o] + _mm_cvtss_f32(r1));
		}
		else {
			O[o] = B[o] + _mm_cvtss_f32(r1);
		}
	}
}

int NNUE::evalNN() {
	float O0[NN_SIZE * 2], O1[32], O2[32], O3[1];
	int color = position->turn();

	// Layer 0 is the output from the accumulator - we only have to clamp it
	for (int o = 0; o < NN_SIZE; o++) {
		O0[o] = ReLU(accumulator[color][o]);
		O0[o + NN_SIZE] = ReLU(accumulator[color ^ 1][o]);
	}

	// Layer 1 is large enough for us to compute 64 floats per loop
	// ... BUT ACTUALLY, computeLayer32 is FASTER than computeLayer64
	// so computeLayer64 is basically redundant and useless
	computeLayer32(B1, O0, W1, O1, NN_SIZE * 2, 32, true);

	// Layer 2 and 3 only have 32 floats so we have to use computeLayer32
	computeLayer32(B2, O1, W2, O2, 32, 32, true);
	computeLayer32(B3, O2, W3, O3, 32, 1, false);

	// Scale to centipawns
	return (int)(O3[0] * 100);
}


// Class functions
void NNUE::updateAll() {
	int whiteKing = getLS1BIndex(position->bitboard_of(WHITE_KING));
	int blackKing = getLS1BIndex(position->bitboard_of(BLACK_KING)) ^ 63;

	// reset the accumulator, since we're updating all
	memcpy(accumulator[0], B0, sizeof(B0));
	memcpy(accumulator[1], B0, sizeof(B0));

	// loop through all squares, and set the accumulator
	for (int square = 0; square < 64; square++) {
		int piece = position->at((Square)square);

		if (piece == NO_PIECE || piece == WHITE_KING || piece == BLACK_KING) continue;

		int color = (piece & 0b1000) ? 0 : 1;

		piece &= ~0b1000;

		const int featureW = NN_SIZE * (640 * whiteKing + 64 * ((piece << 1) + color) + square);
		const int featureB = NN_SIZE * (640 * blackKing + 64 * ((piece << 1) + color ^ 1) + square ^ 63);

		for (int i = 0; i < NN_SIZE; i++) {
			accumulator[WHITE][i] += W0[featureW + i];
			accumulator[BLACK][i] += W0[featureB + i];
		}
	}
}

void NNUE::updateMove(Move m) {
	if (m == Move()) {
		position->play(m);
		return;
	}

	// if we're moving a king, we have to update everything
	if ((position->at(m.from()) & ~0b1000) == WHITE_KING) {
		position->play(m);
		updateAll();
		return;
	}

	int whiteKing = getLS1BIndex(position->bitboard_of(WHITE_KING));
	int blackKing = getLS1BIndex(position->bitboard_of(BLACK_KING)) ^ 63;

	// since castling resets the accumulator, we need to handle:
	// promotions
	// captures
	// en passant
	// and quiet moves will be handled by default

	// quiet move
	const int from = m.from();
	const int to = m.to();
	int piece = position->at(m.from());

	int color = (piece & 0b1000) ? 0 : 1;

	piece &= ~0b1000;

	const int quietWTo = NN_SIZE * (640 * whiteKing + 64 * ((piece << 1) + color) + to);
	const int quietBTo = NN_SIZE * (640 * blackKing + 64 * ((piece << 1) + color ^ 1) + to ^ 63);
	const int quietWFrom = NN_SIZE * (640 * whiteKing + 64 * ((piece << 1) + color) + from);
	const int quietBFrom = NN_SIZE * (640 * blackKing + 64 * ((piece << 1) + color ^ 1) + from ^ 63);

	for (int i = 0; i < NN_SIZE; i++) {
		// add the feature for the to square
		accumulator[WHITE][i] += W0[quietWTo + i];
		accumulator[BLACK][i] += W0[quietBTo + i];

		// remove the feature for the from square
		accumulator[WHITE][i] -= W0[quietWFrom + i];
		accumulator[BLACK][i] -= W0[quietBFrom + i];
	}

	// captures
	if (m.is_capture() && !m.is_enPassant()) {
		const int captured = position->at(m.to()) & ~0b1000;
		const int capturedW = NN_SIZE * (640 * whiteKing + 64 * ((captured << 1) + color ^ 1) + to);
		const int capturedB = NN_SIZE * (640 * blackKing + 64 * ((captured << 1) + color) + to ^ 63);

		for (int i = 0; i < NN_SIZE; i++) {
			// remove the feature for the captured piece (flip the color)
			accumulator[WHITE][i] -= W0[capturedW + i];
			accumulator[BLACK][i] -= W0[capturedB + i];
		}
	}

	// promotions
	if (m.is_promotion()) {
		const int promotionPiece = m.promotion() + 1;
		const int promotionWNew = NN_SIZE * (640 * whiteKing + 64 * ((promotionPiece << 1) + color) + to);
		const int promotionBNew = NN_SIZE * (640 * blackKing + 64 * ((promotionPiece << 1) + color ^ 1) + to ^ 63);
		const int promotionWOld = NN_SIZE * (640 * whiteKing + 64 * (color)+to);
		const int promotionBOld = NN_SIZE * (640 * blackKing + 64 * (color ^ 1) + to ^ 63);

		for (int i = 0; i < NN_SIZE; i++) {
			// add the feature for the new piece
			accumulator[WHITE][i] += W0[promotionWNew + i];
			accumulator[BLACK][i] += W0[promotionBNew + i];

			// remove the feature for the promoted pawn
			accumulator[WHITE][i] -= W0[promotionWOld + i];
			accumulator[BLACK][i] -= W0[promotionBOld + i];
		}
	}

	// en passant
	if (m.is_enPassant()) {
		const int epSquare = to + ((color == 1) ? (-8) : (8));
		const int epW = NN_SIZE * (640 * whiteKing + 64 * (color ^ 1) + epSquare);
		const int epB = NN_SIZE * (640 * blackKing + 64 * (color)+epSquare ^ 63);

		for (int i = 0; i < NN_SIZE; i++) {
			// remove the feature for the en passant pawn
			// flip the color, piece is 0 (pawn)
			accumulator[WHITE][i] -= W0[epW + i];
			accumulator[BLACK][i] -= W0[epB + i];
		}
	}

	position->play(m);
}

void NNUE::undoMove(Move m) {
	if (m == Move()) {
		position->undo(m);
		return;
	}

	// if we're moving a king, we have to update everything
	if ((position->at(m.to()) & ~0b1000) == WHITE_KING) {
		position->undo(m);
		updateAll();
		return;
	}

	int whiteKing = getLS1BIndex(position->bitboard_of(WHITE_KING));
	int blackKing = getLS1BIndex(position->bitboard_of(BLACK_KING)) ^ 63;

	// since castling resets the accumulator, we need to handle:
	// promotions
	// captures
	// en passant
	// and quiet moves will be handled by default

	// quiet move
	const int from = m.from();
	const int to = m.to();
	int piece = position->at(m.to());

	int color = (piece & 0b1000) ? 0 : 1;

	piece &= ~0b1000;

	const int quietWTo = NN_SIZE * (640 * whiteKing + 64 * ((piece << 1) + color) + to);
	const int quietBTo = NN_SIZE * (640 * blackKing + 64 * ((piece << 1) + color ^ 1) + to ^ 63);
	const int quietWFrom = NN_SIZE * (640 * whiteKing + 64 * ((piece << 1) + color) + from);
	const int quietBFrom = NN_SIZE * (640 * blackKing + 64 * ((piece << 1) + color ^ 1) + from ^ 63);
	for (int i = 0; i < NN_SIZE; i++) {
		// remove the feature for the to square
		accumulator[WHITE][i] -= W0[quietWTo + i];
		accumulator[BLACK][i] -= W0[quietBTo + i];

		// add the feature for the from square
		accumulator[WHITE][i] += W0[quietWFrom + i];
		accumulator[BLACK][i] += W0[quietBFrom + i];
	}

	// captures
	if (m.is_capture() && !m.is_enPassant()) {
		const int captured = position->history[position->ply()].captured & ~0b1000;
		const int capturedW = NN_SIZE * (640 * whiteKing + 64 * ((captured << 1) + color ^ 1) + to);
		const int capturedB = NN_SIZE * (640 * blackKing + 64 * ((captured << 1) + color) + to ^ 63);

		for (int i = 0; i < NN_SIZE; i++) {
			// add the feature for the captured piece (flip the color)
			accumulator[WHITE][i] += W0[capturedW + i];
			accumulator[BLACK][i] += W0[capturedB + i];
		}
	}

	// promotions
	if (m.is_promotion()) {
		const int promotionPiece = m.promotion() + 1;
		const int promotionWNew = NN_SIZE * (640 * whiteKing + 64 * ((piece << 1) + color) + from);
		const int promotionBNew = NN_SIZE * (640 * blackKing + 64 * ((piece << 1) + color ^ 1) + from ^ 63);
		const int promotionWOld = NN_SIZE * (640 * whiteKing + 64 * (color)+from);
		const int promotionBOld = NN_SIZE * (640 * blackKing + 64 * (color ^ 1) + from ^ 63);

		for (int i = 0; i < NN_SIZE; i++) {
			// remove the feature for the new piece, which is now at the from square
			accumulator[WHITE][i] -= W0[promotionWNew + i];
			accumulator[BLACK][i] -= W0[promotionBNew + i];

			// add the feature for the pawn (piece is PAWN, which is 0)
			accumulator[WHITE][i] += W0[promotionWOld + i];
			accumulator[BLACK][i] += W0[promotionBOld + i];
		}
	}

	// en passant
	if (m.is_enPassant()) {
		const int epSquare = to + ((color == 1) ? (-8) : (8));
		const int epW = NN_SIZE * (640 * whiteKing + 64 * (color ^ 1) + epSquare);
		const int epB = NN_SIZE * (640 * blackKing + 64 * (color)+epSquare ^ 63);

		for (int i = 0; i < NN_SIZE; i++) {
			// add the feature for the en passant pawn
			// flip the color, piece is 0 (pawn)
			accumulator[WHITE][i] += W0[epW + i];
			accumulator[BLACK][i] += W0[epB + i];
		}
	}

	position->undo(m);
}