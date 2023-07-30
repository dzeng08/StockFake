# StockFake
Stockfish but worse

A lot of features are in this engine. Possibly the one that I'm most proud of is NNUE, since it's pretty powerful. 
NNUE is kind of like a cheat code, since I hate making hand-crafted evaluation and NNUE allows me to evaluate king safety without the pain of tuning evaluation parameters.
Compared to PeSTO tables, NNUE is about 270 ELO better. PeSTO tables are specially Texel-tuned piece square tables, which are about 200 ELO better than normal
PSQT tables. So NNUE is worth, maybe, 500 ELO.

# Search Techniques
* Aspiration Windows
* Static Null Move Pruning, also known as Reverse Futility Pruning, also sometimes known as Evaluation Pruning
* Razoring
* Null Move Pruning
* Late Moves Reduction
* Principal Variation Search
* Delta Pruning (Quiescence Search)
* Check Extensions

There were some techniques and stuff that I tried but just could not get to work. (or did not have the time).
These include:
* Singular Extension Search
* Multi-Cut
* ProbCut
* Checks in Quiescence
* SEE-based Pruning in Quiescence

For move-ordering, I had:
* PV Table first
* Hash Move second (should be the same as PV Table, but hash move can get replaced, so we have both)
* Promotions
* All captures, by MVV-LVA
* Killer 1 and Killer 2
* History

Tangentially related to Search is Transposition Tables, which were pretty generic actually.
My TT was basically just Stockfish's TT, but with its parameters tuned to the depths that my engine would search at.

# Evaluation Techniques
* NNUE
* Mop Up Eval

Yup. That's it.

My NNUE architecture is similar to SFNNv1, but... the first layer is half the size, and there's no quantization. So this is like
the Netflix adaptation of NNUE.

# Move Generation
I actually had my own move generation scheme, but then I realized, "Wow! This sucks!". So I changed it, by copying someone's code!! Yipee!!

To be honest, move generation doesn't matter that much anyways, since NNUE is the biggest bottleneck. So it doesn't really make much of a difference.
In addition, the library I worked with, Surge, was a bit lacking in some features. Here's the stuff I had to add:
* 50 move rule (very important)
* Three-fold repetition (very important)
* Side to move in Zobrist hash (very very important)
* Inability to null-move (pretty important)
* Inability to reset the board. And the creator deleted the assignment operator!

And finally, it also had these annoying template functions. And you can't pass variables into template functions. So I had to make this stupid code
that checks if the side to move is WHITE, then calls the WHITE template function. And if the side to move is BLACK, it calls the BLACK template function.

# How to run
Download bin folder. Double click .exe. If you're on linux, figure it out yourself
