/* 2048 in 303 bytes -- a complete terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int, plus one GNU C extension: the omitted-middle
   conditional  a?:b  (a if a is nonzero, else b; a evaluated once).
   ONE function.  The whole game is a single for loop: its BODY is one
   step of a slide pass (one cell read, or one cell written), and its
   CONDITION, which runs between steps, is the TURN TAIL -- it notices
   when a pass has finished and sets up the next one (spawn, probe,
   render, key read), or ends the game.  There is no s() any more: the
   function-call plumbing (the definition, three calls per turn, the
   return) is gone, and with it the need for the status flag V to be a
   global: V is main's parameter, i.e. argc, which is 1 -- exactly the
   value an empty board needs (see STATUS).  The scratch variables
   (j,k,l,P,x) are globals and each is relied on to persist from one
   pass to the next: l ends every pass at 0, k and j at 0, P at 1.

   THE BOARD lives in M[1..16].  There is no row/column arithmetic at
   all: a cell is  M[c*x%17]  for a cursor c in 16..1 and the pass code
   x.  Multiplying by 4 modulo 17 is a QUARTER TURN of the 4x4 (4*4 ==
   16 == -1 mod 17), so one cursor counting 16 down to 1 walks the board
   in ROWS when x is +-r and in COLUMNS when x is +-4r (mod 17), where
   r is the stride of the pass that DRAWS the screen -- the layout is
   whatever the render pass says it is, and ANY nonzero stride r is a
   valid one.  Here r is 7 (see PASSES for why), so:
        x == 11 (mod 17)   columns, first cell  6   UP
        x ==  6            columns, first cell 11   DOWN
        x == 10            rows,    first cell  7   RIGHT
        x ==  7            rows,    first cell 10   LEFT
   (The compaction target of a line is the first cell it visits.)  The
   screen is drawn by the x=7 pass, cells 10,3,13,6 on the top row, left
   to right; so screen row R, column C is cell 7*(16-4R-C) mod 17, and
   the table above reads off the picture.
   Lines are simply "every four cursor steps", so the loop needs no
   line counter, no line base, no per-line setup and no exit test beyond
   the cursor itself.

   PASSES.  Every turn runs three passes, and the CODE x of the pass
   that just ended tells the tail what comes next:
        x == 6   the vertical probe (a dry run, see the lift) is done
                 -> clear the screen; x = what puts() returned, 7
        x has bit 1 (that is x == 7, the only such code)
                 the render pass (horizontal probe + drawing) is done
                 -> read the key, x = its decode (or 0: game over)
        else     a move (or the idle pass, or nothing yet: x starts 0)
                 -> throw the dart; a HIT spawns and sets x=6,
                    a MISS sets x=4, the IDLE pass (below)
   The screen clear is  puts("\e[H\e[J"), which returns the number of
   bytes it wrote, 7; that 7 is the render code, and the layout follows
   from it (above).  The decode P/7&8125 maps the three arrow bytes to
   5672, 6840, 7949, 925 (Up, Down, Right, Left): residues 11, 6, 10, 7
   mod 17.  Three more things about those numbers matter: all exceed
   119 (lift), none is a multiple of 7 (render gate), and none has bit
   1 set -- nor do 0 and 4 -- so "the render just ended" is the two-
   character test x&2.  If the read FAILS the buffer P keeps the 1 it
   always holds between passes, and 1/7&8125 is 0: x=0 is the value of
   the whole condition, so the loop ends -- the key read is also the
   endgame test (see STATUS).  No idle value is needed from the decode,
   because a missed dart never reaches it.
   The probe code 6 is the DOWN residue (any vertical pass will do for
   a probe) and is also the status base (STATUS).  The IDLE code 4 is a
   dry pass too: it reads the board with stride 4 and writes its slides
   into M[29..31], scratch; it prints nothing, draws no random number,
   and leaves the board alone.  Its only effect is a pass's worth of
   bookkeeping (j=k=0, l=0, P=1) and ORing a 1 into V, which a miss has
   already made nonzero.

   DRY RUNS: the one write site ORs the index with 119/x.  For the
   moves (x > 119) that is 0 and the board is written; for the render
   pass (x=7) it is 17, for the vertical probe (x=6) it is 19 and for
   the idle pass (x=4) it is 29, so their slides are written into
   M[17..31] -- write-only scratch (the probes READ the board, unlifted,
   and must), and the board is untouched while the cursor paces exactly
   as on a real move.

   SPAWN: a dart.  Each turn throws one uniform-random dart V =
   rand()%k+1 -- k is the write cursor, which the tail has just reset
   to 16, so this is rand()%16+1 spelled with the 16 it already paid
   for; an empty cell catches it and receives the new tile, an
   occupied cell just re-rolls (a fresh dart per throw, no key consumed,
   so exactly one tile still appears per keypress).  The tail IS the
   dart loop: a miss sets x=4, the idle pass runs, and the tail comes
   round to throw again.  The loop needs a certificate that an empty
   cell exists, or a full board would spin forever -- that certificate
   is V (below): V=0 happens only when the board is FULL.
   The gate is the && in front of the dart: V&&M[V=rand()%k+1].  On a
   live turn V is nonzero and the dart is thrown -- INTO V, which the
   spawn then indexes; on a full board V is 0, the && short-circuits,
   no dart is thrown at all, and the spawn's store lands in M[V] = M[0],
   which is not a board cell (no cursor product is 0 mod 17) and
   nothing reads: a junk cell the full-board turn may scribble on,
   which is what lets the spawn be the bare 2<<(random bit).  Either
   way V is then set to 6, the probe's base, and that 6 is the value
   the tail hands to x.
   The screen clear is printed when the probe ends, right before the
   render, exactly once per turn; a missed throw prints nothing at all.

   STATUS: one variable, V.  Its LOW BIT is the motion flag; a win
   drives it to zero.
     Motion: events RAISE it -- the write site does V|=!(stored value)
        whenever a flush writes out an EMPTY hold, i.e. whenever a line
        compacts (gap swallowed or merge made).  A move starts from 0 --
        the key read's descriptor V-=7 leaves exactly 0 behind (see
        below) -- so at dart time V answers "did that move pass see an
        empty cell or a merge?" (either one leaves an empty cell behind,
        so V=1 certifies the dart lands; V=0 means the board is FULL).
        The vertical probe starts from the 6 the spawn tuple left (the
        spawn may just have filled the last cell, so the 1 left by the
        move would be a lie).  The render pass keeps what the vertical
        probe found and accumulates on top of it, so at the endgame test
        V is 6 (nothing can move) or 7 (something can).
     Win: the render pass delivers it MULTIPLICATIVELY as one of
        printf's extra arguments -- C evaluates and IGNORES arguments
        beyond the format string, so the slot is free: every cell the
        render pass reads lands in P, and V*=P<2e3 multiplies V by 0
        exactly for a 2048 tile (2e3 is the float constant 2000, three
        characters: no lesser tile exceeds it, and 4096 can never exist,
        since the game ends the turn the first 2048 appears).  V drops
        to 0 and STAYS at most 1 -- later flushes only OR in bit 0.
   The endgame test is folded into the key read, taken right after the
   two probes: read(V-=7,&P,3).  V is 7 when the game is on -- movable
   and unwon -- and V-=7 makes it 0: the descriptor is 0, stdin, the
   arrow's three bytes arrive in P, and the move that follows starts
   from a clean 0.  V is 6 (stuck) or 0/1 (won) otherwise, and then the
   descriptor is -1, -7 or -6, which no process has open, so read()
   fails at once with EBADF: no byte is asked for, nothing blocks, P
   stays 1, the decode is 0 and the loop is over.  At the select V is
   -1 (stuck) or -7/-6 (won), so ~V -- zero exactly for -1 -- picks
   WIN/LOSE.
   V STARTS AS argc, 1, which is the truth about an empty board: the
   first dart lands at once.  Nothing in the game needs V to be 0
   before the first throw, and x starts 0, which the tail reads as
   "throw the dart". */

M[99],k,j,l,P,x;

/* main(V): V is argc (see STATUS).  rand() is left unseeded, so every
   game replays identically.
   The slide:   j = read cursor     k = write cursor     (both 16 down to
                                                 1, global, never reset per line)
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

   A line is four read steps followed by as many flush steps as it takes
   the write cursor to catch up with the read cursor.  The phase test is
   j&3|k==j :
   - j&3 != 0 means the line's reads are not done (j counts 16..1, so
     the four reads of a line leave it at a multiple of 4 exactly when
     the fourth is done);
   - k==j means a line is about to START (both cursors at its first
     cell) -- and it can mean nothing else, because the first read of a
     line never writes (nothing is held yet), so when the reads are done
     the write cursor is strictly ahead and at least one flush follows.
   So read iff j&3 or k==j, else flush.  A pass is over when k reaches 0,
   the end of the last line; every pass thus leaves j==k==0, and the
   tail resets both to 16 when it starts the next one. */
main(V){
    /* INIT: the terminal setup, exactly once.
       CONDITION: while a pass is running (k != 0) it is simply k, and
       the body takes the next step.  When k is 0 a pass has ended (or
       none has started yet), and the tail runs: reset both cursors,
       then choose the next pass by the code of the one that ended --
       the value of the whole condition is the new x, so 0 ends the
       loop (see PASSES):
         x == 6  (vertical probe done)  ->  clear the screen; the 7
                 that puts() returns is the render pass's code.
         x&2     (render done: 7 is the only code with bit 1)  ->  read
                 the key on descriptor V-=7 (stuck or won makes it fail
                 at once, see STATUS) and decode it from P: an arrow
                 gives the move code, a failed read leaves P at 1 and
                 gives 0: game over.
         else    (a move, the idle pass, or the start)  ->  the dart.
                 V && the dart: on a live turn V is nonzero, V becomes
                 rand()%k+1, a board cell, and an occupied one is a
                 MISS: x=4, the idle pass, then come round and throw
                 again, silently.  When V=0 the board is provably FULL:
                 the && stops before the dart, nothing is drawn from
                 rand(), and the turn goes right.
                 The HIT (or full-board) tuple: the spawn, a plain STORE
                 of 2 shifted by one random bit -- 2 or 4 (50/50) into
                 M[V], the landing cell, empty by the dart's contract
                 (on a full board V is 0 and the junk lands in M[0], the
                 cell no pass reads); then V=6, the probe's base, whose
                 value is also the tuple's: x=6.
       INCREMENT: the new hold, phase-free (see above).  Reads:
       P&~(l+1) is 0 right after a merge, else P; an empty cell (P==0)
       keeps l.  Flushes: P==1 makes the whole mask 0 (bit 0 of ~(l+1)
       is 0 for even l).  Then P=1, unconditionally: a read iteration
       overwrites it, a flush iteration inherits exactly the 1 it needs
       -- so the flush condition in the body is 1 -- and the tail
       inherits it too, as the read buffer whose top byte is 0 and
       whose decode is 0 if no key arrived. */
    for(system("stty cbreak");
        k||(j=k=16,
            x=x^6
                ?   x&2
                    ?   read(V-=7,&P,3),P/7&8125
                    :   V&&M[V=rand()%k+1]
                        ?   4
                        :   (M[V]=2<<rand()%2,V=6)
                :   puts("\e[H\e[J"));
        l=P?P&~-~l:l,P=1)
        /* The body, one step of the pass, a single expression:
               V |= (should we write?) && !(the store)
           -- the && short-circuits past the store exactly when a guarding
           `if` would have skipped it.  Reads: fetch the cell, maybe print
           it, and write iff a tile was read while one is held (P*l != 0)
           -- the emit.  Flushes: always write.  The MOTION raise happens
           at the write site, keyed on the value written. */
        V|=(j&3|k==j
            ?   P=M[j--*x%17],                /* read cell (j-- comma-sequenced
                                                 before the printf args below) */
                x%7||printf("%4.d|%c",P,j&3?:10,V*=P<2e3),
                                              /* only the x=7 pass renders: 7 is the
                                                 one code that is a multiple of 7.
                                                 %4.d: a period with no digits is
                                                 precision ZERO (C99 7.19.6.1), so this
                                                 is %4.0d -- BLANKS for 0.  The %c rides
                                                 the row \n: j has just stepped onto a
                                                 multiple of 4 exactly at the 4th cell
                                                 of a row, so j&3 is 0 there and the
                                                 elvis yields 10 -- and before it the
                                                 elvis yields j&3 itself, the bytes 3, 2,
                                                 1 (ETX, STX, SOH), which every terminal
                                                 discards unrendered, like the NUL they
                                                 replace.  The LAST argument is the WIN
                                                 DELIVERY (see header): printf evaluates
                                                 it and ignores it -- a free expression
                                                 slot, once per rendered cell. */
                P*l
            :   1
            /* The one write site, shared by moves, probes and the idle
               pass.  Real moves write the board cell k*x%17; the others
               write the same cell lifted into scratch by 119/x (see
               header), so k paces identically on dry passes and nothing
               on the board is touched.  The value written is the MOTION
               witness: at a flush it is the hold l, so a stored 0 means
               this line compacted (gap or merge) -- and emit or merge
               writes are never 0 -- hence the !(the store). */
            )&&!(M[k--*x%17|119/x]=l+P&~P);
    /* Game over: the read failed.  V is -1 (stuck: 6, less 7) or -7/-6
       (won: 0/1, less 7), so ~V -- zero exactly for -1 -- chooses the
       string. */
    puts(~V?"WIN":"LOSE");
}
