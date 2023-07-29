#include "Tuning.h"

// 0.118151 K = 1.13
// 0.118836 K = 1.2
// 0.117478 K = 1.05
// 0.116846 K = 0.95
// 0.116521 K = 0.85
// 0.116501 K = 0.83
// 0.116498 K = 0.82
// 0.116499 K = 0.81
// 0.116505 K = 0.8
// 0.116609 K = 0.75
double K = 0.82;
std::fstream fenDraw;
std::fstream fenWhite;
std::fstream fenBlack;

struct ErrorInfo {
	int total;
	double totalError;
};

// games obtained from CCRL 40/15
// fens extracted using pgn-extract
// pgn-extract -Tr0-1 --dropply 16 -Wfen --stopafter 20000 --notags games.pgn -ofenBlack.pgn
// then, lines were shuffled using shuffle-big-file python library
// shuffle-big-file --input_file fenBlack.pgn --batch_size 1000000 --output_file fenBlack.txt
void openFens() {
	// relative path? what's that?
	fenDraw.open("C:\\Users\\dzeng\\source\\repos\\ChessMain\\ChessMain\\weights\\fenDraw.txt", std::ios::in);
	fenWhite.open("C:\\Users\\dzeng\\source\\repos\\ChessMain\\ChessMain\\weights\\fenWhite.txt", std::ios::in);
	fenBlack.open("C:\\Users\\dzeng\\source\\repos\\ChessMain\\ChessMain\\weights\\fenBlack.txt", std::ios::in);

	if (!fenDraw.is_open() || !fenWhite.is_open() || !fenBlack.is_open()) {
		std::cout << "problem";
	}
}

double sigmoid(int score) {
	return (double)1 / (1 + pow(10, -K * (double)score / 400));
}

ErrorInfo oneFileEvaluationError(std::fstream& fenFile, double result) {
	int total = 0;
	double totalError = 0;

	Position p;

	std::string fen;
	while (getline(fenFile, fen)) {
		if (fen.length() < 5) {
			continue;
		}
		total++;
		// if ((total % 1000) == 0) std::cout << total << std::endl;
		if (total == 100) break;

		p.resetBoard();
		Position::set(fen, p);
		tt.resetTable();

		Search s = Search(&p);

		SearchInfo info;
		info.p = &p;
		info.net = s.net;
		info.net->updateAll();

		int score = s.quiescence(-32000, 32000, &info) * ((p.turn() == WHITE) ? 1 : -1);
		// std::cout << score << " " << sigmoid(score) << std::endl;
		totalError += pow(result - sigmoid(score), 2);
	}

	ErrorInfo err;
	err.total = total;
	err.totalError = totalError;

	return err;
}

double evaluationError(std::vector<int> params) {
	loadParams(params);

	ErrorInfo draws = oneFileEvaluationError(fenDraw, 0.5);
	ErrorInfo white = oneFileEvaluationError(fenWhite, 1);
	ErrorInfo black = oneFileEvaluationError(fenBlack, 0);

	return (draws.totalError + white.totalError + black.totalError) / (draws.total + white.total + black.total);
}

std::vector<int> loadWeights(const char* fileName) {
	std::vector<int> weights;
	std::fstream weightFile;
	weightFile.open(fileName, std::ios::in);

	std::string line;
	while (getline(weightFile, line)) {
		int index = 0;
		int lastIndex = line.find(" ", index);

		while (lastIndex != std::string::npos) {
			weights.push_back(stoi(line.substr(index, lastIndex - index)));
			index = lastIndex + 1;
			lastIndex = line.find(" ", index);
		}

		//weights.push_back(stoi(line.substr(index, lastIndex)));
	}

	return weights;
}

std::vector<int> tune(std::vector<int> initialGuess) {
	const int nParams = initialGuess.size();
	double bestError = evaluationError(initialGuess);
	std::vector<int> bestGuess = initialGuess;

	bool improved = true;

	int iteration = -1;
	double finalError;

	while (improved) {
		iteration++;
		improved = false;

		for (int pi = 0; pi < nParams; pi++) {
			std::cout << "iteration " << iteration << " parameter " << pi << " of " << nParams << std::endl;

			std::vector<int> newGuess = bestGuess;
			newGuess[pi] += 1;
			double newError = evaluationError(newGuess);
			if (newError < bestError) {
				bestError = newError;
				bestGuess = newGuess;
				improved = true;
			}
			else {
				newGuess[pi] -= 2;
				newError = evaluationError(newGuess);
				if (newError < bestError) {
					bestError = newError;
					bestGuess = newGuess;
					improved = true;
				}
				else {
					newGuess[pi] += 1;
				}
			}
		}

		finalError = evaluationError(bestGuess);

		std::fstream saveWeights;
		saveWeights.open("C:\\Users\\dzeng\\source\\repos\\ChessMain\\ChessMain\\weights\\iteration " + std::to_string(iteration) + " error " + std::to_string(finalError) + ".txt", std::ios::out | std::ios::trunc);

		int index = 0;
		for (int parameter : bestGuess) {
			index++;
			saveWeights << parameter << " ";
			if (index % 8 == 0) saveWeights << std::endl;
		}

		saveWeights.close();
	}

	return bestGuess;
}