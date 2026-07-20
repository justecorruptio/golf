/* 2048 in 355 bytes -- a complete terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int. s()'s scratch variables (i,j,l,P,B) are GLOBALS
   rather than params -- byte-neutral vs a long param list, and it lets one
   of them, l, persist its value between calls (see the loop note below).

   The board M[0..15] is the 4x4 grid in row-major order; nothing ever
   writes past M[15] (the rest of M[99] is free headroom).  The probes
   write NOTHING at all: the one write site is gated on x%2, and probes
   are even (see below) -- their slide runs dry, pure state-machine,
   board untouched.  Full-board turns write nothing either: the spawn
   itself is skipped (see main).

   Everything runs through one routine, s(x): it slides and merges every
   line toward its start, in direction x. Conceptually the cell index is
   the stride  i*I + G*(c ^ m)  -- G = x%5 is the within-line stride
   (4 vertical / 1 horizontal), I = 4/G the row coefficient (I*G == 4),
   and m = x%9 (in {0,3}) the reflect mask.  The cursors run DOWNWARD (4
   to 0, so the loop tests are the bare `j`/`k`), which reverses every
   scan -- so each move carries the OPPOSITE reflect of the classic
   layout, and the two terms still never share bits: + is XOR, and
   G*(c^m) = G*c ^ G*m since G is a shift.  The whole line-constant part
   folds into one accumulator:
        B = i*I ^ G*m        (written  4/G*i ^ x%9*G)
   and a cell is just  M[B ^ G*cursor].  I never appears by name, and G
   needs no arithmetic at all: the modulus extracts the STRIDE directly
   (the axis pair is {1,4} with I*G = 4, so I = 4/G).
   Each turn calls s three times:
        s(84)   probe the vertical axis   (a dry-run sentinel: G=4)
        s(66)   probe the horizontal axis -- and draw the board on the way
                (its reflect 3 un-mirrors the descending scan, so rows
                still print left-to-right)
        s(k%664*3)  the real move
   The decode reduces mod 664 and TRIPLES the residue: k%664*3 (the
   cheap twin of k*3%1992, since kc mod cN = c(k mod N)).  Tripling
   re-lands the arrow codes' arithmetic progression on a universe whose
   reflect pattern matches descending scans AND whose axis residues are
   the strides themselves -- no plain k%N reaches either, and a census
   of every two-operation decode form shows this one is unique.
   Moves are always ODD, so parity (x%2) is the gate on the one write
   site; the even sentinels 84/66 are dry-runs that write nothing.  The
   one even value carrying bit 1 (66) is the pass that renders (x&2).

   SPAWN: a dart.  Each turn throws one uniform-random dart i=rand()%16;
   an empty cell catches it and receives the new tile, an occupied cell
   just re-rolls -- the dart is a bare for-loop that spins until it lands
   (a fresh dart per spin, no key consumed, so exactly one tile still
   appears per keypress, and every retry consumes fresh entropy).  The
   loop needs a certificate that an empty cell exists, or a full board
   would spin forever -- that certificate is V (see below): V=0 happens
   only when the board is FULL, and then the multiply gate kills the
   condition on the first roll and the spawn statement is skipped
   outright (a ternary on V, see main) -- nothing is thrown away,
   nothing is written.

   STATUS: two flags, one bit each, split by job.
     V is the MOTION flag, and events RAISE it: the flush arm does
        V|=!l whenever a line compacts (gap swallowed or merge made).
        It is reset twice per turn, each reset riding an argument that
        had to be there anyway:  V=0 rides read()'s fd (stdin IS fd 0)
        just before the move -- so at dart time V answers "did that
        move pass see an empty cell or a merge?" (either one leaves an
        empty cell behind, so V=1 certifies the dart lands; V=0 means
        the board is FULL) -- and V=!G&&... rides the stty gate just
        before the probes -- so at the endgame test V answers "can
        anything still move?".  Between a reset and its readers V is
        EXACTLY 0 or 1, never junk, which is what pays for the dart's
        two-char multiply gate.  The declaration's V=1 covers turn
        one: the first dart must land although no move preceded it
        (16 empty cells make their own certificate).
     W is the WIN flag and nothing else.  It rides the render call
        as one of printf's extra arguments -- C evaluates and IGNORES
        arguments beyond the format string, so the slot is free: every
        cell the render pass reads lands in P, and P>>11 is 1 exactly
        for a 2048 (no lesser tile has bit 11).  The render pass scans
        all 16 cells each turn, and the board still holds a fresh 2048
        when it runs (moves write it, probes never overwrite it), so
        the win is delivered before the same turn's endgame test.
        Keeping the stuck bit out of W is what legalizes the bare
        shift: no mask, no collision.  (Six other homes tie at this
        byte count -- capturing the hold update in the loop increment,
        the read site, the merge write, the loop condition -- the
        delivery is 8 chars plus one glue comma wherever it lives.)
   The endgame reads both flags in two characters: V>W continues
   exactly on (V,W) = (1,0), movable and unwon. */

M[99],V=1,W,k,G,i,j,l,P,B;

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
    G=x%5;                                    /* the stride, extracted DIRECTLY: 4
                                                 (vertical) or 1 (horizontal); the row
                                                 coeff is recovered as 4/G in B (I*G==4),
                                                 the reflect mask x%9 is inline there */
    for(i=4;i--;)                             /* 4 lines; the row's \n is folded into
                                                 the 4th cell's printf below */
        /* j,k reset per line to 4 and COUNT DOWN -- the loop tests are the
           bare j and k.  NOT l: every line ends with l==0 (the final flush
           zeroes it via the mask), and l is a global, so that 0 carries
           into the next line; re-zeroing is redundant.  The first call
           inherits l==0 from the zero-init globals. */
        for(B=4/G*i^x%9*G,j=k=4;k;
            /* Increment slot: the new hold, phase-free (see header).
               Reads: P&~(l+1) is 0 right after a merge, else P; an empty
               cell (P==0) keeps l.  Flushes: P==1 makes the whole mask 0
               (bit 0 of ~(l+1) is 0 for even l). */
            l=P?P&~-~l:l)
            /* Decide whether to write.
               Reads (j<4): fetch the cell, maybe print it, and write iff a
               tile was read while one is held (P*l != 0) -- the emit.
               Flushes (j>3): raise the MOTION flag (l==0 here means this
               line compacted -- gap or merge -- so the board changed /
               can change, whichever question V is currently answering),
               force P=1 (also the arm's value, so flushes always write),
               and write out the hold. */
            if(j
            ?   P=M[B^G*--j],                 /* read cell (--j comma-sequenced
                                                 before the printf args below) */
                x&2&&printf("%4.d|%c",P,!j*10,W|=P>>11),
                                              /* only the x=66 pass renders (bit 1 is
                                                 unique to 66). %4.d: a period with no
                                                 digits is precision ZERO (C99 7.19.6.1),
                                                 so this is %4.0d -- BLANKS for 0.  The %c
                                                 rides the row \n: j has counted down to 0
                                                 exactly at the 4th cell, so !j*10 is \n
                                                 there and a NUL (which the terminal
                                                 ignores) before.  The LAST argument is
                                                 the WIN DELIVERY (see header): printf
                                                 evaluates it and ignores it -- a free
                                                 expression slot, once per rendered cell. */
                P*l
            :   (V|=!l,P=1)
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
       cell is occupied.  V is exactly 0 or 1 here (junk-free -- see header),
       so the gate is a bare multiply: on live boards (V=1, the last move
       saw an empty or opened one by merging -- or the declaration's V=1 on
       the virgin board) the condition is M[i] itself and the loop spins
       exactly until an empty cell catches the dart; when V=0 the board is
       provably FULL, the product dies on the first roll, and i is never
       looked at again -- the ternary below skips the spawn store entirely.
       The multiply, unlike &&, keeps M[i] evaluated, so the clear is
       emitted (and i assigned) on every roll.  (The old unified flag
       carried junk bits that forced ~W%2&M[...] here -- the clean V buys
       the short gate.)  The screen clear rides this rand's argument --
       retries re-clear, invisibly. */
    for(;M[i=rand(puts("\e[H\e[J"))%16]*V;);
    /* The spawn, gated on V: a 2 or 4 (50/50) into the dart cell -- but
       only when the board can take it.  When V=0 (full board, dart done
       in one roll) the else arm does nothing: no store, no rand consumed.
       The skip is legal because the middle arm's side effects are
       IDEMPOTENT on the skip path: V is already 0, and the terminal setup
       can't still be pending (stty belongs to turn one, where the
       declaration's V=1 guarantees the middle arm runs).  On V=1 turns
       this rand's argument resets V for the probes ("assume stuck, let
       the flushes prove otherwise") and gates the setup: G is 0 only on
       the very first turn (s has never run), so !G&& runs system()
       exactly once -- and V lands 0 either way (turn 1: 1 && stty's 0
       exit; after: 0 && short-circuit).  (k can't serve as that gate:
       descending cursors end every line at 0.  The ?: sequence point
       orders the gate's V-read before the arm's V-write.) */
    V?M[i]=2<<rand(V=!G&&system("stty cbreak"))%2:0,
    s(84),s(66),                              /* probe both axes; s(66) redraws */
    /* Game over?  V>W reads both flags at once: continue exactly when
       movable and unwon (V=1, W=0); any 2048 prints WIN (movable or not),
       stuck without one prints LOSE.  Otherwise read the next arrow into
       k -- and V=0 rides the fd argument (stdin IS file descriptor 0),
       resetting the motion flag just before the move whose flushes may
       raise it, so the next dart routes honestly.  The COMMA between the
       read and the recursion is a sequence point (a ternary's middle arm
       is a full expression, commas welcome): the key is read into k
       strictly before the decode reads k -- no evaluation-order luck. */
    V>W?read(V=0,&k,3),main(s(k%664*3)):puts(W?"WIN":"LOSE");
}
