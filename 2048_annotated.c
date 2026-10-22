/* 2048 in 313 bytes -- a complete terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int, plus one GNU C extension: the omitted-middle
   conditional  a?:b  (a if a is nonzero, else b; a evaluated once).
   s()'s scratch variables (j,k,l,P) are
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
        s(V=4) probe the vertical axis (a dry run, see the lift below)
        s(1)   probe the horizontal axis AND draw the board on the way
        s(j/44|9452)  the real move
   -- and once more, s(9452), on every dart that misses (see main).
   The decode j/44|9452 maps the three arrow bytes to 97516, 108286,
   108527, 110076 (Up, Down, Right, Left): residues 4, 13, 16, 1 mod
   17.  Nothing else about those numbers matters except that all
   exceed 68 (lift).  The |9452 is for an UNREAD j, which is 0: 9452 is
   a multiple of 17, and a code that is 0 mod 17 makes every cursor
   product 0, so that pass reads and writes nothing but M[0], the junk
   cell (see SPAWN) -- an IDLE pass that leaves the board, the screen
   and the random stream alone.  (One byte cheaper than j/7%6538|4,
   whose idle code 4 was the vertical probe itself.)
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
   occupied cell just re-rolls (a fresh dart per throw, no key consumed,
   so exactly one tile still appears per keypress).  The main loop IS
   the dart loop: its condition throws, and a miss simply comes round
   again (see main).  The loop needs a certificate that an empty cell
   exists, or a full board would spin forever -- that certificate is V
   (below): V=0 happens only when the board is FULL.
   The gate is the && in front of the dart: V&&M[k=rand()%16+1].  On a
   live turn V is 1 and the dart is thrown; on a full board V is 0, the
   && short-circuits, no dart is thrown at all, and the turn falls
   straight through the || to the probes.  k is then whatever s left it
   -- 0 -- so the spawn's store lands in M[0], which is not a board cell
   (no cursor product is 0 mod 17 for a move, a probe or the render) and
   nothing that matters reads it: the dart only ever indexes 1..16, and
   the one pass that touches it is the idle pass (code 9452, above),
   which reads and writes M[0] alone and leaves it 0.  So M[0] is a junk
   cell the full-board turn may scribble on, which is what lets the
   spawn be the bare 2<<(random bit) instead of a V-gated one.
   The screen clear rides the SPAWN's rand() argument, so it is printed
   exactly once per turn, right before the probes and the render; a
   missed throw prints nothing at all.

   STATUS: one variable, two channels.  V's LOW BIT is the motion flag;
   V's SIGN is the win flag.
     Motion: events RAISE it -- the write site does V|=!(stored value)
        whenever a flush writes out an EMPTY hold, i.e. whenever a line
        compacts (gap swallowed or merge made).  s itself never resets
        it: the two callers that need a fresh start pay for one in main,
        each with a value they need anyway.  A move starts from 0 -- the
        key read's descriptor V-=5 leaves exactly 0 behind (see below) --
        so at dart time V answers "did that move pass see an empty cell
        or a merge?" (either one leaves an empty cell behind, so V=1
        certifies the dart lands; V=0 means the board is FULL).  The
        vertical probe starts from 4: it is called s(V=4), its own code
        doubling as the base, because the spawn may just have filled the
        last cell and the 1 left by the move would then be a lie.  The
        render pass (x=1) keeps what the vertical probe found and
        accumulates on top of it, so at the endgame test V is 4 (nothing
        can move) or 5 (something can).  The miss-path idle pass needs
        no reset either: flushing zeros into M[0] ORs a 1 into V, but a
        miss proves an empty cell, so the 1 it inherits is already the
        right answer and the OR cannot change it.  Between a
        reset and its readers V is EXACTLY 0 or 1 (4 or 5 after the
        probes), never junk, which is what pays for the dart's &&
        gate and the spawn's bare shift.
     Win: the render call delivers it MULTIPLICATIVELY as one of printf's
        extra arguments -- C evaluates and IGNORES arguments beyond the
        format string, so the slot is free: every cell the render pass
        reads lands in P, and V*=P<2e3 multiplies V by 0 exactly for a
        2048 tile (2e3 is the float constant 2000, three characters: no
        lesser tile exceeds it, and 4096 can never exist, since the game
        ends the turn the first 2048 appears).  V drops to 0 and STAYS
        at most 1 -- later flushes only OR in bit 0 -- and no reset
        intervenes between the render and the test below.
   The endgame test is folded into the key read, taken right after the
   two probes: ~read(V-=5,&j,3).  V is 5 when the game is on -- movable
   and unwon -- and V-=5 makes it 0: the descriptor is 0, stdin, the
   arrow's three bytes arrive, and the move that follows starts from a
   clean 0.  V is 4 (stuck) or 0/1 (won) otherwise, and then the
   descriptor is -1, -5 or -4, which no process has open, so read()
   fails at once with EBADF: no byte is asked for, nothing blocks, and
   it returns -1 -- the one value whose complement is 0, so the ~
   turns exactly that failure into "loop over".  At the select V is -1
   (stuck) or -5/-4 (won), so ~V -- zero exactly for -1 -- picks
   WIN/LOSE.
   V is DECLARED 1, which is the truth about an empty board: the first
   dart lands at once.  No phantom pass is needed, and nothing in the
   game needs V to be 0 before the first throw. */

M[99],V=1,k,j,l,P;

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
                x<2&&printf("%4.d|%c",P,j&3?:10,V*=P<2e3),
                                              /* only the x=1 pass renders.  %4.d: a
                                                 period with no digits is precision
                                                 ZERO (C99 7.19.6.1), so this is %4.0d
                                                 -- BLANKS for 0.  The %c rides the row
                                                 \n: j has just stepped onto a multiple
                                                 of 4 exactly at the 4th cell of a row,
                                                 so j&3 is 0 there and the elvis yields
                                                 10 -- and before it the elvis yields
                                                 j&3 itself, the bytes 3, 2, 1 (ETX, STX,
                                                 SOH), which every terminal discards
                                                 unrendered, like the NUL they replace.
                                                 The LAST
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

/* main(): ONE loop, and it is the dart loop.  rand() is left unseeded,
   so every game replays identically.  Each pass of the condition throws
   a dart, if V says a cell is free; a MISS makes the condition true at
   once (the occupied cell), so the body runs and the loop comes round
   to throw again; a HIT -- or a full board, which throws nothing --
   falls through the || into the rest of the turn -- clear, spawn, both
   probes, the key read, which is also the endgame test (see header) --
   its value,
   the 3 bytes read, keeps the loop alive, so the body runs and makes
   the move; then the next throw.  The body is s(j/44|9452): after a read,
   j holds the arrow and this is THE MOVE; after a miss, j is 0 (every
   call of s leaves it so) and this is s(9452), the idle pass -- 9452 is
   0 mod 17, so every cell it reads or writes is M[0], the junk cell:
   harmless (touches no board cell, prints nothing, draws no random
   number), and it leaves V exactly as it found it: s has no entry
   reset, and the OR can only confirm the 1 it was handed (a miss
   proves an empty cell; the zeros it flushes into M[0] raise nothing
   new).
   k is dual-role: s()'s write cursor above (reset on every entry) and
   the dart's cell below (on a full board, the 0 s left in it: the junk
   cell); j is s()'s read cursor above and the 3-byte
   read buffer for the arrow escape below. */
main(){
    /* INIT: the terminal setup, exactly once (V is already 1, see
       header).
       CONDITION, left of the ||: V && the dart.  V is exactly 0 or 1
       here.  On a live turn it is 1, k becomes rand()%16+1, a board
       cell, and an occupied one makes the condition true -- a miss,
       come round and throw again, silently (the clear is not here).
       When V=0 the board is provably FULL: the && stops before the
       dart, nothing is drawn from rand(), and the turn goes right.
       Right of the ||: first the screen clear, riding the spawn's
       rand() argument (once per turn, always before the render); then
       the spawn, a plain STORE of 2 shifted by one random bit -- 2 or 4
       (50/50) into the landing cell, empty by the dart's contract.  On
       a full board no dart was thrown, k is the 0 that s left behind,
       and the junk lands in M[0], the cell no board pass reads (only
       the idle pass churns it, and leaves it 0).  No gate, no else arm.  Then the vertical probe, called s(V=4) so
       that it starts from its own code as a clean base (see header),
       and the render pass (which keeps it and accumulates: V is now 4
       or 5, or 0/1 on a win), then the key read on descriptor
       V-=5 (see header): stuck or won makes it return -1, whose
       complement is 0, so the whole condition is 0 and the loop ends;
       otherwise V is left at 0, j is filled with the arrow, and the
       byte count's complement is nonzero, so the body runs the move
       from a clean V. */
    for(system("stty cbreak");V&&M[k=rand()%16+1]||(M[k]=2<<rand(puts("\e[H\e[J"))%2,s(V=4),s(1),~read(V-=5,&j,3));)
        s(j/44|9452);
    /* Game over: the read failed.  V is -1 (stuck: 4, less 5) or -5/-4
       (won: 0/1, less 5), so ~V -- zero exactly for -1 -- chooses the
       string. */
    puts(~V?"WIN":"LOSE");
}
