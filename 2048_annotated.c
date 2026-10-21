/* 2048 in 324 bytes -- a complete terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int.  s()'s scratch variables (j,k,l,P) are
   GLOBALS rather than params, and every one of them is relied on to
   persist between calls: l ends every call at 0, k and j at 0, P at 1
   (see s and main for who reads what).

   THE BOARD lives in M[1..16].  There is no row/column arithmetic at
   all: a cell is  M[c*x%17]  for a cursor c in 16..1 and the move code
   x.  Multiplying by 4 modulo 17 is a QUARTER TURN of the 4x4 (4*4 ==
   16 == -1 mod 17), so one cursor counting 16 down to 1 walks the board
   in ROWS when x is +-1 and in COLUMNS when x is +-4 (mod 17), and the
   sign reverses every line -- which is the slide direction:
        x ==  4 (mod 17)   columns, first cell 13   UP
        x == 13            columns, first cell  4   DOWN
        x == 16            rows,    first cell  1   RIGHT
        x ==  1            rows,    first cell 16   LEFT
   (The compaction target of a line is the first cell it visits.)  The
   screen is drawn by the x=1 pass, cells 16,15,14,13 on the top row,
   left to right; so screen row R, column C is cell 16-4R-C, and the
   table above reads off the picture.  Compared with the previous card
   the picture is the same board column-mirrored -- the game is the
   same game, only the random spawn lands in the mirrored cell.
   Lines are simply "every four cursor steps", so the function needs no
   line counter, no line base, no per-line setup and no exit test beyond
   the cursor itself: everything the old card spent on i, B, G and their
   bookkeeping is gone.

   Every turn calls s three times:
        s(4)   probe the vertical axis (a dry run, see the lift below)
        s(1)   probe the horizontal axis AND draw the board on the way
        s(j/7%6538)  the real move
   The decode j/7%6538 maps the three arrow bytes to 3846, 132, 2957,
   5781 (Up, Down, Right, Left): residues 4, 13, 16, 1 mod 17.  Nothing
   else about those numbers matters except that all exceed 68 (lift).
   The two probes are ONE DIGIT each because the residues 4 and 1 are
   themselves legal codes, and their smallness is exactly what the dry
   run keys on.

   DRY RUNS: the one write site ORs the index with 68/x.  For the moves
   (x > 68) that is 0 and the board is written; for the render pass
   (x=1) it is 68 and for the vertical probe (x=4) it is 17, so their
   slides are written into M[17..84] -- write-only scratch (the probes
   READ the board, unlifted, and must), and the board is untouched while
   the cursor paces exactly as on a real move.

   SPAWN: a dart.  Each turn throws one uniform-random dart k =
   rand()%16+1; an empty cell catches it and receives the new tile, an
   occupied cell just re-rolls -- a bare for-loop that spins until it
   lands (a fresh dart per spin, no key consumed, so exactly one tile
   still appears per keypress).  The loop needs a certificate that an
   empty cell exists, or a full board would spin forever -- that
   certificate is V (below): V=0 happens only when the board is FULL,
   and then the multiply gate kills the condition on the first roll and
   the spawn, which is ADDITIVE and scaled by V, adds 0 to an occupied
   cell -- nothing is thrown away, nothing changes.

   STATUS: one variable, two channels.  V's LOW BIT is the motion flag;
   V's SIGN is the win flag.
     Motion: events RAISE it -- the write site does V|=!(stored value)
        whenever a flush writes out an EMPTY hold, i.e. whenever a line
        compacts (gap swallowed or merge made).  It is reset by s itself
        on entry, V/=x: every real move code is large, so a move starts
        from 0 -- at dart time V answers "did that move pass see an
        empty cell or a merge?" (either one leaves an empty cell
        behind, so V=1 certifies the dart lands; V=0 means the board is
        FULL) -- and the vertical probe (x=4) starts from 0 too, while
        the render pass (x=1) keeps what the vertical probe found and
        accumulates on top of it; at the endgame test V answers "can
        anything still move?".  Between a reset and its readers V is
        EXACTLY 0 or 1, never junk, which is what pays for the dart's
        two-char multiply gate -- and for the key read's byte count,
        see below.
     Win: the render call delivers it SUBTRACTIVELY as one of printf's
        extra arguments -- C evaluates and IGNORES arguments beyond the
        format string, so the slot is free: every cell the render pass
        reads lands in P, and P&2048 is 2048 exactly for a 2048 tile (no
        lesser tile has bit 11, and 4096 can never exist: the game ends
        the turn the first 2048 appears).  V-=2048 drives V negative and
        it STAYS negative -- later flushes only OR in bit 0 -- and no
        reset intervenes between the render and the test below.
   The endgame test is V>=P.  P is 1 after every call of s (its loop
   increment leaves it so) and 0 at declaration, so the test is exactly
   V==1 (movable and unwon: any win is negative, stuck is 0) on every
   turn but the first, where it is trivially true.  At the select V is
   0 or negative, so a bare V picks WIN/LOSE.  V itself gates the key
   read: read(0,&j,V*3) -- V is exactly 1 on every live turn (the loop
   test just passed on V==1) and 0 at declaration, so turn one reads
   NOTHING (POSIX: a zero-byte read returns 0 at once) and every later
   turn reads the three arrow bytes. */

M[99],V,k,j,l,P;

/* s(x): slide + merge every line in direction x.
       j = read cursor     k = write cursor     (both 16 down to 1,
                                                 global, never reset per line)
       l = the held tile (awaiting a landing spot, or an equal to merge with)
   Tiles are powers of two, which keeps the whole merge in bare bit-ops --
   and makes ONE write expression serve both emit and flush:  l+P&~P  is
   2l on a merge (l==P), l on a block, and degenerates to exactly l when
   P is 0 or 1 (l is always even).  The loop increment sets P=1 after
   every iteration (a read overwrites it, a flush inherits it) -- and that
   1, the only odd P in the program, then doubles as the PHASE FLAG: the
   hold update  P&~-~l  is P&~l untouched for even tiles but exactly 0
   at a flush, so the loop needs no second phase test.
   (~-~l is ~(l+1); l is always even, so l+1 == l|1 and this one unary
   ripple equals the two-mask form ~l&~1 everywhere it runs.)

   ONE loop, one line of code, all sixteen cells.  A line is four read
   steps followed by as many flush steps as it takes the write cursor to
   catch up with the read cursor.  The phase test is  j&3|k==j :
   - j&3 != 0 means the line's reads are not done (j counts 16..1, so
     the four reads of a line leave it at a multiple of 4 exactly when
     the fourth is done);
   - k==j means a line is about to START (both cursors at its first
     cell) -- and it can mean nothing else, because the first read of a
     line never writes (nothing is held yet), so when the reads are done
     the write cursor is strictly ahead and at least one flush follows.
   So read iff j&3 or k==j, else flush.  The call exits when k reaches 0,
   which is the end of the last line.  Every call thus leaves j==k==0,
   and resets both to 16 on entry. */
s(x){
    V/=x;                                     /* the motion reset (see header): 0 for
                                                 a move or the vertical probe, kept
                                                 for the render pass (x=1) */
    for(j=k=16;k;
        /* Increment slot: the new hold, phase-free (see header).
           Reads: P&~(l+1) is 0 right after a merge, else P; an empty
           cell (P==0) keeps l.  Flushes: P==1 makes the whole mask 0
           (bit 0 of ~(l+1) is 0 for even l).  Then P=1, unconditionally:
           a read iteration overwrites it, a flush iteration inherits
           exactly the 1 it needs -- so the flush condition below is 1. */
        l=P?P&~-~l:l,P=1)
        /* The body, a single expression:
               V |= (should we write?) && !(the store)
           -- the && short-circuits past the store exactly when a guarding
           `if` would have skipped it.  Reads: fetch the cell, maybe print
           it, and write iff a tile was read while one is held (P*l != 0)
           -- the emit.  Flushes: always write.  The MOTION raise happens
           at the write site, keyed on the value written. */
        V|=(j&3|k==j
            ?   P=M[j--*x%17],                /* read cell (j-- comma-sequenced
                                                 before the printf args below) */
                x<2&&printf("%4.d|%c",P,j&3?0:10,V-=P&2048),
                                              /* only the x=1 pass renders.  %4.d: a
                                                 period with no digits is precision
                                                 ZERO (C99 7.19.6.1), so this is %4.0d
                                                 -- BLANKS for 0.  The %c rides the row
                                                 \n: j has just stepped onto a multiple
                                                 of 4 exactly at the 4th cell of a row,
                                                 so it is \n there and a NUL (which the
                                                 terminal ignores) before.  The LAST
                                                 argument is the WIN DELIVERY (see
                                                 header): printf evaluates it and
                                                 ignores it -- a free expression slot,
                                                 once per rendered cell. */
                P*l
            :   1
            /* The one write site, shared by moves and probes.  Real moves
               write the board cell k*x%17; probes write the same cell lifted
               into scratch by 68/x (see header), so k paces identically on
               dry passes and nothing on the board is touched.  The value
               written is the MOTION witness: at a flush it is the hold l, so
               a stored 0 means this line compacted (gap or merge) -- and emit
               or merge writes are never 0 -- hence the !(the store). */
            )&&!(M[k--*x%17|68/x]=l+P&~P);
}

/* main(): the turn loop.  rand() is left unseeded, so every game replays
   identically.  The loop is a bare for: its CONDITION is the endgame
   test, its BODY reads the key, makes the move and throws the dart, and
   its INCREMENT spawns and probes -- test, read, move, dart, spawn,
   probes, test: the turn cycle exactly, with the test right after the
   probes.  k is dual-role: s()'s write cursor above (reset on every
   entry) and the dart's cell below; j is s()'s read cursor above and
   the 3-byte read buffer for the arrow escape below (it is 0 after every
   call, so the top byte of the key is clean). */
main(){
    /* The outer for.  CONDITION V>=P (see header).  INCREMENT: the spawn
       and the two probes.  The spawn, ADDITIVE and scaled by V: V+V is 2
       on a live turn and 0 on a full board, shifted by one random bit --
       so an empty landing cell receives 2 or 4 (50/50) and an occupied
       one receives +0.  No gate, no else arm.  Then the vertical probe
       (which zeroes V on entry, see s) and the render pass (which keeps
       it and accumulates). */
    for(;V>=P;M[k]+=V+V<<rand()%2,s(4),s(1))
        /* The body: a second for whose INIT slot runs once per turn and
           already owns its semicolon.  read() fills j with the arrow and
           returns the byte count, which selects: 3 -> THE MOVE,
           s(j/7%6538), whose entry zeroes V; 0 (turn one, V==0) ->
           system("stty cbreak")+4: the terminal setup
           runs exactly once, and its 0 exit status plus 4 is the vertical
           probe's own code.  That dry run over the empty board is the
           PHANTOM MOVE: it writes nothing (lifted), prints nothing, and
           its sixteen empty-cell flushes raise V -- exactly the
           certificate the first dart needs.
           Then the dart, as a bare retry loop: spin while the board is live
           AND the cell is occupied.  V is exactly 0 or 1 here, so the gate
           is a bare multiply: on live boards the condition is M[k] itself
           and the loop spins until an empty cell catches the dart; when
           V=0 the board is provably FULL, the product dies on the first
           roll, and the spawn adds 0 to whatever occupied cell k points
           at.  The multiply, unlike &&, keeps M[k] evaluated, so the clear
           is emitted (and k assigned) on every roll.  The screen clear
           rides this rand's argument -- retries re-clear, invisibly. */
        for(s(read(0,&j,V*3)?j/7%6538:system("stty cbreak")+4);M[k=rand(puts("\e[H\e[J"))%16+1]*V;);
    /* Game over: the loop exited on V<P.  V is 0 (stuck) or negative (won),
       so a bare V chooses the string. */
    puts(V?"WIN":"LOSE");
}
