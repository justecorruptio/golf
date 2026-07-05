/* 2048 in 382 bytes -- a complete, faithful terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int. s()'s scratch variables (i,j,l,P,t,B) are GLOBALS
   rather than params -- byte-neutral vs a long param list, and it lets one
   of them, l, persist its value between calls (see the loop note below).

   The board M[0..15] is the 4x4 grid in row-major order; M[16] is one
   spare "scratch" cell. X (=16) triples as the board size, the index-wrap
   modulus, and that scratch index.

   Everything runs through one routine, s(x): it slides and merges every
   line toward its start, in direction x. Conceptually the cell index is
   the stride  i*I + G*(c ^ m)  -- I = x%11 is the row coefficient
   (1 vertical / 4 horizontal), G = 5-I its swap, and m = x%7 (in {0,3})
   the reflect mask that flips a line for Right/Up.  But the two terms
   never share bits (one is the 4s pair, the other the 1s pair), so + is
   XOR -- and since G is 1 or 4 (a shift), G*(c^m) = G*c ^ G*m.  The whole
   line-constant part folds into one accumulator computed per line:
        B = i*I ^ G*m        (written  x%11*i ^ x%7*G)
   and a cell is just  M[B ^ G*cursor].  That XOR split is what lets the
   index be shared by the read and write sites at all -- with +, the
   reflect term is welded to the cursor and nothing can be hoisted.
   Each turn calls s three times:
        s(56)   probe the vertical axis   (a dry-run sentinel: I=1, no reflect)
        s(70)   probe the horizontal axis -- and draw the board on the way
        s(k%162)  the real move
   The three are told apart entirely by the argument: k%162 is always ODD
   (k is odd, 162 even), so x&1 marks a real move; the even sentinels 56/70
   are dry-runs, writing to the scratch cell instead of the board. The one
   even value carrying bit 6 (70) is the pass that renders (x&64).

   W carries two status bits, rebuilt every turn (the reset rides the stty
   gate -- see main). Each cell read does W|=P, piling tile values into W. Tiles are
   even, so bit 0 is never touched and serves as a "stuck" flag: it starts
   set and clears the instant a slide opens a gap. Bit 11 lights iff some
   tile reached 2048. The endgame test W&2049 selects those two bits. */

M[17],X=16,W,k,G,i,j,l,P,t,B;

/* s(x): slide + merge every line in direction x.
       i = current line     j = read cursor     k = write cursor
       l = the held tile (awaiting a landing spot, or an equal to merge with)
       B = this line's index base (row term XOR reflect term)
       t = the slot the next write lands in
   Tiles are powers of two, which keeps the whole merge in bare bit-ops. */
s(x){
    G=5-x%11;                                 /* position coeff: 4 (vertical) or 1
                                                 (horizontal); the row coeff x%11 and
                                                 reflect mask x%7 are used inline in B */
    for(i=4;i--;)                             /* 4 lines; the row's \n is folded into
                                                 the 4th cell's printf below */
        /* Inner loop resets only j,k -- NOT l. Every line ends with l==0 (the
           final flush stores the last held tile and clears l), and since l is
           a global that 0 carries into the next line, so re-zeroing it is
           redundant. The first call inherits l==0 from the zero-init globals. */
        for(j=k=0;k<4;
            l=j<4
            ?   W|=P=M[B^G*j++],              /* read cell: line base XOR G*cursor;
                                                 W|=P lights bit 11 at 2048 */
                x&64&&printf("%4.0d|%c",P,j/4*10),/* only the x=70 pass renders (bit 6 is
                                                 unique to 70). %4.0d prints BLANKS for 0;
                                                 the %c rides the row \n (j/4*10==10 at the
                                                 4th cell, a NUL the terminal ignores before). */
                /* A tile (P!=0): if one is held, emit l+P&~P -- 2l on a merge
                   (l==P) else just l -- and advance k. The branch then evaluates
                   to the new hold P&~l (0 after a merge, else P). An empty cell
                   takes the final :l, holding on across the gap. */
                P?l?M[k++,t]=l+P&~P:0,P&~l:l
            :   (M[k++,t]=l,W&=~!l,0))         /* line done: store the hold (the comma
                                                  in M[k++,t] slips the k++ in), record
                                                  the gap in W, and 0 clears the hold */
        /* Loop body, run before each read: refresh the line base B (row term
           x%11*i XOR reflect term x%7*G) and aim the write cursor. */
        B=x%11*i^x%7*G,t=x&1?B^G*k:X;         /* real move (x odd) -> real slot;
                                                  dry-run sentinel (even) -> scratch X */
}

/* main(): play one turn, then tail-recurse into the next. rand() is left
   unseeded, so every game replays identically. k is dual-role -- s()'s write
   cursor above, and here the read buffer for the 3-byte arrow escape (it
   ends each s() at 4, a clean high byte, so the read lands correctly).
   main takes no parameter: it borrows the GLOBAL i for its spawn scan --
   safe because every use of i below happens before the s() calls clobber
   it, and each call re-initializes i before reading it (so neither argc on
   the first entry nor the -1 that s() leaves behind is ever seen). The
   recursive call still passes s(k%162) as an "argument" purely to sequence
   the move before re-entry; its value lands nowhere. */
main(){
    /* Put the terminal in cbreak (per-key) mode exactly once -- the call hides
       in rand()'s ignored argument, gated by k (0 only on the first turn).
       The gate expression k||system(...) is a logical OR, so its value is
       exactly 0 or 1 -- and W= captures it as the per-turn flag reset: W=1
       on every turn after the first (k holds the last key), sitting exactly
       between the move and the probes.  The first turn gets W=0 instead,
       which is unobservable: one tile on the board can be neither stuck nor
       a win, so bit 0's start value never reaches the endgame test.
       Then spawn: from a random start, scan down for an empty cell (i reaches
       0 only if the board is completely full). */
    for(i=X+rand(W=k||system("stty cbreak"))%X;M[--i%X]*i;);

    /* Drop a 2 or a 4 -- and make this one write do two extra jobs.  The screen
       clear is the value's rand() argument: rand ignores it, but the puts() fires
       every turn.  And the scratch-place M[i?i%X:X]= sends a full board (i==0) to
       the M[16] sink, so the value -- hence the clear -- ALWAYS runs. */
    M[i?i%X:X]=2<<rand(puts("\e[H\e[J"))%2;

    /* Probe both axes with the dry-run sentinels (W was already reset above);
       the horizontal one (70) also redraws (screen already cleared above). */
    s(56),s(70);

    /* W&2049==0 -> still in play: read the 3-byte arrow escape into k and
       move with s(k%162). Otherwise the game is over -- print WIN if a 2048
       exists (bit 11) else LOSE, and unwind back to the OS. */
    W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,3)|main(s(k%162));
}
