/*
    Golfed 2048 for the terminal (~425 bytes).  Build & play:
        cc 2048.c -o 2048 && ./2048
    Arrow keys slide; reach 2048 to win, fill the board with no move to lose.

    K&R implicit-int style: functions/globals omit `int`, and each function
    declares its locals as extra (never-passed) parameters.

    Globals
        M[17] : 4x4 board in M[0..15] (row-major), plus M[16] -- a scratch
                cell that dry-run slides write to instead of the board.
        X     : 16 -- board size; also the wrap modulus (i%X) and the
                scratch index M[X].
        W     : status bits, rebuilt each turn -- W&1: a 2048 tile exists
                (won); W&2: an empty slot remains after a slide (a move is
                still possible).  W==2 means "still in play".
        k     : three roles -- s()'s write cursor (left at 4 after each
                call), the buffer read() drops the 3-byte arrow escape into,
                and (0 only on the very first call) the "stty done" flag.

    Three tricks worth naming:
      * Scratch target -- every write lands on M[t], with t = the real slot
        for a move but t = X (scratch) for a dry run, so one routine both
        slides for real and tests a slide without disturbing the board.
      * The renderer rides the x==0 dry run: it prints each cell as it reads
        it, so there is no separate draw loop.  That read order mirrors the
        board left<->right, which the move's key map (k%985) undoes.
      * w() rotates by recursing d times, and 4 rotations = identity, so it
        self-reduces mod 4.  The move can therefore pass x in raw (no %4/%7)
        as long as x is bounded -- hence k%985.
*/

M[17],X=16,W,k;

/*
    s(x): slide + merge every line.  x carries the direction (w uses x mod 4)
    and, via x>1, whether to move for real (only the huge key value is >1;
    the dry-run args 0 and 1 are not).  Each line compacts toward its start:
    read cursor j scans the cells, write cursor k emits results, held tile l
    waits to be placed or merged with the next equal tile.  The x==0 pass
    also draws (per-cell print + per-row newline); the screen clear is done
    once in main, just before this call.
*/
s(x,i,j,l,P,t){
    for(i=4;i--;x||puts(""))      /* each of 4 lines (x==0: end with a newline) */
        /* for(A;B;)C,D == for(A;B;D)C: the slide rides the update clause,
           leaving the body as just the write-slot CSE t=w(x,i,k) */
        for(j=k=l=0;k<4;          /* read cursor j, write cursor k, held tile l */
            j<4
            ?   P=M[w(x,i,j++)],          /* read next cell, advance j */
                x||printf(P?"%4d|":"    |",P),    /* x==0: draw it (blank if empty) */
                W|=P>>11,                 /* P>=2048 -> set WIN bit (2048>>11==1) */
                /* cell non-empty (P!=0; the P? wrapper tests it once for both steps):
                   - if a tile is held (l), emit it -- l+(l&P) is 2l when l==P (merge) else l (block) -- and bump k
                   - then set the held tile to P^(l&P): 0 if we just merged (l==P), else P (hold/carry)
                   P==0 falls to the :0 and leaves l untouched (carry the hold across a gap). */
                P?l?M[t]=l+(l&P),k++:0,l=P^l&P:0
            :   (M[t]=l,++k,              /* reads done: flush held tile, bump k */
                W|=2*!l,l=0))             /* this slot ended empty -> MOVE bit; reset l */
        t=x>1?w(x,i,k):X;                 /* write target: real rotated slot (move), or scratch X (dry run) */
}

/*
    w(d,i,j): linear index 4*i+j, rotated d quarter-turns -- each turn maps
    (i,j)->(j,3-i).  Recurses d times, so only d mod 4 matters: one left-ward
    slide covers all four directions, and the move can feed x in raw (k%985
    is bounded -> ~984 self-calls at most, a few KB of stack, harmless).
*/
w(d,i,j){
    return d ? w(d-1,j,3-i) : 4*i+j;
}

/* main(i): one turn per call, tail-recursing. rand() is unseeded, so every
   run is identical. i is scratch (the spawn loop overwrites it). */
main(i){
    /* stty setup hides in rand()'s ignored arg, gated by k (0 only on the
       first call) so it runs once.  Then spawn: from a random start in
       [16,31] scan down for an empty cell (i hits 0 if the board is full). */
    for(i=X+rand(k||system("stty cbreak"))%X;M[i%X]*i;i--);
    i?M[i%X]=2<<rand()%2:0;       /* drop a 2 or a 4 into it */

    /* clear, then rebuild W + redraw via two dry runs: s(W=0) resets the
       flags and does horizontal (also the renderer), s(1) does vertical. */
    puts("\e[H\e[J"),s(W=0),s(1);

    /* W==2 = playable: read the 3-byte arrow escape into k, move with
       s(k%985).  Else print WIN (W&1) / LOSE and return, unwinding to the OS.
       (Reads the whole ESC '[' seq, so assumes CSI arrows, not SS3.) */
    W-2?puts(W&1?"WIN":"LOSE"):read(0,&k,3)|main(s(k%985));
}
