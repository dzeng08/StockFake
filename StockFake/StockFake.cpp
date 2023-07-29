// ChessMain.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "position.h"
#include "Eval.h"
#include "Search.h"
#include "UCI.h"
#include "Book.h"
#include "nnue.h"
#include "Tuning.h"

const std::string startFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0";
const std::string endgameFen = "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - 0 1";

int main()
{
    // initialize everything
    initialise_all_databases();
    zobrist::initialise_zobrist_keys();
    openBook("finalBook.bin");

    tt.resize(128);
    tt.resetTable();

    loadNN("network.nn");

    // test files? what are those? just do your testing in main

    //Position p;
    //Position::set("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2", p);

    //NNUE* net = new NNUE(&p);
    //net->updateAll();

    //std::cout << net->evalNN() << std::endl;

    //Move m1 = UCIToMove(&p, "f7f5");
    //net->updateMove(m1);
    //std::cout << net->evalNN() << std::endl;

    //Move m2 = UCIToMove(&p, "e5f6");
    //net->updateMove(m2);
    //std::cout << net->evalNN() << std::endl;

    //net->undoMove(m2);
    //std::cout << net->evalNN() << std::endl;

    //net->undoMove(m1);
    //std::cout << net->evalNN() << std::endl;
    //std::vector<int> initialGuess = loadWeights("C:\\Users\\dzeng\\source\\repos\\ChessMain\\ChessMain\\weights\\iteration 0 error 0.107470.txt");
    //openFens();
    //tune(initialGuess);

    //Position p;
    //Position::set("1k1r3q/1ppn3p/p4b2/4p3/8/P2N2P1/1PP1R1BP/2K1Q3 w - -", p);

    //Move m = UCIToMove(&p, "d3e5");

    //std::cout << SEE(&p, m);

    //nnue* net = new nnue(&p);

    //eval(&p, net);

    UCILoop();
    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
