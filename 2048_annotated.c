/* 2048 in 392 bytes -- a complete, faithful terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int -- functions and globals omit `int`, and each
   function declares its scratch locals as extra, never-passed parameters.

   The board M[0..15] is the 4x4 grid in row-major order; M[16] is one
   spare "scratch" cell. X (=16) triples as the board size, the index-wrap
   modulus, and that scratch index.

   Everything runs through one routine, s(x): it slides and merges every
   line toward its start, in direction x. The cell index is a direction-
   dependent stride  i*I + G*(j ^ x%7)  where the argument x is decoded by
   two mods: I = x%11 is the row coefficient (1 vertical / 4 horizontal),
   and x%7 (in {0,3}) is the reflect mask that flips a line for Right/Up.
   Each turn calls s three times:
        s(56)   probe the vertical axis   (a dry-run sentinel: I=1, no reflect)
        s(70)   probe the horizontal axis -- and draw the board on the way
        s(k%162)  the real move
   The three are told apart entirely by the argument: k%162 is always ODD
   (k is odd, 162 even), so x&1 marks a real move; the even sentinels 56/70
   are dry-runs, writing to the scratch cell instead of the board. The one
   even value carrying bit 6 (70) is the pass that renders (x&64).

   W carries two status bits, rebuilt every turn (reset by the standalone
   W=1). Each cell read does W|=P, piling the tile values into W. Tiles are
   even, so bit 0 is never touched and serves as a "stuck" flag: it starts
   set and clears the instant a slide opens a gap. Bit 11 lights iff some
   tile reached 2048. The endgame test W&2049 selects those two bits. */

M[17],X=16,W,k,I,G;

/* s(x): slide + merge every line in direction x.
       i = current line     j = read cursor     k = write cursor
       l = the held tile (awaiting a landing spot, or an equal to merge with)
       t = the slot the next write lands in
   Tiles are powers of two, which keeps the whole merge in bare bit-ops. */
s(x,i,j,l,P,t){
    I=x%11;                                   /* row coeff: 1 (vertical) or 4 (horizontal);
                                                 x%7 (used inline below) is the reflect mask */
    G=5-I;                                    /* position coeff: the swap, 4 or 1 */
    for(i=4;i--;)                             /* 4 lines; the row's \n is folded into
                                                 the 4th cell's printf below */
        for(j=k=l=0;k<4;
            /* The loop's update clause is itself `l = <ternary>`: the held
               tile just becomes whatever the taken branch evaluates to. */
            l=j<4
            ?   W|=P=M[i*I+G*(j++^x%7)],      /* read cell (line i, reflected pos j^x%7);
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
        t=x&1?i*I+G*(k^x%7):X;                /* real move (x odd) -> real slot;
                                                  dry-run sentinel (even) -> scratch X */
}

/* main(): play one turn, then tail-recurse into the next. rand() is left
   unseeded, so every game replays identically. */
main(i){
    /* Put the terminal in cbreak (per-key) mode exactly once -- the call hides
       in rand()'s ignored argument, gated by k (0 only on the first turn).
       Then spawn: from a random start, scan down for an empty cell (i reaches
       0 only if the board is completely full). */
    for(i=X+rand(k||system("stty cbreak"))%X;M[--i%X]*i;);

    /* Drop a 2 or a 4 -- and make this one write do two extra jobs.  The screen
       clear is the value's rand() argument: rand ignores it, but the puts() fires
       every turn.  And the scratch-place M[i?i%X:X]= sends a full board (i==0) to
       the M[16] sink, so the value -- hence the clear -- ALWAYS runs. */
    M[i?i%X:X]=2<<rand(puts("\e[H\e[J"))%2;

    /* Reset the flags, then probe both axes with the dry-run sentinels; the
       horizontal one (70) also redraws (screen already cleared above). */
    W=1,s(56),s(70);

    /* W&2049==0 -> still in play: read the 3-byte arrow escape into k and
       move with s(k%162). Otherwise the game is over -- print WIN if a 2048
       exists (bit 11) else LOSE, and unwind back to the OS. */
    W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,3)|main(s(k%162));
}
