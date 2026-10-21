/* 2048 in 348 bytes -- a complete terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int. s()'s scratch variables (i,j,l,P,B) are GLOBALS
   rather than params -- byte-neutral vs a long param list, and it lets one
   of them, l, persist its value between calls (see the loop note below).

   The board M[0..15] is the 4x4 grid in row-major order.  The probes
   never touch it: the one write site is unconditional, but on a probe
   pass its index is lifted into scratch (M[16..47], see the write site
   below), so their slide runs as a pure state machine over a throwaway
   region.  Full-board turns change nothing either: the spawn adds 0
   (see main).

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
        s(84)   probe the vertical axis   (a dry-run sentinel: G=4;
                must run FIRST -- its entry zeroes V, see s)
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
   Moves are always ODD; the even sentinels 84/66 are dry runs.  What
   separates them from moves at the write site is not parity but bits
   1-2: 84 carries bit 2, 66 carries bit 1, and no arrow code carries
   either, so x*8&48 is 32, 16 or 0 -- the scratch lift.  The one even
   value carrying bit 1 (66) is the pass that renders (x&2).

   SPAWN: a dart.  Each turn throws one uniform-random dart i=rand()%16;
   an empty cell catches it and receives the new tile, an occupied cell
   just re-rolls -- the dart is a bare for-loop that spins until it lands
   (a fresh dart per spin, no key consumed, so exactly one tile still
   appears per keypress, and every retry consumes fresh entropy).  The
   loop needs a certificate that an empty cell exists, or a full board
   would spin forever -- that certificate is V (see below): V=0 happens
   only when the board is FULL, and then the multiply gate kills the
   condition on the first roll and the spawn, which is ADDITIVE and
   scaled by V, adds 0 to an occupied cell (see main) -- nothing is
   thrown away, nothing changes.

   STATUS: one variable, two channels.  V's LOW BIT is the motion
   flag; V's SIGN is the win flag.
     Motion: events RAISE it -- the write site does V|=!(stored value)
        whenever a flush writes out an EMPTY hold, i.e. whenever a line
        compacts (gap swallowed or merge made).  It is reset
        twice per turn, each reset riding an argument that had to be
        there anyway:  V=0 rides read()'s fd (stdin IS fd 0) just
        before the move -- so at dart time V answers "did that move
        pass see an empty cell or a merge?" (either one leaves an
        empty cell behind, so V=1 certifies the dart lands; V=0 means
        the board is FULL) -- and the vertical probe divides it away
        on entry (V/=G, G=4: see s) -- so at the endgame test V
        answers "can anything still move?".  Between a reset and its readers V is
        EXACTLY 0 or 1, never junk, which is what pays for the dart's
        two-char multiply gate.  Turn one is covered by a PHANTOM
        MOVE (see main): a slide over the empty board whose empty-line
        flushes raise V before the first dart, so no declaration
        initializer is needed.
     Win: the render call delivers it SUBTRACTIVELY as one of
        printf's extra arguments -- C evaluates and IGNORES arguments
        beyond the format string, so the slot is free: every cell the
        render pass reads lands in P, and P&2048 is 2048 exactly for
        a 2048 tile (no lesser tile has bit 11, and 4096 can never
        exist: the game ends the turn the first 2048 appears).
        V-=2048 drives V negative and it STAYS negative -- later
        flushes only OR in bit 0, which cannot flip the sign, and
        further 2048s only subtract more -- and no reset intervenes
        between the render and the test below, which is the whole
        trick: the win only has to survive two commas.  The mask is
        load-bearing twice over: P>>10 would leak a 1024's bit into
        the motion channel (a stuck board holding a 1024 would miss
        its LOSE), and the OR form V|=P&2048 keeps the sign positive
        and forces the costlier test V==1.  Moves never deliver
        (their bit 1 is clear, so x&2 never fires), which is what
        keeps the dart's and the spawn's V strictly 0-or-1.
   The endgame reads both channels in three characters: V>0 continues
   exactly on V==1, movable and unwon -- any win is negative, stuck
   is 0.  At the select V is 0 or negative, so a bare V picks
   WIN/LOSE. */

M[99],V,k,G,i,j,l,P,B;

/* s(x): slide + merge every line in direction x.
       i = current line     j = read cursor     k = write cursor
       l = the held tile (awaiting a landing spot, or an equal to merge with)
       B = this line's index base (row term XOR reflect term)
   Tiles are powers of two, which keeps the whole merge in bare bit-ops --
   and makes ONE write expression serve both emit and flush:  l+P&~P  is
   2l on a merge (l==P), l on a block, and degenerates to exactly l when
   P is 0 or 1 (l is always even).  The loop increment sets P=1 after
   every iteration (a read overwrites it, a flush inherits it) -- and that
   1, the only odd P in the program, then doubles as the PHASE FLAG: the
   hold update  P&~-~l  is P&~l untouched for even tiles but exactly 0
   at a flush, so the loop needs no second phase test.  Bit 0's fourth job.
   (~-~l is ~(l+1); l is always even, so l+1 == l|1 and this one unary
   ripple equals the two-mask form ~l&~1 everywhere it runs.)

   The inner for uses all three slots: INIT computes the line base B once
   (it is line-constant -- it squatted in the body for five generations
   before anyone noticed it didn't need per-iteration recompute), the
   BODY is a bare if around the one write site (statement grammar: no
   ternary, no :0 arm), and the INCREMENT carries the hold update and P=1, which
   thereby runs after the write every iteration, exactly as before. */
s(x){
    V/=G=x%5;                                 /* the stride, extracted DIRECTLY: 4
                                                 (vertical) or 1 (horizontal); the row
                                                 coeff is recovered as 4/G in B (I*G==4),
                                                 the reflect mask x%9 is inline there.
                                                 And G doubles as a FLAG: only the
                                                 vertical probe has G=4, so V/=G zeroes
                                                 V (0 or 1) exactly once per turn, just
                                                 before the probes arm it; the render
                                                 pass (G=1) and moves (V already 0 from
                                                 the read) are untouched */
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
               (bit 0 of ~(l+1) is 0 for even l).  Then P=1, unconditionally:
               a read iteration overwrites it, a flush iteration inherits
               exactly the 1 it needs -- so the flush arm below is empty. */
            l=P?P&~-~l:l,P=1)
            /* Decide whether to write.
               Reads (j<4): fetch the cell, maybe print it, and write iff a
               tile was read while one is held (P*l != 0) -- the emit.
               Flushes (j>3): the arm is a bare 1 -- always write.  P is
               already 1 (set by the increment) and the MOTION raise happens
               at the write site, keyed on the value written. */
            if(j
            ?   P=M[B^G*--j],                 /* read cell (--j comma-sequenced
                                                 before the printf args below) */
                x&2&&printf("%4.d|%c",P,!j*10,V-=P&2048),
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
            :   1
            /* The one write site, UNCONDITIONAL.  Real moves write the board
               slot B^G*(--k); probes write the same slot lifted into scratch
               by x*8&48 (32 for the vertical probe, 16 for the render -- no
               arrow code has bits 1-2, so moves get 0), so k paces identically
               on dry passes and nothing on the board is touched.  The value
               written is the MOTION witness: at a flush it is the hold l, so
               a stored 0 means this line compacted (gap or merge) -- and emit
               or merge writes are never 0 -- hence V|=!(the store). */
            )V|=!(M[B^G*--k|x*8&48]=l+P&~P);
}

/* main(): the turn loop.  rand() is left unseeded, so every game replays
   identically.  The loop is a bare for: its CONDITION is the endgame
   test, its BODY reads the key, makes the move and throws the dart, and
   its INCREMENT spawns and probes -- test, read, move, dart, spawn,
   probes, test: the turn cycle exactly, with the test sitting right
   after the probes.  Turn one needs no special casing, because two
   variables are 0 at declaration and a useful constant ever after:
     B is 3 after every render pass (last line, i=0, G=1, 66%9=3), so
       read(V=0,&k,B) reads NOTHING on turn one (POSIX: a zero-byte read
       returns 0 at once) and the three arrow bytes on every later turn;
     G is 1 after every render pass, so V>=G is trivially true on turn
       one and exactly V==1 afterwards.
   k is dual-role: s()'s write cursor above, and the read buffer for the
   3-byte arrow escape below. */
main(){
    /* The outer for.  CONDITION V>=G: continue exactly on V==1 (movable and
       unwon -- any 2048 has driven V negative, a stuck board leaves it 0),
       except on turn one, where G is still 0.  INCREMENT: the spawn and the
       two probes, run after each body.
       The spawn, ADDITIVE and scaled by V: V+V is 2 on a live turn and 0
       on a full board, shifted by one random bit -- so an empty landing
       cell receives 2 or 4 (50/50) and an occupied one receives +0.  No
       gate, no else arm.  (The V reset for the probes is NOT here: V+V
       reads V in this same expression, and a V-write anywhere else in it
       would be unsequenced -- undefined.  It lives in s, riding G.) */
    for(;V>=G;M[i]+=V+V<<rand()%2,s(84),s(66))
        /* The body: a second for whose INIT slot runs once per turn and
           already owns its semicolon.  read() fills k with the arrow (and
           resets the motion flag: V=0 rides the fd argument, stdin IS
           file descriptor 0) and returns the byte count, which selects:
           3 -> THE MOVE, s(k%664*3) (the decode is explained in the
           header); 0 (turn one, B==0) -> !system("stty cbreak"): the
           terminal setup runs exactly once, and its 0 exit status becomes
           the argument 1.  s(1) is a PHANTOM MOVE: a horizontal slide with
           reflect 1 (a permutation of each row), odd so it writes -- but on
           the empty board it writes zeros onto zeros -- and with no render
           bit, so nothing is printed.  Its four empty-line flushes raise V,
           which is exactly the certificate the first dart needs; that is
           why V has no declaration initializer.  (If stty ever failed, the
           argument would be 0 and 4/G would divide by zero -- only possible
           with no terminal attached.)
           Then the dart, as a bare retry loop: spin while the board is live
           AND the cell is occupied.  V is exactly 0 or 1 here (junk-free --
           see header), so the gate is a bare multiply: on live boards (V=1,
           the last move saw an empty or opened one by merging -- or the
           phantom move's raise on the virgin board) the condition is M[i]
           itself and the loop spins exactly until an empty cell catches the
           dart; when V=0 the board is provably FULL, the product dies on the
           first roll, and the spawn adds 0 to whatever occupied cell i
           points at.  The multiply, unlike &&, keeps M[i] evaluated, so the
           clear is emitted (and i assigned) on every roll.  The screen clear
           rides this rand's argument -- retries re-clear, invisibly. */
        for(s(read(V=0,&k,B)?k%664*3:!system("stty cbreak"));M[i=rand(puts("\e[H\e[J"))%16]*V;);
    /* Game over: the loop exited on V<G.  V is 0 (stuck) or negative (won),
       so a bare V chooses the string. */
    puts(V?"WIN":"LOSE");
}
