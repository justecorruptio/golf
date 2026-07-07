/* 2048 in 373 bytes -- a complete terminal 2048.
   Build & play:  cc 2048.c -o 2048 && ./2048    (arrow keys slide;
   make a 2048 tile to win, fill the board with no move left to lose)

   Style: K&R implicit int. s()'s scratch variables (i,j,l,P,t,B) are GLOBALS
   rather than params -- byte-neutral vs a long param list, and it lets one
   of them, l, persist its value between calls (see the loop note below).

   The board M[0..15] is the 4x4 grid in row-major order; everything at
   M[16] and above is scratch.  The probes park their dry-run writes at
   M[B^(G*k+x)] -- their own argument lifts the write into scratch
   (landing in M[56..79]) -- and the throwaway spawns of full-board turns
   land in M[16..31].  All scratch is write-only (the W%2 pre-gate below
   means nothing ever reads it), so it may be overwritten forever.

   Everything runs through one routine, s(x): it slides and merges every
   line toward its start, in direction x. Conceptually the cell index is
   the stride  i*I + G*(c ^ m)  -- I = x%11 is the row coefficient
   (1 vertical / 4 horizontal), G = 5-I its swap, and m = x%7 (in {0,3})
   the reflect mask that flips a line for Right/Up.  But the two terms
   never share bits (one is the 4s pair, the other the 1s pair), so + is
   XOR -- and since G is 1 or 4 (a shift), G*(c^m) = G*c ^ G*m.  The whole
   line-constant part folds into one accumulator computed per line:
        B = i*I ^ G*m        (written  x%11*i ^ x%7*G)
   and a cell is just  M[B ^ G*cursor].
   Each turn calls s three times:
        s(56)   probe the vertical axis   (a dry-run sentinel: I=1, no reflect)
        s(70)   probe the horizontal axis -- and draw the board on the way
        s(k%162)  the real move
   The three are told apart entirely by the argument: k%162 is always ODD
   (k is odd, 162 even), so x&1 marks a real move; the even sentinels 56/70
   are dry-runs, writing to the scratch cell instead of the board. The one
   even value carrying bit 6 (70) is the pass that renders (x&64).

   SPAWN: a dart.  Each turn throws one uniform-random dart i=rand()%16;
   an empty cell catches it and receives the new tile, an occupied cell
   sends main into a tail-recursive retry (a fresh dart, no key consumed,
   so exactly one tile still appears per keypress).  The retry needs a
   certificate that an empty cell exists, or a full board would loop
   forever -- that certificate is W's bit 0, double-armed (see below),
   and it rides IN THE INDEX: +W%2*16 lifts a full board's darts into the
   scratch region, whose zero cells catch them, so the spawn lands in the
   sink with no separate routing test at all.

   W carries two status bits, each rebuilt in its own window every turn.
   Cell reads do W|=P, piling tile values into W. Tiles are even, so bit 0
   is never touched by that and works as a flag armed twice per turn:
     arm 1  W=3 rides the read() byte count -- armed just before the MOVE,
            whose flushes clear it (W&=~!l) iff a line compacts, i.e. iff
            the board has an empty cell afterward.  So at dart time,
            bit 0 == 0 certifies the retry loop terminates.
     arm 2  W=k||system(...) re-arms it after the spawn for the PROBES;
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
   P is 0 or 1 (l is always even).  So the flush arm just forces P=1 and
   the single write M[...]=l+P&~P covers every store the engine makes. */
s(x){
    G=5-x%11;                                 /* position coeff: 4 (vertical) or 1
                                                 (horizontal); the row coeff x%11 and
                                                 reflect mask x%7 are used inline in B */
    for(i=4;i--;)                             /* 4 lines; the row's \n is folded into
                                                 the 4th cell's printf below */
        /* Inner loop resets only j,k -- NOT l. Every line ends with l==0 (the
           final flush stores the last held tile and clears l), and since l is
           a global that 0 carries into the next line, so re-zeroing it is
           redundant. The first call inherits l==0 from the zero-init globals. */
        for(j=k=0;k<4;
            /* First expression: decide whether to write.
               Reads (j<4): fetch the cell, maybe print it, and write iff a
               tile was read while one is held (P*l != 0) -- the emit.
               Flushes (j>3): force P=1 (see above) and always write. */
            (j<4
            ?   W|=P=M[B^G*j],                /* read cell; W|=P lights bit 11 at 2048 */
                x&64&&printf("%4.0d|%c",P,j/3*10),/* only the x=70 pass renders (bit 6 is
                                                 unique to 70). %4.0d prints BLANKS for 0;
                                                 the %c rides the row \n -- j is the
                                                 pre-increment index here, so the 4th
                                                 cell is j==3 and j/3*10 is the newline
                                                 (a NUL the terminal ignores before). */
                P*l
            :   (P=1)
            /* The one write site.  k++ rides the index; the +!(x&1)*x lifts
               dry-run writes into scratch (M[56..79]) -- the sentinel is its
               own sink offset -- while real moves (x odd) add 0 and write the
               board slot B^G*k. */
            )?M[B^G*k+++!(x&1)*x]=l+P&~P:0,
            /* Second expression: advance the read cursor and update the hold.
               Reads: P&~l is 0 right after a merge, else P; an empty cell
               (P==0) keeps l. Flushes: record the compaction in W's bit 0
               and clear the hold. */
            l=j++>3?W&=~!l,0:P?P&~l:l
        )B=x%11*i^x%7*G;                      /* line base, refreshed before each read */
}

/* main(): one turn per entry, tail-recursing into the next -- and doubling
   as the spawn's retry loop.  rand() is left unseeded, so every game
   replays identically.  k is dual-role: s()'s write cursor above, and the
   read buffer for the 3-byte arrow escape below. */
main(){
    /* The dart, with the full-board routing fused into the index: W's bit 0
       set (the last move proved nothing compacts = board full) lifts the
       dart into scratch [16,31].  The W%2| pre-gate makes full-board turns
       proceed WITHOUT reading the scratch cell -- scratch is write-only, so
       sink cells can be dirty and it never matters (and the retry loop can
       never be starved: on live boards the dart only reads board cells).
       The screen clear rides this rand's argument -- retries re-clear,
       invisibly. */
    W%2|!M[i=rand(puts("\e[H\e[J"))%16+W%2*16]
    ?   /* The dart landed: play the turn.  This whole chain sits in the
           ternary's MIDDLE, which the C grammar lets hold top-level commas
           -- the else-branch could not.
           The spawn: a 2 or 4 (50/50) into the dart cell -- a real cell on
           a live board, a scratch cell (a throwaway) on a full one.  This
           rand's argument carries arm 2: re-arm bit 0 for the probes, and
           put the terminal in cbreak (per-key) mode exactly once -- k is 0
           only on the very first turn, and || short-circuits the system()
           call away after that.  (The index above reads W, this expression
           writes it -- legal only because the ternary sequences them.) */
        M[i]=2<<rand(W=k||system("stty cbreak"))%2,
        s(56),s(70),                          /* probe both axes; s(70) redraws */
        /* Game over?  W&2049 reads stuck+win together.  Otherwise read the
           next arrow into k -- and W=3 rides the byte count (arm 1): bit 0
           set just before the move, so the move's flushes can prove an empty
           cell exists for the next dart.  Bit 1 is junk range, harmless. */
        W&2049?puts(W>>11?"WIN":"LOSE"):read(0,&k,W=3)|main(s(k%162))
    :   main();                               /* dart bounced off a tile: re-throw */
}
