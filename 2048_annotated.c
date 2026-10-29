/* 2048 in 286 bytes -- a complete terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int.  No GNU extension is needed any more: the
   omitted-middle conditional  a?:b  of the earlier cards is gone.
   ONE function.  The whole game is a single for loop: its BODY is one
   step of a slide pass (one cell read, or one cell written), and its
   CONDITION, which runs between steps, is the TURN TAIL -- it notices
   when a pass has finished and sets up the next one (spawn, probe,
   render, key read), or ends the game -- and its truth value, 1 for as
   long as the game runs, is stored into P on the way (see main).  There is no s() any more: the
   function-call plumbing (the definition, three calls per turn, the
   return) is gone, and with it the need for the status flag V to be a
   global: V is main's parameter, i.e. argc, which is 1 -- exactly the
   value an empty board needs (see STATUS).  The scratch variables
   (j,k,l,P,x) are globals and each is relied on to persist from one
   pass to the next: l ends every pass at 0, k and j at 0, P at 0 (or
   at 2048, but only once a win is already banked -- see STATUS).

   THE BOARD lives in M[1..16].  There is no row/column arithmetic at
   all: a cell is  M[c*x%17]  for a cursor c in 16..1 and the pass code
   x.  Multiplying by 4 modulo 17 is a QUARTER TURN of the 4x4 (4*4 ==
   16 == -1 mod 17), so one cursor counting 16 down to 1 walks the board
   in ROWS when x is +-r and in COLUMNS when x is +-4r (mod 17), where
   r is the stride of the pass that DRAWS the screen -- the layout is
   whatever the render pass says it is, and ANY nonzero stride r is a
   valid one.  Here r is 1 (see PASSES for why), so:
        x ==  4 (mod 17)   columns, first cell 13   UP
        x == 13            columns, first cell  4   DOWN
        x == 16            rows,    first cell  1   RIGHT
        x ==  1            rows,    first cell 16   LEFT
   (The compaction target of a line is the first cell it visits.)  The
   screen is drawn by the x=1 pass, cells 16,15,14,13 on the top row,
   left to right; so screen row R, column C is simply cell 16-4R-C, and
   the table above reads off the picture.
   Lines are simply "every four cursor steps", so the loop needs no
   line counter, no line base, no per-line setup and no exit test beyond
   the cursor itself.

   PASSES.  Every turn runs three passes -- the vertical probe (x=4),
   the render (x=1), and after the key the move -- and the tail chooses
   the next one by the SIGN OF V (the status, see STATUS), which the
   probe's base makes negative and the key read makes zero again:
        V < 0    a probe or the render just ended -> READ.  One read
                 serves both: after the probe its descriptor is negative
                 and it fails at once, leaving the buffer -- which is x,
                 the pass code itself -- at 4, and the decode of 4 is 1,
                 the render code.  After the render the descriptor is 0
                 (stdin) if the game is on, and the arrow's three bytes
                 land in x and decode to the move code; if the game is
                 over the descriptor is negative again, x stays 1, and
                 the decode of 1 is 0: the loop ends.
        else     a move (or the idle pass, or nothing yet: x starts 0)
                 -> throw the dart; a HIT spawns and sets x=4,
                    a MISS sets x=85, the IDLE pass (below)
   The decode is x%52946/4: 4 -> 1, 1 -> 0, and the arrow bytes ESC [ A,
   B, C, D (0x415b1b .. 0x445b1b as little-endian ints, x's top byte
   being 0 both times) -> 11870, 1781, 4929, 8076 (Up, Down, Right,
   Left): residues 4, 13, 16, 1 mod 17, exactly the table above, and all
   exceed 68, the lift.  The render stride 1 and probe stride 4 are what
   make the one read possible: the decode must send the probe code to
   the render code and the render code to 0, and 4/4 = 1, 1/4 = 0 is as
   cheap as that gets (x%52946 leaves 4 and 1 alone and only shapes the
   big key values).
   The IDLE code 85 is a dry pass of a special kind: 85 is 0 mod 17, so
   every cursor product is 0 and the pass READS ONLY M[0], the junk
   cell, sixteen times, and -- 68/85 being 0 -- writes its slides into
   M[0] as well; it draws no random number and leaves the board alone
   (M[0] holds 0, 2 or 4 when it starts and 0 when it ends: the four
   reads of its first line see v, v, 2v, 2v -- the two merges land in
   M[0] itself -- and the flushes that follow write 0).  Its only effect
   is a pass's worth of bookkeeping (j=k=0, l=0, P=0) and ORing a 1
   into V, which a miss has already made positive.

   DRY RUNS: the one write site ORs the index with 68/x.  For the moves
   (x > 68) that is 0 and the board is written; for the render pass
   (x=1) it is 68 and for the vertical probe (x=4) it is 17, so their
   slides are written into M[68..84] and M[17..31] -- write-only scratch
   (the probes READ the board, unlifted, and must), and the board is
   untouched while the cursor paces exactly as on a real move.  The
   idle pass's writes go to M[0] (above).

   THE SCREEN CLEAR is the render printf's OTHER format: every pass
   calls printf once per cell read, and the format is chosen by the
   pass code -- "%4.d|%c" for the render pass (x=1), the clear
   "\e[H\e[J\n" (home, erase to end of screen, newline: the board
   starts on row 2) for every other pass.  So the move pass, the idle
   pass and the vertical probe each print the clear sixteen times, and
   the render pass then draws on a blank screen; a repeated clear is
   idempotent, so the screen is exactly what one clear before the
   render gives, and the extra bytes cost nothing on the display.  The
   arguments are evaluated whatever the format (C ignores surplus
   arguments), which is why the idle pass must not read the board.

   SPAWN: a dart.  Each turn throws one uniform-random dart V =
   rand()%k+1 -- k is the write cursor, which the tail has just reset
   to 16, so this is rand()%16+1 spelled with the 16 it already paid
   for; an empty cell catches it and receives the new tile, an
   occupied cell just re-rolls (a fresh dart per throw, no key consumed,
   so exactly one tile still appears per keypress).  The tail IS the
   dart loop: a miss sets x=85, the idle pass runs, and the tail comes
   round to throw again.  The loop needs a certificate that an empty
   cell exists, or a full board would spin forever -- that certificate
   is V (below): V=0 happens only when the board is FULL.
   The gate is the && in front of the dart: V&&M[V=rand()%k+1].  On a
   live turn V is nonzero and the dart is thrown -- INTO V, which the
   spawn then indexes; on a full board V is 0, the && short-circuits,
   no dart is thrown at all, and the spawn's store lands in M[V] = M[0],
   which is not a board cell (no cursor product is 0 mod 17 in a board
   pass) and nothing but the idle pass reads: a junk cell the full-board
   turn may scribble on, which is what lets the spawn be the bare
   2<<(random bit).  Either way V is then set to -4, the probe's base --
   and the whole tuple is NEGATED: -(..., V=-4) is 4, the probe code,
   so the value the tail hands to x costs no character of its own.

   STATUS: one variable, V.  Its LOW BIT is the motion flag; its SIGN
   is the phase of the turn; a win drives it far below zero.
     Motion: events RAISE it -- the write site does V|=!(stored value)
        whenever a flush writes out an EMPTY hold, i.e. whenever a line
        compacts (gap swallowed or merge made).  A move starts from 0 --
        the key read leaves exactly 0 behind (see Phase) -- so at dart
        time V answers "did that move pass see an empty cell or a
        merge?" (either one leaves an empty cell behind, so V=1
        certifies the dart lands; V=0 means the board is FULL).
        The vertical probe starts from the -4 the spawn tuple left (the
        spawn may just have filled the last cell, so the 1 left by the
        move would be a lie) and ends at -4 (no column can move) or -3
        (one can).
     Phase: the key read is read(V/=2,&x,3), and it runs twice a turn.
        After the probe V/=2 makes -2 or -1: a negative descriptor, so
        the read fails at once with EBADF (no byte is asked for, nothing
        blocks, x is untouched) and the tail turns the untouched x=4
        into the render code 1.  The render pass then raises the low bit
        or not: -2 stays -2 if nothing can move at all and becomes -1 if
        a row can; -1 stays -1 (a column could already move).  The
        second V/=2 makes 0 of -1 -- descriptor 0, stdin, the arrow's
        three bytes arrive in x, and the move that follows starts from a
        clean 0 -- and -1 of -2: EBADF again, x stays 1, the decode is 0
        and the loop is over with V == -1.  Truncating division is what
        makes this work: -1/2 is 0 but -2/2 is -1.
     Win: every pass delivers it SUBTRACTIVELY in the loop increment:
        every cell a pass reads lands in P, and V-=P&=2048 takes 2048 off
        V exactly for a 2048 tile (tiles are powers of two, so P&2048 is
        2048 or 0; 4096 can never exist, since the game ends the turn
        the first 2048 appears).  The COMPOUND assignment is the point:
        P&=2048 also leaves P itself as that bit -- 0 after every
        ordinary step -- which is exactly the P a flush step needs (see
        the merge notes below), so the loop condition need not hoist a 1:
        one character here replaces the two of a  P=  there.  At a flush
        step the term is 0.  (After a 2048 has been read P stays 2048
        for one step, so the flush that may follow takes a second 2048
        off V and holds a junk 2048 -- the win is already banked, V is
        thousands below zero, and only dry, lifted passes remain, so
        nothing of it shows.)  Both the probe and the render read the
        winning tile, so V is thousands below zero at both key reads:
        both fail, the decode of the untouched 1 ends the loop, and V is
        nowhere near -1.  The same expression runs in the move pass,
        which never sees a 2048 (the game ends before the next move),
        and in the idle pass, which reads only M[0] (see PASSES):
        harmless in both.
   At the select V is -1 (stuck) or far below (won), so ~V -- zero
   exactly for -1 -- picks WIN/LOSE.
   V STARTS AS argc, 1, which is the truth about an empty board: the
   first dart lands at once.  Nothing in the game needs V to be 0
   before the first throw, and x starts 0; V not being negative is what
   the tail reads as "throw the dart". */

M[99],k,j,l,P,x;

/* main(V): V is argc (see STATUS).  rand() is left unseeded, so every
   game replays identically.
   The slide:   j = read cursor     k = write cursor     (both 16 down to
                                                 1, global, never reset per line)
                l = the held tile (awaiting a landing spot, or an equal to merge with)
   Tiles are powers of two, which keeps the whole merge in bare bit-ops --
   and makes ONE write expression serve both emit and flush:  l+P&~P  is
   2l on a merge (l==P), l on a block, and degenerates to exactly l when
   P is 0 (l is always even).  Every iteration starts with P=0: the
   increment's  P&=2048  (see STATUS, Win) clears P after every ordinary
   step (a read overwrites it, a flush inherits it) -- and that 0 then
   doubles as the PHASE FLAG: the hold update is an XOR (the body is
   l^=...) whose delta is  l|P  on the write arm and  P  on the other,
   and with P==0 at a flush the write arm's delta is l itself, which
   empties the hold -- so the loop needs no second phase test (and the
   win term is 0 there).  Earlier cards hoisted a 1 into P from the
   loop condition and had to mask it back out of the hold with ~(l+1);
   with 0 in that role the mask, and then the whole assignment, folded
   into the XOR.

   A line is four read steps followed by as many flush steps as it takes
   the write cursor to catch up with the read cursor.  The phase test is
   k>j>j%4, read as (k>j)>(j%4): it is 1 -- FLUSH -- exactly when k>j
   and j%4 is 0, else 0 -- READ:
   - j%4 == 0 means the line's reads are done (j counts 16..1 and is
     decremented AFTER each read, so it stands on a multiple of 4
     exactly when the fourth cell of a line has been read);
   - k>j means the write cursor has not caught up -- and k==j can mean
     nothing but "a line is about to START" (both cursors at its first
     cell), because the first read of a line never writes (nothing is
     held yet), so when the reads are done the write cursor is strictly
     ahead and at least one flush follows.
   So flush iff k>j and j%4==0, else read.  A pass is over when k
   reaches 0, the end of the last line; every pass thus leaves j==k==0,
   and the tail resets both to 16 when it starts the next one. */
main(V){
    /* INIT: the terminal setup, exactly once.
       CONDITION: while a pass is running (k != 0) it is simply k, and
       the body takes the next step.  When k is 0 a pass has ended (or
       none has started yet), and the tail runs: reset both cursors,
       then choose the next pass by the sign of V -- the value of the
       k||(...) is 1 unless the new x is 0, which ends the loop (see
       PASSES).  (Earlier cards assigned that value to P as the 1 a
       flush step inherits; the increment's  P&=2048  now leaves the 0
       a flush step is equally happy with, see the merge notes above.)
         V < 0   (a probe or the render ended)  ->  read the key on
                 descriptor V/=2 into x.  After the probe that descriptor
                 is -2 or -1: the read fails at once, x stays 4, and 4
                 decodes to 1, the render.  After the render it is 0
                 (live: an arrow arrives and decodes to its move code)
                 or -1 (stuck or won: the read fails, x stays 1, and 1
                 decodes to 0: game over).
         else    (a move, the idle pass, or the start)  ->  the dart.
                 V && the dart: on a live turn V is nonzero, V becomes
                 rand()%k+1, a board cell, and an occupied one is a
                 MISS: x=85, the idle pass, then come round and throw
                 again.  When V=0 the board is provably FULL:
                 the && stops before the dart, nothing is drawn from
                 rand(), and the turn goes right.
                 The HIT (or full-board) tuple: the spawn, a plain STORE
                 of 2 shifted by one random bit -- 2 or 4 (50/50) into
                 M[V], the landing cell, empty by the dart's contract
                 (on a full board V is 0 and the junk lands in M[0], the
                 cell no board pass reads); then V=-4, the probe's base;
                 the tuple is negated, so its value is 4, the probe
                 code: x=4.
       INCREMENT: the WIN DELIVERY alone (see header), as a compound
       assignment: V-=P&=2048 takes the tile's 2048 bit off V and leaves
       P as exactly that bit -- 0 after every ordinary step -- so a read
       iteration overwrites it and a flush iteration inherits exactly
       the 0 it needs (merge notes, above): a flush step needs no P of its own,
       and the condition no longer hoists one.  The hold update used to
       live here as well; it is now the body's own XOR (see the body). */
    for(system("stty cbreak");
        k||(j=k=16,
            x=V<0
                ?   read(V/=2,&x,3),x%52946/4
                :   V&&M[V=rand()%k+1]
                    ?   85
                    :   -(M[V]=2<<rand()%2,V=-4));
        V-=P&=2048)
        /* The body, one step of the pass, a single expression, and that
           expression is an XOR INTO THE HOLD:
             l ^= flush? || (read, and emit?)
                    ?  V |= !(the store), l|P      (write arm)
                    :  P                           (skipped store)
           -- the || short-circuits past the read exactly when the step is
           a flush, and the ?: takes the write arm exactly when a guarding
           `if` would have.  Flushes (the test is 1): always write.
           Reads (the test is 0, so the || goes on to the printf): fetch
           the cell, maybe print it, and write iff a tile was read while
           one is held (P*l != 0) -- the emit.  The MOTION raise happens
           at the write site, keyed on the value written.  Either arm
           then yields the DELTA that turns the old hold into the new:
           on the write arm it is l|P, and l^(l|P) is 0 after a flush
           (P is 0 there) and after a merge (P==l), else exactly the
           tile just read (l and P are distinct single bits); on the
           skipped arm it is P, and l^P keeps l when the cell was empty
           (P==0) and takes P when nothing was held (l==0) -- the only
           two ways to reach it.  Every l on the right-hand side is the
           OLD hold: the update of l is sequenced after the whole
           right-hand side (C11 6.5.16), so there is no unsequenced
           hazard. */
        l^=k>j>j%4
        ||  printf(x^1?"\e[H\e[J\n":"%4.d|%c",P=M[j*x%17],j%4*10)
                                              /* the READ is printf's second argument:
                                                 the cell lands in P and is printed.
                                                 Only the x=1 pass renders; every other
                                                 pass prints the (idempotent) screen
                                                 clear instead, see header -- the
                                                 arguments are evaluated either way.
                                                 %4.d: a period with no digits is
                                                 precision ZERO (C99 7.19.6.1), so this
                                                 is %4.0d -- BLANKS for 0.  The %c rides
                                                 the row \n: j is still the cursor BEFORE
                                                 this read (it steps below, after the
                                                 call), so the 4th cell of a row is
                                                 j%4==1 and j%4*10 is 10 there; the
                                                 other cells get 0, 30, 20 (NUL, RS, DC4)
                                                 -- C0 bytes which the terminal discards
                                                 unrendered, like the NUL they replace.
                                                 Within the argument list nothing else
                                                 reads P or writes j, so the list is
                                                 free of unsequenced hazards. */
            &&j--*P*l                     /* printf returned nonzero (it printed),
                                                 so now -- after the sequence point --
                                                 step the read cursor and answer "emit?"
                                                 with P*l. */
            /* The one write site, shared by moves, probes and the idle
               pass.  Real moves write the board cell k*x%17; the probes
               write the same cell lifted into scratch by 68/x and the
               idle pass writes M[0] (see header), so k paces identically
               on dry passes and nothing on the board is touched.  The
               value written is the MOTION witness: at a flush it is the
               hold l, so a stored 0 means this line compacted (gap or
               merge) -- and emit or merge writes are never 0 -- hence
               the !(the store).  The comma then hands the arm's delta,
               l|P, up to the XOR; the bare P is the skipped store. */
            ?   V|=!(M[k--*x%17|68/x]=l+P&~P),l|P
            :   P;
    /* Game over: the read failed.  V is -1 (stuck: -2, halved) or some
       thousand below zero (won), so ~V -- zero exactly for -1 -- chooses
       the string. */
    puts(~V?"WIN":"LOSE");
}
