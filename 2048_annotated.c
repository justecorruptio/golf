/*
    Bare-bones golfed 2048, in ~460 bytes of C.

    Plays in the terminal with the arrow keys:
        cc 2048.c -o 2048 && ./2048
    Reach a 2048 tile to win; fill the board with no move left to lose.

    Written in implicit-int K&R style: every function and global drops
    its `int`, and functions list their locals as extra parameters.

    Globals
    -------
    M[16] : the 4x4 board, row-major (M[0] top-left ... M[15] bottom-right).
            Each cell is 0 (empty) or a power of two.
    X     : the constant 16 -- the board size. Used as a modulus (i%X) to
            wrap an index into the 16-cell board, and as the spawn/render
            bound.
    W     : status bit-field, recomputed every turn:
              bit 0 (W&1) : a 2048 tile exists       -> the player has WON
              bit 1 (W&2) : an empty slot remains after sliding -> a move is
                            still possible.  So W==2 means "still in play".
    k     : scratch with three jobs: the write-cursor inside s(); the buffer
            read() drops the 3-byte arrow-key escape into (its whole value
            feeds k%7 to choose a direction); and -- being 0 only on the very
            first call -- the "stty already done" flag for k||system(...).

    Function locals
    ---------------
    s(f,d,...): f = 0 dry-run (only update W), 1 = actually move the tiles.
                d = direction -- only d mod 4 matters (w() rotates d times).
                i = line, j = read cursor,
                l = the tile currently held/pending, P = the cell just read,
                t = the write slot M[t] for the current write cursor k.
    w(d,i,j)  : rotates a coordinate so s() can always slide one way.
    main(i)   : the whole game -- one call per turn, recursing; i is scratch.
*/

/* ---- globals ---- */
M[16],X=16,W,k;

/*
    s(f,d): slide + merge every line of the board in direction d.
    With f==0 it only updates the W status flags (a dry run, used to test
    whether the game is still playable); with f==1 it writes the moved
    tiles back into M.  Each line is compacted toward its start: a read
    cursor j walks the cells, a write cursor k emits the result.
*/
s(f,d,i,j,l,P,t){
    for(i=4;i--;)                 /* for each of the four lines */
        for(j=k=l=0;k<4;)         /* k = write cursor; reset read cursor & held tile */
            /* both branches below write to the same slot, M[w(d,i,k)], so
               compute it once here as t (a comma-prefix before the j<4 test) */
            t=w(d,i,k),
            j<4
            ?   /* --- still cells left to read on this line --- */
                P=M[w(d,i,j++)],          /* read next cell (through the rotation), advance j */
                W|=P>>11,                 /* P>=2048 ? set the WIN bit (2048>>11 == 1) */
                l*P?                    /* both the held tile and this cell are non-empty: */
                    f?M[t]=l<<(l==P):0,   /* emit the held tile, doubled if it == P (a merge) */
                    k++                   /* advance the write cursor */
                :0,
                /* update the held tile:
                     cell nonzero, l empty or l!=P -> hold this cell P
                     cell nonzero and l==P         -> just merged: clear the hold
                     cell empty (P==0)             -> keep the current hold     */
                l=P?l-P?P:0:l
            :   /* --- reads exhausted: flush the hold, then zero-fill the rest --- */
                (f?M[t]=l:0,              /* write whatever is still held (0 once flushed) */
                ++k,                      /* advance the write cursor */
                W|=2*!l,                  /* this slot ended empty ? set the MOVE-POSSIBLE bit */
                l=0);
}

/*
    w(d,i,j): coordinate rotation. d==0 is the identity, linear index 4*i+j.
    Each step rotates the square 90 degrees by remapping (i,j) -> (j,3-i).
    It recurses d times, so only d mod 4 matters -- any larger d just spins
    extra full turns. The four residues let s()'s single left-ward slide
    cover all four on-screen directions with one routine.
*/
w(d,i,j){
    return d ? w(d-1,j,3-i) : 4*i+j;
}

/*
    main(i): the whole game. The OS calls it once; after that it tail-
    recurses, one call per turn -- spawn, evaluate, render, read a key,
    move, recurse. (rand() is never seeded, so every run deals the same
    tile sequence; i is scratch, overwritten immediately by the spawn loop.)
*/
main(i){
    /* one-time setup: cbreak mode so read() hands us each key at once (no
       Enter), output left cooked. k is 0 only on the OS's first call
       (globals zero-init); every recursion enters with k==4 (s() leaves its
       write cursor there), so k||... runs stty exactly once. */
    k||system("stty cbreak");

    /* spawn: from a random start in [16,31] scan downward for an empty
       cell (M[i%16]==0), stopping at i==0 if the board is full */
    for(i=X+rand()%X;M[i%X]*i;i--);
    i?M[i%X]=2<<rand()%2:0;       /* found one -> drop a 2 or a 4 into it */

    /* recompute the flags: clear W, then dry-run a slide. Only TWO directions
       are needed -- one horizontal (0) and one vertical (1): any mergeable
       pair or gap that a right/down slide would expose, left/up exposes too.
       W=0 rides in on s()'s f argument to reset the flags before s re-fills. */
    s(W=0,0),s(0,1);

    /* render: home the cursor, then \e[J erases from there to the end of
       the screen, clearing it. Then print cells 15..0, four to a row,
       each 4 columns wide; empty cells print as blanks. */
    for(i=X,puts("\e[H\e[J");i--;i%4||puts(""))
        printf(M[i]?"%4d|":"    |",M[i]);

    /* W==2 means "still playable" (a move exists and not yet won). If so,
       read the 3-byte arrow escape into k, which holds the whole sequence
       ESC '[' letter = 27 + 91*256 + letter*65536 (the high byte stays 0).
       k%7 turns that into a slide direction: the four arrows land on
       {Up:3,Down:1,Right:0,Left:2}, a value w() reduces mod 4 by rotating.
       The arrow->direction map is an awkward permutation, which is why it
       needs a modulo rather than a plain shift or mask. Because k%7 reads
       the whole escape it assumes the ESC '[' prefix -- a terminal in SS3 /
       application cursor-key mode (ESC O ...) would be misread, but a normal
       terminal never sends that. s(1,...) performs the real move, then main
       recurses for the next turn.
       Any other W (a win, or no move left) takes the other branch: print
       WIN or LOSE and return -- which unwinds the whole recursion back to
       the OS and ends the game. (read() must fill k before main's argument
       is evaluated, which holds under gcc's left-to-right evaluation of
       `|`.) */
    W-2?puts(W&1?"WIN":"LOSE"):read(0,&k,3)|main(s(1,k%7));
}
