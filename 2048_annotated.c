/*
    Bare-bones golfed 2048, in ~500 bytes of C.

    Plays in the terminal with the arrow keys:
        cc 2048.c -o 2048 && ./2048
    Reach a 2048 tile to win; fill the board with no move left to lose.

    Written in implicit-int K&R style: every function and global drops
    its `int`, and functions list their locals as extra parameters.

    Globals
    -------
    M[16] : the 4x4 board, row-major (M[0] top-left ... M[15] bottom-right).
            Each cell is 0 (empty) or a power of two.
    X     : the constant 16, pulling double duty --
              - as a modulus  (i%X) to wrap an index into the 16-cell board
              - as a shift    (k>>X) to pull the 3rd byte out of an arrow code
    W     : status bit-field, recomputed every turn:
              bit 0 (W&1) : a 2048 tile exists       -> the player has WON
              bit 1 (W&2) : an empty slot remains after sliding -> a move is
                            still possible.  So W==2 means "still in play".
    k     : scratch. Doubles as the write-cursor inside s(), and as the
            buffer that read() drops the 3-byte arrow-key escape into.

    Function locals
    ---------------
    s(f,d,...): f = 0 dry-run (only update W), 1 = actually move the tiles.
                d = direction 0..3.  i = line, j = read cursor,
                l = the tile currently held/pending, P = the cell just read.
    w(d,i,j)  : rotates a coordinate so s() can always slide one way.
    T(i)      : plays one whole turn then recurses; i is scratch.
*/

/* ---- globals ---- */
M[16],X=16,W,k;

/*
    main(): put the terminal in cbreak mode so read() hands us each key
    immediately (no waiting for Enter) while leaving output cooked, then run
    the game. system()'s return value is fed in as T's argument purely to
    save a second statement -- T overwrites it at once. When T() finally
    returns the game is over, so report the result from the win bit.
*/
main(){
    T(system("stty cbreak"));
    puts(W&1?"WIN":"LOSE");
}

/*
    Arrow-key -> direction table, indexed by (the escape's final byte & 3):
        Left=68&3=0, Up=65&3=1, Down=66&3=2, Right=67&3=3
    Logically the table is {2,3,1,0}; the trailing 0 is omitted and supplied
    for free by the zeroed storage just past the array -- so the Right arrow's
    K[3] is an intentional out-of-bounds read that lands on a 0 (saves 2 bytes).
*/
K[]={2,3,1};

/*
    s(f,d): slide + merge every line of the board in direction d.
    With f==0 it only updates the W status flags (a dry run, used to test
    whether the game is still playable); with f==1 it writes the moved
    tiles back into M.  Each line is compacted toward its start: a read
    cursor j walks the cells, a write cursor k emits the result.
*/
s(f,d,i,j,l,P){
    for(i=4;i--;)                 /* for each of the four lines */
        for(j=k=l=0;k<4;)         /* k = write cursor; reset read cursor & held tile */
            j<4
            ?   /* --- still cells left to read on this line --- */
                P=M[w(d,i,j++)],          /* read next cell (through the rotation), advance j */
                W|=P>>11,                 /* P>=2048 ? set the WIN bit (2048>>11 == 1) */
                l*P&&(                    /* both the held tile and this cell are non-empty: */
                    f?M[w(d,i,k)]=l<<(l==P):0, /* emit the held tile, doubled if it == P (a merge) */
                    k++                   /* advance the write cursor */
                ),
                /* update the held tile:
                     held && cell && held!=P -> can't merge: now hold P
                     held && cell && held==P -> just merged: clear the hold
                     held && empty cell      -> skip it, keep holding
                     not holding             -> start holding this cell      */
                l=l?P?l-P?P:0:l:P
            :   /* --- reads exhausted: flush the hold, then zero-fill the rest --- */
                (f?M[w(d,i,k)]=l:0,       /* write whatever is still held (0 once flushed) */
                ++k,                      /* advance the write cursor */
                W|=2*!l,                  /* this slot ended empty ? set the MOVE-POSSIBLE bit */
                l=0);
}

/*
    w(d,i,j): coordinate rotation. d==0 is the identity, linear index 4*i+j.
    Each step rotates the square 90 degrees by remapping (i,j) -> (j,3-i),
    so the four values of d let s()'s single left-ward slide cover all four
    on-screen directions with one routine.
*/
w(d,i,j){
    return d ? w(d-1,j,3-i) : 4*i+j;
}

/*
    T(i): one turn of the game -- spawn, evaluate, render, read, recurse.
    (rand() is never seeded, so every run deals the same tile sequence.)
*/
T(i){
    /* spawn: from a random start in [16,31] scan downward for an empty
       cell (M[i%16]==0), stopping at i==0 if the board is full */
    for(i=X+rand()%X;M[i%X]*i;i--);
    i?M[i%X]=2<<rand()%2:0;       /* found one -> drop a 2 or a 4 into it */

    /* recompute the flags: clear W, then dry-run a slide in all four
       directions so W reflects win / move-possible for the new board */
    for(W=i=0;i<4;)s(0,i++);

    /* render: clear screen + home the cursor, then print cells 15..0, four
       to a row, each 4 columns wide; empty cells print as blanks */
    for(i=X,puts("\e[2J\e[H");i--;i%4||puts(""))
        printf(M[i]?"%4d|":"    |",M[i]);

    /* W==2 means "still playable" (a move exists and not yet won). If so,
       read a 3-byte arrow escape into k, turn its final byte into a
       direction through K[], perform the real move with s(1,...), and
       recurse into the next turn. Any other W (won, or no move left)
       short-circuits, so T() returns and the game ends.
       (Relies on read() filling k before T's argument is evaluated --
       true under gcc's left-to-right evaluation here.) */
    W-2||read(0,&k,3)|T(s(1,K[(k>>X)%4]));
}
//[2048]
