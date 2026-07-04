/* 2048 in 396 bytes -- a complete, faithful terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int -- functions and globals omit `int`, and each
   function declares its scratch locals as extra, never-passed parameters.

   The board M[0..15] is the 4x4 grid in row-major order; M[16] is one
   spare "scratch" cell. X (=16) triples as the board size, the index-wrap
   modulus, and that scratch index.

   Everything runs through one routine, s(x): it slides and merges every
   line toward its start, in direction x. Rather than a rotation function,
   the cell index is a direction-dependent stride  i*I + G*(j^m)  where the
   two low bits of x pick the direction: bit 0 = axis (rows vs columns),
   bit 1 = reflect (forward vs reverse). Each turn calls s three times:
        s(W=1)    probe the vertical axis (and reset the flags in W)
        s(0)      probe the horizontal axis -- and draw the board on the way
        s(k%985)  the real move (only this arg exceeds 1, which is how s
                  tells a real move from a dry-run probe)
   A probe writes its slid tiles into the scratch cell instead of the
   board, so the very same code can either move or just test a move.

   W carries two status bits, rebuilt every turn. Each cell read does W|=P,
   piling the tile values into W. Tiles are even, so bit 0 is never touched
   and serves as a "stuck" flag: it starts set (W=1) and clears the instant
   a slide opens a gap. Bit 11 lights iff some tile reached 2048. The
   endgame test W&2049 selects exactly those two bits and ignores the rest. */

M[17],X=16,W,k,I,G,m;

/* s(x): slide + merge every line in direction x.
       i = current line     j = read cursor     k = write cursor
       l = the held tile (awaiting a landing spot, or an equal to merge with)
       t = the slot the next write lands in
   Tiles are powers of two, which keeps the whole merge in bare bit-ops. */
s(x,i,j,l,P,t){
    I=4-x%2*3;                                /* row coeff: 4 (horizontal) or 1 (vertical),
                                                 chosen by x's bit 0 (the axis) */
    G=5-I;                                    /* position coeff: the swap, 1 or 4 */
    m=x&2?3:0;                                /* reflect mask from x's bit 1: XOR 3 sends
                                                 pos 0<->3, 1<->2, so j^m = 3-j reverses a
                                                 line for the Right / Up directions */
    for(i=4;i--;)                             /* 4 lines; the row's \n is folded into
                                                 the 4th cell's printf below, so the
                                                 outer update clause is now empty */
        for(j=k=l=0;k<4;
            /* The loop's update clause is itself `l = <ternary>`: the held
               tile just becomes whatever the taken branch evaluates to. */
            l=j<4
            ?   W|=P=M[i*I+G*(j++^m)],        /* read cell (line i, reflected pos j);
                                                 W|=P lights bit 11 at 2048 */
                x||printf("%4.0d|%c",P,j/4*10),/* x==0: print it; %4.0d prints BLANKS for
                                                 0 (precision 0 on a zero emits no digits,
                                                 so an empty cell is four spaces). The %c
                                                 rides the row \n: after j++ the 4th cell
                                                 has j==4 so j/4*10==10=='\n'; cells 0-2
                                                 have j/4==0, emitting a NUL the terminal
                                                 ignores (written to the stream, never
                                                 shown). Costs -2 vs the old outer puts. */
                /* A tile (P!=0): if one is held, emit l+P&~P -- that is 2l on a
                   merge (l==P), else just l -- and advance k. The branch then
                   evaluates to the new hold P&~l (0 right after a merge, else P).
                   An empty cell takes the final :l, holding on across the gap. */
                P?l?M[k++,t]=l+P&~P:0,P&~l:l
            :   (M[k++,t]=l,W&=~!l,0))         /* line done: store the hold (the comma
                                                  in M[k++,t] slips the k++ in), record
                                                  the gap in W, and 0 clears the hold */
        t=x>1?i*I+G*(k^m):X;                  /* move -> real slot (write cursor k, same
                                                  reflected stride); probe -> scratch X */
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
       the M[16] sink, so the value -- hence the clear -- ALWAYS runs (a guarded
       i?...:0 would skip the clear on full-board turns).  (Cost: a full board now
       spends one rand the guarded form skipped, shifting the spawn sequence but
       not its uniform-cell / 50-50 distribution.) */
    M[i?i%X:X]=2<<rand(puts("\e[H\e[J"))%2;

    s(W=1),s(0);              /* probe both axes, redraw (screen already cleared above) */

    /* W&2049==0 -> still in play: read the 3-byte arrow escape into k and
       move with s(k%985). Otherwise the game is over -- print WIN if a 2048
       exists (bit 11) else LOSE, and unwind back to the OS. */
    W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,3)|main(s(k%985));
}
