/*
    Bare-bones golfed 2048, in ~450 bytes of C.

    Plays in the terminal with the arrow keys:
        cc 2048.c -o 2048 && ./2048
    Reach a 2048 tile to win; fill the board with no move left to lose.

    Written in implicit-int K&R style: every function and global drops
    its `int`, and functions list their locals as extra parameters.

    Globals
    -------
    M[17] : the 4x4 board in M[0..15] (row-major, M[0] top-left ... M[15]
            bottom-right; each cell 0=empty or a power of two), plus M[16] --
            a scratch sink that dry-run slides write to instead of the board.
    X     : the constant 16 -- the board size. Used as a modulus (i%X) to wrap
            an index into the 16-cell board, as the spawn/render bound, and as
            the scratch-cell index M[X] (=M[16]) for dry-run writes.
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
    s(x,...)  : the slide routine. x packs the direction (d = x%7) and the
                perform-flag (f = x>X, i.e. only the huge key value k); the
                calls are s(0..15) (dry runs, in the render loop) and s(k)
                (the real move).  i = line, j = read cursor,
                l = the tile currently held/pending, P = the cell just read,
                t = where this pass writes M[t]: w(x%7,i,k) for a real move,
                    or X (the scratch cell) for a dry run -- so a dry run
                    never disturbs the board.
    w(d,i,j)  : rotates a coordinate so s() can always slide one way.
    main(i)   : the whole game -- one call per turn, recursing; i is scratch.
*/

/* ---- globals ---- */
M[17],X=16,W,k;

/*
    s(x): slide + merge every line of the board. The single argument x packs
    both the direction (d = x%7) and whether to actually move. A dry run
    (any small x, here 0..15) just updates the W status flags to test whether
    the game is still playable; the real move (x = the key value k, which is
    huge) actually shifts the tiles. Rather than guard every write, the trick
    is the write *target*: t = (x>X ? real slot : X), so a dry run aims every
    write at the scratch cell M[X] and leaves the board untouched -- while
    still computing W. Each line is compacted toward its start: a read cursor
    j walks the cells, a write cursor k emits the result.
*/
s(x,i,j,l,P,t){
    for(i=4;i--;)                 /* for each of the four lines */
        /* for(A;B;)C,D == for(A;B;D)C : the slide (the j<4? expression) goes
           in the loop's UPDATE clause and the body becomes just C, the write-
           slot CSE t=w(x%7,i,k). Each pass still runs the body (compute t)
           then the update (slide) -- but with no comma needed between them. */
        for(j=k=l=0;k<4;          /* reset read cursor & held tile; k = write cursor */
            j<4
            ?   /* --- still cells left to read on this line --- */
                P=M[w(x%7,i,j++)],        /* read next cell (through the rotation), advance j */
                W|=P>>11,                 /* P>=2048 ? set the WIN bit (2048>>11 == 1) */
                l*P?                    /* both the held tile and this cell are non-empty: */
                    M[t]=l<<(l==P),       /* emit held tile to M[t], doubled if ==P (a merge) */
                    k++                   /* advance the write cursor */
                :0,
                /* update the held tile:
                     cell nonzero, l empty or l!=P -> hold this cell P
                     cell nonzero and l==P         -> just merged: clear the hold
                     cell empty (P==0)             -> keep the current hold     */
                l=P?l-P?P:0:l
            :   /* --- reads exhausted: flush the hold, then zero-fill the rest --- */
                (M[t]=l,                 /* write whatever is still held to M[t] (0 once flushed) */
                ++k,                      /* advance the write cursor */
                W|=2*!l,                  /* this slot ended empty ? set the MOVE-POSSIBLE bit */
                l=0))
        /* the loop body picks the write target t: the real rotated slot
           w(x%7,i,k) for a move (x>X), or the scratch index X for a dry run */
        t=x>X?w(x%7,i,k):X;
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
    /* The one-time terminal setup is folded into the spawn's rand() call as
       a throwaway argument: rand() is K&R-declared (no prototype), so the arg
       k||system("stty cbreak") is evaluated -- running the setup -- before
       rand() even fires, then discarded. cbreak mode makes read() hand us
       each key at once (no Enter), output left cooked. k is 0 only on the
       OS's first call (globals zero-init); every recursion enters with k==4
       (s() leaves its write cursor there), so k||... runs stty exactly once.

       The spawn itself: from a random start in [16,31] scan downward for an
       empty cell (M[i%16]==0), stopping at i==0 if the board is full */
    for(i=X+rand(k||system("stty cbreak"))%X;M[i%X]*i;i--);
    i?M[i%X]=2<<rand()%2:0;       /* found one -> drop a 2 or a 4 into it */

    /* Recompute the W flags AND render the board in ONE loop. W=0 clears the
       flags; then for each cell i=15..0 the body prints M[i] while the update
       runs s(i) as a dry run (i <= 15 < X, so x>X is false and every write is
       aimed at the scratch cell M[X], never the board). Sweeping i over 0..15
       drives s through all four slide directions (x%7) -- a superset of the
       horizontal+vertical needed for win / move-possible -- so W ends up
       correct, and since the dry runs never disturb M the printed cells are
       the true post-spawn board.
       (puts homes the cursor and \e[J clears the screen; four cells per row,
       each 4 wide, empties printed blank.) */
    W=0;for(i=X,puts("\e[H\e[J");i--;s(i),i%4||puts(""))
        printf(M[i]?"%4d|":"    |",M[i]);

    /* W==2 means "still playable" (a move exists and not yet won). If so,
       read the 3-byte arrow escape into k, which holds the whole sequence
       ESC '[' letter = 27 + 91*256 + letter*65536 (the high byte stays 0),
       and hand k straight to s(). Inside, s uses k%7 as the direction: the
       four arrows land on {Up:3,Down:1,Right:0,Left:2} (and k>1 is true, so
       s performs the move). The arrow->direction map is an awkward
       permutation, which is why it needs a modulo rather than a plain shift
       or mask. Because k%7 reads the whole escape it assumes the ESC '['
       prefix -- a terminal in SS3 / application cursor-key mode (ESC O ...)
       would be misread, but a normal terminal never sends that. main then
       recurses for the next turn.
       Any other W (a win, or no move left) takes the other branch: print
       WIN or LOSE and return -- which unwinds the whole recursion back to
       the OS and ends the game. (read() must fill k before main's argument
       is evaluated, which holds under gcc's left-to-right evaluation of
       `|`.) */
    W-2?puts(W&1?"WIN":"LOSE"):read(0,&k,3)|main(s(k));
}
