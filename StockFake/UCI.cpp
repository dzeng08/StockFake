#include "UCI.h"

Move UCIToMove(Position* p, std::string input) {
	int fromFile = input.at(0) - 'a';
	int fromRank = input.at(1) - '1';
	int toFile = input.at(2) - 'a';
	int toRank = input.at(3) - '1';

	int promotion = -1;

	if (input.length() == 5) {
		switch (input.at(4)) {
		case 'N':
		case 'n':
			promotion = 0;
			break;
		case 'B':
		case 'b':
			promotion = 1;
			break;
		case 'R':
		case 'r':
			promotion = 2;
			break;
		case 'Q':
		case 'q':
			promotion = 3;
			break;
		}
	}

	int fromSquare = fromRank * 8 + fromFile;
	int toSquare = toRank * 8 + toFile;

	MoveList moves(*p);

	for (Move m : moves) {
		if (m.from() == fromSquare && m.to() == toSquare) {
			if (promotion == -1) {
				return m;
			}
			else if (promotion == m.promotion()) {
				return m;
			}
		}
	}

	return Move();
}

void UCILoop() {
	using namespace std;
	string line;

	cout.setf(ios::unitbuf);

	Position position;
	Position::set(DEFAULT_FEN, position);

	Search s = Search(&position);

	Move ponderMove;

	while (getline(cin, line)) {
		if (line == "uci") {
			cout << "id name pain" << endl;
			cout << "id author dan" << endl;
			cout << "uciok" << endl;
		}
		else if (line == "quit") {
			cout << "bye" << endl;
			break;
		}
		else if (line == "isready") {
			cout << "readyok" << endl;
		}
		else if (line == "ucinewgame") {
			tt.resetTable();
		}

		if (line.substr(0, 8) == "position") {
			if (line.substr(9, 8) == "startpos") {
				position.resetBoard();
				Position::set(DEFAULT_FEN, position);
				s = Search(&position);
			}
			else if (line.substr(9, 3) == "fen") {
				position.resetBoard();
				Position::set(line.substr(13, line.find("moves" - 13)), position);
				s = Search(&position);
			}

			int index = line.find("moves");
			if (index == string::npos) continue;

			index += 6;

			int lastIndex = line.find(" ", index);

			while (lastIndex != string::npos) {
				Move m = UCIToMove(&position, line.substr(index, lastIndex - index));
				position.play(m);
				index = lastIndex + 1;
				lastIndex = line.find(" ", index);
			}

			Move m = UCIToMove(&position, line.substr(index, lastIndex));
			position.play(m);
		}

		if (line.substr(0, 2) == "go") {
			// time setting
			int wtime = -1;
			int btime = -1;
			int movestogo = 10000;

			int index = line.find("wtime");
			int lastIndex;
			if (index != string::npos) {
				index += 6;
				lastIndex = line.find(" ", index);

				wtime = stoi(line.substr(index, lastIndex - index));
			}

			index = line.find("btime");
			if (index != string::npos) {
				index += 6;
				lastIndex = line.find(" ", index);

				btime = stoi(line.substr(index, lastIndex - index));
			}

			index = line.find("movestogo");
			if (index != string::npos) {
				index += 10;
				lastIndex = line.find(" ", index);

				movestogo = stoi(line.substr(index, lastIndex - index));
			}

			int timeToUse;

			if (position.turn() == WHITE) timeToUse = wtime;
			else timeToUse = btime;

			if (timeToUse != -1) {
				int matCount = 0;
				int halfMovesLeft;

				// use formula to estimate half-moves left in match
				// http://facta.junis.ni.ac.rs/acar/acar200901/acar2009-07.pdf
				for (int square = 0; square < 64; square++) {
					matCount += materialCount[position.at((Square)square)];
				}

				if (matCount < 20) {
					halfMovesLeft = matCount + 10;
				}
				else if (matCount <= 60) {
					halfMovesLeft = 3 * matCount / 8 + 22;
				}
				else {
					halfMovesLeft = 5 * matCount / 4 - 30;
				}

				timeLimit = timeToUse / std::min(movestogo, halfMovesLeft);
			}
			else {
				timeLimit = 5000;
			}

			// max depth setting
			int depth = -1;
			index = line.find("depth");
			lastIndex;
			if (index != string::npos) {
				index += 6;
				lastIndex = line.find(" ", index);

				depth = stoi(line.substr(index, lastIndex - index));
			}

			if (depth != -1) maxDepth = depth;

			ponderMove = s.startSearch();
		}

		if (line.substr(0, 7) == "showpos") {
			cout << position;
		}
	}
}