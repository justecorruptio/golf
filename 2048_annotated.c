/* 2048 in 362 bytes -- a complete terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int. s()'s scratch variables (i,j,l,P,B) are GLOBALS
   rather than params -- byte-neutral vs a long param list, and it lets one
   of them, l, persist its value between calls (see the loop note below).

   The board M[0..15] is the 4x4 grid in row-major order; the throwaway
   spawns of full-board turns land in the scratch row M[16..31], which is
   write-only (the retry loop's mask below means nothing ever acts on a
   scratch read).  The probes write NOTHING at all: the one write site is
   gated on x%2, and probes are even (see below) -- their slide runs dry,
   pure state-machine, board untouched.

   Everything runs through one routine, s(x): it slides and merges every
   line toward its start, in direction x. Conceptually the cell index is
   the stride  i*I + G*(c ^ m)  -- I = x%5 is the row coefficient
   (1 vertical / 4 horizontal), G = 5-I its swap, and m = x%7 (in {0,3})
   the reflect mask.  The cursors run DOWNWARD (4 to 0, so the loop tests
   are the bare `j`/`k`), which reverses every scan -- so each move
   carries the OPPOSITE reflect of the classic layout, and the two terms
   still never share bits: + is XOR, and G*(c^m) = G*c ^ G*m since G is a
   shift.  The whole line-constant part folds into one accumulator:
        B = i*I ^ G*m        (written  4/G*i ^ x%7*G)
   and a cell is just  M[B ^ G*cursor].  I never appears by name: the
   axis pair (I,G) is always {1,4}, so I*G = 4 exactly and I = 4/G --
   the modulus that computed G already paid for both.
   Each turn calls s three times:
        s(56)   probe the vertical axis   (a dry-run sentinel: I=1)
        s(94)   probe the horizontal axis -- and draw the board on the way
                (its reflect 3 un-mirrors the descending scan, so rows
                still print left-to-right)
        s(k*3%42972)  the real move
   The decode TRIPLES k first: the arrow codes are an arithmetic
   progression, and *3 re-lands them on the one modulus universe whose
   reflect pattern matches descending scans (no plain k%N does; this
   was censused).  Moves are always ODD, so parity (x%2) is the gate on
   the one write site; the even sentinels 56/94 are dry-runs that write
   nothing.  The one even value carrying bit 1 (94) is the pass that
   renders (x&2).

   SPAWN: a dart.  Each turn throws one uniform-random dart i=rand()%16;
   an empty cell catches it and receives the new tile, an occupied cell
   just re-rolls -- the dart is a bare for-loop that spins until it lands
   (a fresh dart per spin, no key consumed, so exactly one tile still
   appears per keypress, and every retry consumes fresh entropy).  The
   loop needs a certificate that an empty cell exists, or a full board
   would spin forever -- that certificate is W's bit 0, double-armed (see
   below), and it rides IN THE INDEX: +W%2*16 lifts a full board's darts
   into the scratch region -- while the same bit zeroes the loop mask, so
   the routed dart also exits the loop at once (see main).

   W carries two status bits, each rebuilt in its own window every turn.
   Cell reads do W|=P, piling tile values into W. Tiles are even, so bit 0
   is never touched by that and works as a flag armed twice per turn:
     arm 1  W=3 rides the read() byte count -- armed just before the MOVE,
            whose flushes clear it (W&=~!l) iff a line compacts, i.e. iff
            the board has an empty cell afterward.  So at dart time,
            bit 0 == 0 certifies the retry loop terminates.
     arm 2  W=G||system(...) re-arms it after the spawn for the PROBES;
            if neither probe finds a gap or merge, bit 0 survives = stuck.
   Bit 11 lights iff some tile reached 2048 (probes re-pile the values).
   The endgame test W&2049 selects the stuck and win bits together. */

M[99],W,k,G,i,j,l,P,B;

/* s(x): slide + merge every line in direction x.
       i = current line     j = read cursor     k = write cursor
       l = the held tile (awaiting a landing spot, or an equal to merge with)
       B = this line's index base (row term XOR reflect term)
   Tiles are powers of two, which keeps the whole merge in bare bit-ops --
   and makes ONE write expression serve both emit and flush:  l+P&~P  is
   2l on a merge (l==P), l on a block, and degenerates to exactly l when
   P is 0 or 1 (l is always even).  The flush arm forces P=1 -- and that
   1, the only odd P in the program, then doubles as the PHASE FLAG: the
   hold update  P&~-~l  is P&~l untouched for even tiles but exactly 0
   at a flush, so the loop needs no second phase test.  Bit 0's fourth job.
   (~-~l is ~(l+1); l is always even, so l+1 == l|1 and this one unary
   ripple equals the two-mask form ~l&~1 everywhere it runs.)

   The inner for uses all three slots: INIT computes the line base B once
   (it is line-constant -- it squatted in the body for five generations
   before anyone noticed it didn't need per-iteration recompute), the
   BODY is a bare if around the one write site (statement grammar: no
   ternary, no :0 arm), and the INCREMENT carries the hold update, which
   thereby runs after the write every iteration, exactly as before. */
s(x){
    G=5-x%5;                                  /* position coeff: 4 (vertical) or 1
                                                 (horizontal); the row coeff is recovered
                                                 as 4/G in B (I*G==4), the reflect mask
                                                 x%7 is used inline there too */
    for(i=4;i--;)                             /* 4 lines; the row's \n is folded into
                                                 the 4th cell's printf below */
        /* j,k reset per line to 4 and COUNT DOWN -- the loop tests are the
           bare j and k.  NOT l: every line ends with l==0 (the final flush
           zeroes it via the mask), and l is a global, so that 0 carries
           into the next line; re-zeroing is redundant.  The first call
           inherits l==0 from the zero-init globals. */
        for(B=4/G*i^x%7*G,j=k=4;k;
            /* Increment slot: the new hold, phase-free (see header).
               Reads: P&~(l+1) is 0 right after a merge, else P; an empty
               cell (P==0) keeps l.  Flushes: P==1 makes the whole mask 0
               (bit 0 of ~(l+1) is 0 for even l).  W|= accumulates every
               held value -- the win bit rides here (tile bits only:
               every branch value is even or 0). */
            W|=l=P?P&~-~l:l)
            /* Decide whether to write.
               Reads (j<4): fetch the cell, maybe print it, and write iff a
               tile was read while one is held (P*l != 0) -- the emit.
               Flushes (j>3): record the compaction in W's bit 0 (l==0 here
               means this line compacted -- gap or merge -- so the board is
               movable), force P=1 (also the arm's value, so flushes always
               write), and write out the hold. */
            if(j
            ?   P=M[B^G*--j],                 /* read cell (--j comma-sequenced
                                                 before the printf args below) */
                x&2&&printf("%4.d|%c",P,!j*10),/* only the x=94 pass renders (bit 1 is
                                                 unique to 94). %4.d: a period with no
                                                 digits is precision ZERO (C99 7.19.6.1),
                                                 so this is %4.0d -- BLANKS for 0.  The %c
                                                 rides the row \n: j has counted down to 0
                                                 exactly at the 4th cell, so !j*10 is \n
                                                 there and a NUL (which the terminal
                                                 ignores) before. */
                P*l
            :   (W&=~!l,P=1)
            /* The one write site, gated on parity: real moves (x odd) write
               the board slot B^G*(--k), probes (x even) write nothing --
               their pass is pure state machine.  The decrement is spelled in
               BOTH arms so k still counts every would-be write and the flush
               phase paces identically on dry passes; that duplicate exactly
               replaces the comma-split the ascending form needed. */
            )x%2?M[B^G*--k]=l+P&~P:--k;
}

/* main(): one turn per entry, tail-recursing into the next.  rand() is
   left unseeded, so every game replays identically.  k is dual-role:
   s()'s write cursor above, and the read buffer for the 3-byte arrow
   escape below. */
main(){
    /* The dart, as a bare retry loop: spin while the board is live AND the
       cell is occupied.  ~W%2 is -1 (all ones) when bit 0 is clear and 0
       when set, so the & both applies that mask and keeps M[i] evaluated
       without short-circuiting -- i is assigned on every roll.  Full-board
       routing stays fused in the index: bit 0 set lifts the dart into
       scratch [16,31] and simultaneously zeroes the mask, so a routed dart
       exits the loop at once regardless of what junk sits in the sink --
       scratch stays write-only.  On live boards the mask is all ones and
       the loop spins exactly until an empty board cell catches the dart.
       The screen clear rides this rand's argument -- retries re-clear,
       invisibly. */
    for(;~W%2&M[i=rand(puts("\e[H\e[J"))%16+W%2*16];);
    /* The dart landed: play the turn.
       The spawn: a 2 or 4 (50/50) into the dart cell -- a real cell on a
       live board, a scratch cell (a throwaway) on a full one.  This rand's
       argument carries arm 2: re-arm bit 0 for the probes, and put the
       terminal in cbreak (per-key) mode exactly once -- G is 0 only on the
       very first turn (s has never run), and || short-circuits the
       system() call away after that.  (k no longer works as this gate: the
       descending cursors end every line at 0.)  (The dart above reads W,
       this writes it -- the loop's exit sequences them.) */
    M[i]=2<<rand(W=G||system("stty cbreak"))%2,
    s(56),s(94),                              /* probe both axes; s(94) redraws */
    /* Game over?  W&2049 reads stuck+win together.  Otherwise read the
       next arrow into k -- and W=3 rides the byte count (arm 1): bit 0
       set just before the move, so the move's flushes can prove an empty
       cell exists for the next dart.  Bit 1 is junk range, harmless. */
    W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,W=3)|main(s(k*3%42972));
}
