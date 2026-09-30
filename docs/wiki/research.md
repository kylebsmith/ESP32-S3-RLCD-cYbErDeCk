# Research — fewer words with parameters, or more words?

*Part of [the deck, top to bottom](README.md). 2026-09-28. My question, in my
words: "is there research about that distinction of more typing and more layered functions
with parameters versus individual, but more functions?" Every source below was checked
against its publisher's record or the authors' own copy on 2026-09-28. Each entry says what
was read — the full text, an author's copy, or the abstract only.*

## The short answer

**Yes — the question is forty years old, and the answer is neither "fewer" nor "more".
Specific words, inside one consistent structure, beat both one-off names and vague
general ones.**

1. **No study tests exactly this question** — the same functions as many named commands
   against few commands with parameters — and none in live coding or music. No paper
   settles `>disc 8` against `>shape round 8`.
2. **Structure beats one-off names**, and more so as the set grows:
   - a shared verb with an object (`CREATE MESSAGE`, `CREATE GROUP`) was recalled far
     better than a separate verb per function (Scapin 1982);
   - longer keybindings built by one rule were recalled more than twice as well as shorter
     idiosyncratic ones (Walker & Olson 1988);
   - a language whose parts combine independently did best on every measure (Bowden,
     Douglas & Stanford 1989).

   Task-action grammar explains why: what a learner holds is the number of *rules*, not
   the number of words (Payne & Green 1986).
3. **But specific words beat vague ones.** Names that say exactly what they do beat general
   words, which did worst of all — below nonsense names (Black & Moran 1982; Grudin &
   Barnard 1984). One name stretched over operations written differently slowed learning,
   whatever the words were (Landauer, Galotti & Hartwell 1983). A parameter every command
   shares is learned best in one fixed place, first (Barnard et al. 1981).
4. **Decisions cost more than letters.** For a practised typist one moment of deciding
   costs about as much as seven keystrokes (Card, Moran & Newell 1980). The question is
   not how many letters `shape round` adds, but whether it is one habit or two decisions.
   If short forms are offered, one stated rule — keep the front of the word — beats
   hand-picked ones (Streeter et al. 1983; Ehrenreich 1985).
5. **The live-coding papers argue for terse, small, constrained vocabularies**, because of
   time pressure and mental load (Magnusson 2010; McLean & Wiggins 2010). They are design
   arguments, not measurements. A small comparison on the deck itself would add something
   the literature does not have.

## 1. What this means for the deck

**The deck already has the shape the research favours: specific words, one rule.**

- **Every lane line has one rule**: a name, then a pattern — `>kick x...`, `>disc 8`,
  `>cut 0369`. Thirty-three sound and picture words cost one rule, not thirty-three.
- **The names are specific and plain**: `disc`, `box`, `noise`, `echo` — not `shape`,
  `effect` or `thing`.
- **The shared parameter sits in one place**: the pattern always follows the name.
- **What varies continuously is a parameter**: amounts are digits, positions are `:x`
  `:y`, and the four directions are one mark in a pattern, not four verbs
  ([NEXT.md](../NEXT.md) §3.10).
- **Operations on lanes are verb plus object**, the structure Scapin measured:
  `>mute kick`, `>solo bass`, `>route disc kick`.
- **The names are my**: `>conga = note 63`, `>circle = disc`. That is the
  aliasing Furnas et al. recommend.
- **It has made this move once before**: turning names into data took the verb table from
  sixty-nine to thirty-four, and added power ([NEXT.md](../NEXT.md) §3.8).

**Why `>shape round 8` would not help.** Structure pays when words combine — several verbs
sharing several objects, so knowing `delete word` and `move line` predicts `delete line`.
Every picture line has the same single "verb", *draw*. A head word `shape` would sit in
front of every one of them and distinguish nothing. It would be the general word Black &
Moran found worst, and one more rule to hold (Payne & Green).

**The keystroke-level model makes the trade countable**, for a practised typist at 0.20 s a
key (Card, Moran & Newell 1980):

| line | letters | decisions | time |
|---|---|---|---|
| `>disc 8` | 7 | 1 | 1.35 + 1.40 = **2.75 s** |
| `>shape round 8` | 14 | 1, if `shape round` becomes one habit | 1.35 + 2.80 = **4.15 s** |
| `>shape round 8` | 14 | 2, if it stays two choices | 2.70 + 2.80 = **5.50 s** |

The model covers skilled, error-free work only. It says nothing about learning, where a
parameterised form's regularity could help a newcomer guess.

**Where a parameter is the better design**: when several operations are really one
operation with a varying value, and would take their arguments the same way (Landauer et
al.; Barnard et al.). Directions were that case.

**Where structure would pay next**: if the picture words grow well past sixteen. Scapin found
structure mattered more at 31 functions than at 13. A family of words sharing a parameter
and a place would then beat a longer flat list.

**What Tab completion changes** ([completion.md](completion.md)): recalling a name becomes
recognising it. That lowers the cost of a larger vocabulary for a newcomer, and costs an
expert nothing.

**The test** ([next.md](next.md) §2): offer half the testers `>disc 8` and half a
parameterised form, and count errors and time. The literature predicts the named form wins
for a performer; only the deck's own users can say whether the regular form helps a
newcomer enough to matter.

## 2. The studies

| study | what was tested | what was found | read |
|---|---|---|---|
| **Barnard et al. 1981** | 6 commands, each a name plus two numbers, one recurring in every command; 48 people at a terminal | the recurring argument in a fixed position, first, was learned most readily: no reversed-argument errors in 8 people with it, against 26 of 32 without | full text |
| **Black & Moran 1982** | 84 people, 8 editing operations, 7 kinds of name | specific, discriminating names best on every measure; general words worst, below nonsense names | full text |
| **Carroll 1982** | a 16-command robot language: congruent and hierarchical names against unstructured ones | names whose form mirrors how the operations relate were learned better | citation; results as reported by Payne & Green and Sluÿters et al. |
| **Scapin 1982** | 60 office clerks, 13 or 31 functions: verb + object labels against a specific verb per function, recalled days later | structure helped recall "to a great extent" (p < .001), more than letting people name the commands themselves; structure mattered more at 31 functions than at 13 | full text |
| **Landauer, Galotti & Hartwell 1983** | 96 people learning a cut-down `ed` for two hours | *which* words made no reliable difference; separate names for operations written differently were much faster to learn (1657 s against 2039 s) | full text |
| **Streeter, Ackroff & Taylor 1983; Ehrenreich 1985** | how to abbreviate command names | abbreviations made by one rule beat intuitive ones; truncation was as good as or better than every alternative for typing; decoding favoured other rules | full text |
| **Grudin & Barnard 1984** | 30 people over three days, five kinds of name | specific names clearly best on nearly every measure | abstract |
| **Payne & Green 1986** — task-action grammar | a notation counting the rule schemas a command language needs | fewer schemas predicts easier learning — if they follow the user's own grouping of tasks | full text |
| **Furnas et al. 1987** | spontaneous names from hundreds of people in five domains | two people chose the same word with probability 0.07–0.18; remedy: accept many words, as an index into help | full text |
| **Walker & Olson 1988** | 42 students, 25 keybindings: rule-built against Emacs's own | rule-built recalled over twice as well; no reversals of the rule's order in 186 errors | full text |
| **Bowden, Douglas & Stanford 1989** | an orthogonal command language against organised and "antiorganised" ones | orthogonal best on all measures; users needed less time to think of commands | abstract |
| **Green & Petre 1996** — cognitive dimensions | a vocabulary for judging notations | the dimensions trade against each other; the best terseness is unknown | authors' manuscript |
| **Stylos & Clarke 2007; Ellis, Stylos & Myers 2007; Stylos & Myers 2008** | programmers with unfamiliar APIs | direct named creators beat a generic factory; required parameters hurt exploration — an analogy only | full text |
| **Stefik & Siebert 2013** | novices and programming-language keywords | plain English words used correctly far more than jargon: Quorum's `repeat` 58–85 % against Java's `for` 18 % | full text |
| **Blackwell & Collins 2005; Magnusson 2010; McLean & Wiggins 2010** | live coding | terseness and constraint for time pressure and mental load — design arguments, no measurement | full text |

**Keystrokes and choices.** The keystroke-level model gives a key 0.08 s (the best typist)
to 0.28 s (an average one) and a mental operator 1.35 s (Card, Moran & Newell 1980). Choice
time grows with the information in the choice — log(n + 1) in Hick's fit, linear in bits in
Hyman's. So a vocabulary where a few words do most of the work costs less than its size
suggests (Hick 1952; Hyman 1953). These are laboratory tasks, not command languages: they
give the shape of a cost curve, not a verdict.

## 3. Type and legibility — the sources

What [pictures-and-type.md](pictures-and-type.md) §6 rests on:

- **Stroke weight.** Screen strokes of 1/12 to 1/6 of character height, heavier on
  dark-on-light displays (FAA HF-STD-001B, 2016). Boldness helps small letters up to a
  point; the thinnest and the most extreme weights read worse (Beier & Oderkerk 2019;
  Bernard et al. 2013). The 12 × 24 face's 2-pixel stem on a 16-pixel cap is 1/8 — the
  heaviest whole pixel inside that band. Playdate's guide, for the nearest panel, asks for
  strokes of at least 2 pixels.
- **Size.** The 12 × 24 cap is 3.39 mm, about 23 arcminutes at 50 cm — the FAA's preferred
  22–24. The 6 × 12 cap is about 10 — its floor for non-critical text. The author of
  Terminus, which ships exactly these two sizes, advises against his own 6 × 12.
- **The zero.** A round slashed zero was misread as O on a dot-matrix display; a narrow
  plain zero halved the errors (Vartabedian 1969). Readers agreed most on a zero narrower
  than O (Wendt 1969). The FAA's labelling rules say the same.
- **Confusion sets.** 0/O/D/Q, 1/l/I/|, 5/S, 8/B, 2/Z, 6/8/9, 7/1 and rn/m, from the FAA,
  Unicode's confusables, Airbus's B612 confusion matrices and Orth et al. (1976). The worst
  pairs depend on the design (Maddox et al. 1977), so proof the whole set on the panel.
- **Letterforms.** Wider i, j, l and t helped; a one-storey a hurt (Beier & Larson 2010).
- **One-bit dither.** Panic warns that a pattern moved one pixel flickers. Apply the
  pattern after moving, or move in steps of two pixels ("Designing for Playdate").
- **Precedents.**
  - Crouwel's New Alphabet (1967): a typeface for cathode-ray displays on a 5 × 9 grid,
    with 45° corners — not, as often said, verticals and horizontals only.
  - Licko's bitmap faces for Emigre (1985) and her line "You read best what you read
    most" (*Emigre* 15, 1990).
  - Kare's Macintosh faces, 1983–84.
  - Gerstner's *Designing Programmes* (1964) and Müller-Brockmann's *Grid Systems in
    Graphic Design* (1981).
  - Maeda's *Design By Numbers* (1999), from which Processing grew in 2001.
  - Weingart's work, "sometimes described as" Swiss Punk (Letterform Archive).
  - The character grid as the picture grid: teletext's 2 × 3 mosaics (ETSI EN 300 706),
    Unicode's sextants (U+1FB00–1FB3B, 13.0) and octants (U+1CD00–1CDE5, 16.0).
  - Airbus's B612 (Vinot & Athènes 2012) and Atkinson Hyperlegible Mono (2024–25).

## 4. The roundtable's sources

The claims in [pictures-and-type.md](pictures-and-type.md) §7, each checked against the
system's own documentation or its designers' papers. Four of the brief's claims needed
correcting, and the corrections are kept:

- **Pure Data's** ICMC paper is 1997, not 1996. "Zero logical time" is not Pd's phrase:
  the manual says each message cascade completes at one logical time.
- **"Strongly timed"** is not in ChucK's 2003 paper; it is defined in the 2015 article.
- **"Time-safe"** is Sonic Pi's formal soundness result, not a kind of sleep, and
  `live_loop` comes from its tutorial, not the paper.
- **monome's** own words are that the grid "by default does nothing". "Blank slate", "no
  labels" and "varibright" are not theirs.

Bret Victor's talk is verified. The wording of its principle could not be checked against
the video, so the page paraphrases it.

- **Max.** Puckette, M. (2002). Max at seventeen. *Computer Music Journal*, 26(4), 31–43.
  https://doi.org/10.1162/014892602320991356 — the scheduler, logical time, and the patch
  as the whole document; presentation mode from the Cycling '74 documentation.
- **Pure Data.** Puckette, M. (1997). Pure Data. *Proceedings of ICMC 1997*, 224–227; the
  *Pd Manual*, ch. 2, "Theory of operation" (§2.6–2.8) for scheduling and `$1`/`$0`.
- **SuperCollider.** McCartney, J. (2002). Rethinking the computer music language:
  SuperCollider. *Computer Music Journal*, 26(4), 61–68.
  https://doi.org/10.1162/014892602320991383 — the client–server split and bundles sent
  ahead, from the SuperCollider 3.14 help ("Server Architecture", "Scheduling and Server
  timing"); the article itself could not be read.
- **ChucK.** Wang, G., Cook, P. R., & Salazar, S. (2015). ChucK: A strongly timed computer
  music language. *Computer Music Journal*, 39(4), 10–29. https://doi.org/10.1162/comj_a_00324
  — and Wang & Cook (2003), *Proceedings of ICMC 2003*, 217–225.
- **Sonic Pi.** Aaron, S., Orchard, D., & Blackwell, A. F. (2014). Temporal semantics for a
  live coding language. *FARM '14*, 37–47. https://doi.org/10.1145/2633638.2633648 — and the
  Sonic Pi tutorial, §9.2, "Live Loops".
- **Impromptu / Extempore.** Sorensen, A., & Gardner, H. (2010). Programming with time:
  Cyber-physical programming with Impromptu. *Proceedings of OOPSLA '10* (Onward!),
  822–834. https://doi.org/10.1145/1869459.1869526
- **ixi lang.** Magnusson, T. (2011). ixi lang: A SuperCollider parasite for live coding.
  *Proceedings of ICMC 2011*, 503–506. http://hdl.handle.net/2027/spo.bbp2372.2011.101
- **TidalCycles.** The mini-notation reference, tidalcycles.org/docs/reference/mini_notation.
- **Elektron.** *Digitakt User Manual*, OS 1.53, §10.9.1 (parameter locks) and §10.9.3 (trig
  conditions); *Digitakt II User Manual*, OS 1.16. The models differ: on the Digitakt II
  probability is its own per-step parameter.
- **Ableton Live.** *Live 12 Reference Manual*, ch. 16, §16.4 (launch quantization) and §16.7
  (Follow Actions).
- **monome.** monome.org/docs/grid ("The monome grid by default does nothing") and the norns
  reference and clock pages.
- **Bret Victor.** Inventing on principle, CUSEC 2012, Montreal, 20 January 2012,
  https://vimeo.com/906418692; and *Learnable Programming* (2012),
  https://worrydream.com/LearnableProgramming/, where he says immediate feedback is a
  prerequisite, not the point.

## 5. Completion — the sources

What [completion.md](completion.md) §7 rests on:

| source | what it shows | read |
|---|---|---|
| Murphy, Kersten & Findlater 2006 | logs of 41 Java developers: content assist used by all 41, 6.7 % of all commands, fifth after delete, save, next word and paste. A usage log, not a speed study | full text |
| Koester & Levine 1994, 1996 | word prediction for 14 typists over seven sessions: fewer selections, but slower keypresses; the attention cost largely outweighed the keystrokes saved | abstracts |
| Quinn & Zhai 2016 | suggestions shown more assertively reduced keyboard actions and were preferred, but made entry slower on average | abstract |
| Palin et al. 2019 | 37,370 phone typists: prediction use correlated with *slower* typing (r = −0.18); correlational | full text |
| Mărășoiu, Church & Blackwell 2015 | six professional developers, 192 completion interactions: 40 % of opened lists ended in a choice; filter-then-accept the commonest pattern; failures included the list swallowing keys | full text |
| Cockburn, Gutwin, Scarr & Malacria 2014 | a survey of novice-to-expert transitions; the novice's action should rehearse the expert's | preprint |
| Kurtenbach & Buxton 1994 | marking menus over 18 hours of real use: marks 3.5 × faster than the menu; users never graduate when item positions change. The rehearsal principle itself is in Kurtenbach's 1993 thesis | full text |
| Lane, Napier, Peres & Sándor 2005 | 251 experienced Word users: keyboard shortcuts preferred by only 1.6–6.4 %, although they were printed beside every menu item and timed fastest | full text (archived) |
| Mitchell & Shneiderman 1989 | 63 people: menus reordered by frequency were slower at first; 81 % preferred fixed menus; positions were memorised almost at once | full text |
| Sears & Shneiderman 1994 | split menus — a few frequent items above a fixed list, rebuilt rarely — cut selection times 17–58 % | full text |
| Findlater & McGrenere 2004 | a fixed split menu beat an adaptive one in every order; people could build their own as well | full text |
| Cockburn, Gutwin & Greenberg 2007 | a model of menu time; order by recent use was the slowest design tested, because nothing stays put | full text |

**The precedents**, from their own documentation and source:

- **GNU Readline**: Tab inserts the shared letters, a second Tab lists; `menu-complete`
  steps through and restores the typed text after the last.
- **zsh**: `AUTO_MENU` starts cycling on the second Tab. In menu selection, Return accepts
  without running the line, Ctrl-G restores, and any other key accepts and acts.
- **fish**: Enter in the completion pager only accepts; Escape reverts; typing accepts and
  continues.
- **Vim**: any ordinary key keeps the match and is typed, but Enter depends on how the
  match was chosen.
- **Emacs** `dabbrev-expand`: repeated presses step through, and the prefix returns when
  none remain.
- **Live-coding editors**:
  - Sonic Pi completes function names as you type; Tab or Return accepts.
  - Strudel has completion, off by default; Enter accepts.
  - Hydra's editor has none.
  - Tidal's Pulsar plugin completes by default.
  - Max completes object names in a new box; its "polite" mode stops after one
    Backspace.

## 6. Citations

**The vocabulary question**

- Barnard, P. J., Hammond, N. V., Morton, J., Long, J. B., & Clark, I. A. (1981). Consistency
  and compatibility in human-computer dialogue. *International Journal of Man-Machine
  Studies*, 15(1), 87–134. https://doi.org/10.1016/S0020-7373(81)80024-7
- Black, J. B., & Moran, T. P. (1982). Learning and remembering command names. *Proceedings
  of CHI '82*, 8–11. https://doi.org/10.1145/800049.801745
- Blackwell, A., & Collins, N. (2005). The programming language as a musical instrument.
  *Proceedings of PPIG 2005*, 120–130. https://ppig.org/papers/2005-ppig-17th-blackwell/
- Bowden, E. M., Douglas, S. A., & Stanford, C. A. (1989). Testing the principle of
  orthogonality in language design. *Human-Computer Interaction*, 4(2), 95–120.
  https://doi.org/10.1207/s15327051hci0402_1
- Card, S. K., Moran, T. P., & Newell, A. (1980). The keystroke-level model for user
  performance time with interactive systems. *Communications of the ACM*, 23(7), 396–410.
  https://doi.org/10.1145/358886.358895
- Carroll, J. M. (1982). Learning, using and designing filenames and command paradigms.
  *Behaviour & Information Technology*, 1(4), 327–346.
  https://doi.org/10.1080/01449298208914457 — the experiments at length in Carroll, J. M.
  (1982), Learning, using and designing command paradigms, *Human Learning*, 1, 31–62.
- Ehrenreich, S. L. (1985). Computer abbreviations: Evidence and synthesis. *Human Factors*,
  27(2), 143–155. https://doi.org/10.1177/001872088502700202
- Ellis, B., Stylos, J., & Myers, B. (2007). The factory pattern in API design: A usability
  evaluation. *ICSE '07*, 302–312. https://doi.org/10.1109/ICSE.2007.85
- Furnas, G. W., Landauer, T. K., Gomez, L. M., & Dumais, S. T. (1987). The vocabulary
  problem in human-system communication. *Communications of the ACM*, 30(11), 964–971.
  https://doi.org/10.1145/32206.32212
- Green, T. R. G., & Petre, M. (1996). Usability analysis of visual programming
  environments: A "cognitive dimensions" framework. *Journal of Visual Languages and
  Computing*, 7(2), 131–174. https://doi.org/10.1006/jvlc.1996.0009
- Grudin, J., & Barnard, P. (1984). The cognitive demands of learning and representing
  command names for text editing. *Human Factors*, 26(4), 407–422.
  https://doi.org/10.1177/001872088402600404
- Hick, W. E. (1952). On the rate of gain of information. *Quarterly Journal of
  Experimental Psychology*, 4(1), 11–26. https://doi.org/10.1080/17470215208416600
- Hyman, R. (1953). Stimulus information as a determinant of reaction time. *Journal of
  Experimental Psychology*, 45(3), 188–196. https://doi.org/10.1037/h0056940
- Landauer, T. K., Galotti, K. M., & Hartwell, S. (1983). Natural command names and initial
  learning: A study of text-editing terms. *Communications of the ACM*, 26(7), 495–503.
  https://doi.org/10.1145/358150.358157
- Magnusson, T. (2010). Designing constraints: Composing and performing with digital musical
  systems. *Computer Music Journal*, 34(4), 62–73. https://doi.org/10.1162/COMJ_a_00026
- McLean, A., & Wiggins, G. (2010). Tidal — pattern language for the live coding of music.
  *Proceedings of SMC 2010*, 331–334. https://doi.org/10.5281/zenodo.849841
- Payne, S. J., & Green, T. R. G. (1986). Task-action grammars: A model of the mental
  representation of task languages. *Human-Computer Interaction*, 2(2), 93–133.
  https://doi.org/10.1207/s15327051hci0202_1
- Scapin, D. L. (1982). Computer commands labelled by users versus imposed commands and the
  effect of structuring rules on recall. *Proceedings of CHI '82*, 17–19.
  https://doi.org/10.1145/800049.801747
- Stefik, A., & Siebert, S. (2013). An empirical investigation into programming language
  syntax. *ACM Transactions on Computing Education*, 13(4), Article 19, 1–40.
  https://doi.org/10.1145/2534973
- Streeter, L. A., Ackroff, J. M., & Taylor, G. A. (1983). On abbreviating command names.
  *The Bell System Technical Journal*, 62(6), 1807–1826.
  https://doi.org/10.1002/j.1538-7305.1983.tb03514.x
- Stylos, J., & Clarke, S. (2007). Usability implications of requiring parameters in
  objects' constructors. *ICSE '07*, 529–539. https://doi.org/10.1109/ICSE.2007.92
- Stylos, J., & Myers, B. A. (2008). The implications of method placement on API
  learnability. *FSE '08*, 105–112. https://doi.org/10.1145/1453101.1453117
- Walker, N., & Olson, J. R. (1988). Designing keybindings to be easy to learn and resistant
  to forgetting even when the set of commands is large. *Proceedings of CHI '88*, 201–206.
  https://doi.org/10.1145/57167.57201

**Completion**

- Cockburn, A., Gutwin, C., & Greenberg, S. (2007). A predictive model of menu performance.
  *Proceedings of CHI 2007*, 627–636. https://doi.org/10.1145/1240624.1240723
- Cockburn, A., Gutwin, C., Scarr, J., & Malacria, S. (2014). Supporting novice to expert
  transitions in user interfaces. *ACM Computing Surveys*, 47(2), 1–36.
  https://doi.org/10.1145/2659796
- Findlater, L., & McGrenere, J. (2004). A comparison of static, adaptive, and adaptable
  menus. *Proceedings of CHI 2004*, 89–96. https://doi.org/10.1145/985692.985704
- Koester, H. H., & Levine, S. P. (1996). Effect of a word prediction feature on user
  performance. *Augmentative and Alternative Communication*, 12(3), 155–168.
  https://doi.org/10.1080/07434619612331277608 — and (1994) *IEEE Transactions on
  Rehabilitation Engineering*, 2(3), 177–187. https://doi.org/10.1109/86.331567
- Kurtenbach, G., & Buxton, W. (1994). User learning and performance with marking menus.
  *Proceedings of CHI '94*, 258–264. https://doi.org/10.1145/191666.191759 — the rehearsal
  principle: Kurtenbach, G. P. (1993), *The Design and Evaluation of Marking Menus*, PhD
  thesis, University of Toronto, §1.3.
- Lane, D. M., Napier, H. A., Peres, S. C., & Sándor, A. (2005). Hidden costs of graphical
  user interfaces. *International Journal of Human-Computer Interaction*, 18(2), 133–144.
  https://doi.org/10.1207/s15327590ijhc1802_1
- Mărășoiu, M., Church, L., & Blackwell, A. F. (2015). An empirical investigation of code
  completion usage by professional software developers. *Proceedings of PPIG 2015*.
  https://www.ppig.org/files/2015-PPIG-26th-Marasoiu.pdf
- Mitchell, J., & Shneiderman, B. (1989). Dynamic versus static menus: An exploratory
  comparison. *ACM SIGCHI Bulletin*, 20(4), 33–37. https://doi.org/10.1145/67243.67247
- Murphy, G. C., Kersten, M., & Findlater, L. (2006). How are Java software developers using
  the Eclipse IDE? *IEEE Software*, 23(4), 76–83. https://doi.org/10.1109/MS.2006.105
- Palin, K., Feit, A. M., Kim, S., Kristensson, P. O., & Oulasvirta, A. (2019). How do people
  type on mobile devices? *Proceedings of MobileHCI '19*, 1–12.
  https://doi.org/10.1145/3338286.3340120
- Quinn, P., & Zhai, S. (2016). A cost-benefit study of text entry suggestion interaction.
  *Proceedings of CHI '16*, 83–88. https://doi.org/10.1145/2858036.2858305
- Sears, A., & Shneiderman, B. (1994). Split menus: Effectively using selection frequency to
  organize menus. *ACM Transactions on Computer-Human Interaction*, 1(1), 27–51.
  https://doi.org/10.1145/174630.174632
- Precedents: the GNU Readline manual (8.3) and `complete.c`; the zsh manual (5.9), options
  and the complist module; the fish-shell documentation, "Interactive use"; Vim 9.2
  `:help ins-completion`; the GNU Emacs manual, "Dynamic Abbrev Expansion"; the Sonic Pi,
  Strudel, Hydra and tidalcycles/pulsar-tidalcycles source; the Cycling '74 *Max User
  Guide*, "Objects".

**Type and legibility**

- Beier, S. (2012). *Reading Letters: Designing for Legibility*. Amsterdam: BIS.
- Beier, S., & Larson, K. (2010). Design improvements for frequently misrecognized letters.
  *Information Design Journal*, 18(2), 118–137. https://doi.org/10.1075/idj.18.2.03bei
- Beier, S., & Oderkerk, C. A. T. (2019). Smaller visual angles show greater benefit of
  letter boldness than larger visual angles. *Acta Psychologica*, 199, 102904.
  https://doi.org/10.1016/j.actpsy.2019.102904
- Bernard, J.-B., Kumar, G., Junge, J., & Chung, S. T. L. (2013). The effect of
  letter-stroke boldness on reading speed in central and peripheral vision. *Vision
  Research*, 84, 33–42. https://doi.org/10.1016/j.visres.2013.03.005
- Federal Aviation Administration (2016). *Human Factors Design Standard*, HF-STD-001B,
  §5.3.1.7 and §5.4.1.2.3.16.
- Maddox, M. E., Burnette, J. T., & Gutmann, J. C. (1977). Font comparisons for 5 × 7 dot
  matrix characters. *Human Factors*, 19(1), 89–93. https://doi.org/10.1177/001872087701900111
- Orth, B., Weckerle, H., & Wendt, D. (1976). Legibility of numerals displayed in a 4 × 7 dot
  matrix and seven-segment digits. *Visible Language*, 10(2), 145–155.
- Panic. *Designing for Playdate* (the SDK design guide), and the Playdate specifications.
- Vartabedian, A. G. (1969). A proposed fontstyle for the graphic representation of the oh
  and zero. *Journal of Typographic Research*, 3(3), 249–258.
- Vinot, J.-L., & Athènes, S. (2012). Legible, are you sure? An experimentation-based
  typographical design in safety-critical context. *Proceedings of CHI '12*, 2287–2296.
  https://doi.org/10.1145/2207676.2208387
- Wendt, D. (1969). O or 0? *Journal of Typographic Research*, 3(3), 241–248.
- Precedents: Crouwel, W. (1967), *New Alphabet*, Kwadraatblad (Steendrukkerij De Jong); The
  Foundry's digitisation; Licko, Z., interview, *Emigre* 15 (1990); Kare's own account at
  folklore.org and the Computer History Museum; Gerstner, K. (1964), *Designing Programmes*,
  Niggli; Müller-Brockmann, J. (1981), *Grid Systems in Graphic Design*, Niggli; Maeda, J.
  (1999), *Design By Numbers*, MIT Press; Letterform Archive (2026), "Legacies of Swiss
  Style, Part 2 — Wolfgang Weingart"; Zhekov, D., Terminus font, README and source; ETSI EN
  300 706 (teletext); The Unicode Standard 13.0 and 16.0; the B612 and Atkinson Hyperlegible
  Mono releases.
