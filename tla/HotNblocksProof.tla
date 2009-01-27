-------------------- MODULE HotNblocksProof --------------------
(*
Anything labeled PROOF OMITTED  will need to be done before the proof is complete.
Also, we need to figure out what to label the steps that use:
    ((A /\ B ~> C) /\ [](A => B)) => (A ~> C)
*)
EXTENDS HotNblocks

PROOF Prog => HotNblocks

LET OverlapAmt(x) == Cardinality(Overlap(x, Acquired))
      Blocking(x) == { p \in Procs : acquired[p] \in Overlap(x, Acquired) }
      S == Nat \ {0}
      G == ~isHot[x]
      H(c) == x \in Nblocks /\ isHot[x] /\ OverlapAmt(x) = c
IN

------------------------------------------------------------

<1>1. Prog => ((\E c \in S: H(c) ~> ~isHot[x])

 <2>1. Prog /\ c \in S => (H(c) ~> (G \/ \E d \in S : d < c /\ H(d)))
      ASSUME Prog /\ c \in S
      PROVE H(c) ~> (G \/ \E d \in S : d < c /\ H(d))

  <3>1. (\E i \in Procs : H(c) /\ i \in Blocking(x) ~> (G \/ \E d \in S : d < c /\ H(d))

   <4>1. H(c) /\ x \in Procs /\ i \in Blocking(x) ~> (G \/ \E d \in S : d < c /\ H(d))

    <5>1. H(c) /\ i \in Procs /\ i \in Blocking(x) /\ state[i] = nextblock ~> (G \/ \E d \in S : d < c /\ H(d)

     \* Use WF1 on doNextBlock(i)
     LET P == H(c) /\ i \in Procs /\ i \in Blocking(x) /\ state[i] = nextblock
         N == \E j \in Procs : (doNextBlock(j) \/ doSearch(j))
         A == doNextBlock(i) \/ doSearch(i)
         Q == G \/ (H(d) /\ d < c) IN

     <6>1. P /\ [N]_Vars => (P' \/ Q')
           PROOF OMITTED \* TODO Cases, j = i and j # i

     <6>2. P /\ <<N /\ A>>_Vars => Q'
           PROOF OMITTED \* TODO

     <6>3. P => ENABLED<<A>>_Vars
           PROOF OMITTED \* TODO

    <5>2. H(c) /\ i \in Procs /\ i \in Blocking(x) /\ state[i] = search ~> (G \/ \E d in \in S : d < c /\ H(d))

     <6>1. H(c) /\ i \in Procs /\ i \in Blocking(x) /\ state[i] = search
           ~> (G \/ \E d \in S : d < c /\ H(d)) \/ H(c) /\ i \in Procs /\ i \in Blocking(x) /\ state[i] = nextblock

      LET P == H(c) /\ i \in Procs /\ i \in Blocking(x) /\ state[i] = search
          N == \E j \in Procs : (doNextBlock(j) \/ doSearch(j))
          A == doNextBlock(i) \/ doSearch(i)
          Q == (G \/ (H(d) /\ d < c)) \/ (H(c) /\ i \in Procs /\ i \in Blocking(x) /\ state[x] = nextblock)

      <7>1. P /\ [N]_Vars => (P' \/ Q')
            PROOF OMITTED
            \* TODO two cases:
            \* i = j then this leads to the LHS of <5>1 and we have it
            \* i # j two cases here too:
            \*      - Either j will release a block in Overlap(x, Acquired) and we have it (\E d \in S : d < c /\ H(d))
            \*      - or j won't release a block in Overlap(x, Acquired), but that is fine, we have WF1 on i=j so we
            \*        should be good (ENABLED<<doSearch(j) /\ j=i>> is good).

      <7>2. P /\ <<N /\ A>>_Vars => Q'
            PROOF OMITTED \* TODO

      <7>3. P => ENABLED<<A>>_Vars
            PROOF OMITTED \* TODO

     <6>2. QED BY <6>1 and <5>1 \* ((P ~> Q \/ R) /\ (R ~> Q)) => (P ~> Q)

   <4>2. QED BY <4>1 \* UG then change \A to \E (which is allowed).

  <3>2. Prog => [](H(c) => \E i \in Procs : acquired[i] \in Overlap(x, Acquired))
        PROOF OMITTED \* TODO

  <3>3. QED BY <3>1 and <3>2 \* TODO: what is this called?
        \* I dunno what this is called... since H(c) implies that something is acquired in the overlap
        \* we can add/remove the overlap part.

 <2>2. QED \* Lattice rule with <2>1

------------------------------------------------------------

<1>2. Prog => [](x \in Nblocks /\ isHot[x] => \E c \in S : H(c))

 LET I == x \in Nblocks /\ isHot[x] => \E c \in S : H(c) /\ x \notin HotInterference(Acquired)
       N == Next IN
       
 <2>1. Prog => [](I)
 
  <3>1. Init => I
        OBVIOUS \* Nothing is hot in Init, and therefore the implication holds trivially.

  <3>2. I /\ [\E i \in Procs : (doNextBlock(i) \/ doSearch(i))]_Vars => I'

   <4>1. I /\ Vars = Vars' => I'
         OBVIOUS \* Studdering step.

   <4>2. I /\ (\E i \in Procs : doNextBlock(i)) => I'
         ASSUME I /\ \E i \in Procs : do NextBlock(i)
         PROVE I'

    <5>1. CASE isHot[x]

     <6>1. CASE x \in Free(Acquired \ acquired[i])
           OBVIOUS \* isHot'[x] = FALSE, and the implication holds trivially since the LHS is FALSE.

     <6>2. CASE x \notin Free(Acquired \ acquired[i])

      <7>1. Cardinality(Overlap(x, Acquired)) > 0

       <8>1. x \notin HotInterference(Acquired \ acquired[i])

        <9>1. HotInterference(Aqcuired \ acquired[i]) \subseteq HotInterference(Acquired)

         <10>1. Busy(Acquired \ acquired[i]) \subset Busy(Acquired \ acquired[i])
                OBVIOUS \* There are less acquired blocks, and therefore there are less busy blocks.

         <10>2. Overlap(x, Acquired \ acquired[i]) \subseteq Overlap(x, Acquired)

          <11>1. CASE acquired[i] \in Succs[x]
                 OBVIOUS \* There is less overlap

          <11>2. CASE acquired[i] \notin Succs[x]
                 OBVIOUS \* The overlap set doesn't change

          <11>3. QED BY <11>1 and <11>2

         <10>3. QED BY <10>1 and <10>2 \* If there is the same or less overlap, then the hot interference is the same or smaller.

        <9>2. x \notin HotInterference(Acquired)
              BY <4>2 and <5>1 \* By the assumption of the invariant and the case assumption this is trivial.

        <9>3. QED BY <9>1 and <9>2

       <8>2. QED BY <6>2 and <8>1 \* ((<6>2 => <7>1 \/ ~<8>1) /\ <6>2 /\ <8>1) => <7>1

      <7>2. x \notin HotInterference(Acquired')
            PROOF OMITTED

      <7>3. QED BY <7>1 and <7>2

     <6>3. QED BY <5>1 and <5>2

    <5>2. CASE ~isHot[x]
          PROOF OMITTED \* Should be obvious, isHot only gets more false.

    <5>3. QED BY <5>1 and <5>2

   <4>3. I /\ (\E i \in Procs : doSearch(i)) => I'

    <5>1. CASE isHot[x]

     <6>1. isHot'[x]
           OBVIOUS \* Nothing becomes cold in the doSearch action.

     <6>2. x \notin HotInterference(Acquired)
           OBVIOUS \* If a block y becomes hot, it is not in the interference scope of x.

     <6>3. OverlapAmt(x)' = OverlapAmt(x)
           OBVIOUS \* UNCHANGED<<acquired, Succs>>

     <6>4. QED BY <6>1, <6>2 and <6>3


    <5>2. CASE ~isHot[x]
          PROOF OMITTED \* TODO

    <5>3. QED BY <5>1 and <5>2

   <4>4. QED BY <4>1, <4>2 and <4>3

  <3>4. QED BY <3>1, <3>2 \* INV1

 <2>2. I => x \in Nblocks /\ isHot[x] => \E c \in S : H(c)
       OBVIOUS \* Weakening on the right side of the implication.

 <2>3. QED BY <2>1 and <2>2

------------------------------------------------------------

<1>3. Prog => (x \in Nblocks /\ isHot[x] ~> ~isHot[x])
      BY <1>1 and <1>2 \* TODO: what is this called?
      \* Since <1>2 is a safety property (which we will demonstrate in <1>3), we can add/remove it somehow at our leasure.
      \* What we really want here is:
      \* ((A /\ B ~> C) /\ [](A => B)) => (A ~> B)

<1>4. Prog => \A x \in Nblocks : isHot[x] ~> ~isHot[x]
      BY <1>3 \* Universal Generalization

<1>5. QED BY <1>4
============================================================
