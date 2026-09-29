# hello — zine #0

*A getting-started zine for the deck, and the same zine as a document on it.*

| file | what it is |
|---|---|
| `hello-booklet.pdf` | to print: US Letter, landscape, two pages a side — both sides, fold, staple |
| `hello-pages.pdf` | the sixteen pages in reading order, half letter, for a screen |
| `hello-plate-black.pdf`, `hello-plate-red.pdf` | the booklet's two plates apart, each in black: one riso drum apiece |
| `hello.txt` | the zine as a deck document, `hello`: thirty columns, and the headings are lines you can run |

One idea to a page, in one narrow column — thirty characters, the deck's own
line — on a page that is mostly paper. Everything is set in the deck's own two faces — the 12×24 it reads in and the 6×12
it fits sixty columns with — and its own tiles (codes 128–155: nine tones,
sparkles, blocks, arcs). **Every picture was drawn by the deck's own picture
engine**: `tools/zine_art.c` links `firmware/components/viz/viz.c`, marks
primitives the way the clock does when a lane fires, and saves the cells. The
orbit on page 13 is `orbitals` as it is written.

**Two inks, one bit each** (2026-09-29). Black, and a red that means one thing:
what the deck lights — the prompt, the deck's replies, the step you are on, the
page's number, a picture on its own plate. Each ink is one bit deep, like the
panel, and each is its own plate: the PDFs are two stencils a page, red then
black over it, so they stay exact and small, and the plates come apart for a riso
print shop. The cover is ORBITALS' sun, `>disc`, as the engine draws it, twice its
size and half off the corner; every page's number stands at the foot of its outer
edge in the deck's 12×24 face at eighteen times, under the black where they cross.
A grid after Weingart, the zine says on its back, sometimes called Swiss punk.

Each page is titled with a line you could type — `>what`, `>grammar`, `>room` —
and under it is what the deck says to that line, quoted from
`firmware/components/cmd/cmd.c`: `what? try: help`. The back cover is `>help`, the
one title it knows. The deck's refusals on pages 4 and 5 are quoted from the
firmware too (`seq_pattern.h`, `lane_name.h`, `builtins.c`), not paraphrased.
Page numbers are steps: sixteen pages are one bar.

## Print it

Sixteen half-letter pages (5.5 × 8.5 in), one bar: the folio at the foot of
each page is its step, so page five is `....x...........`. Print
`hello-booklet.pdf` on both sides of four US Letter sheets at 100 %, flipping
on the **short** edge. Stack the sheets in order, fold them in half together,
and staple twice along the fold. The sides pair pages the usual saddle-stitch
way — 16|1, 2|15, 14|3, 4|13, 12|5, 6|11, 10|7, 8|9 — so the fold reads
straight through.

## Build it

```sh
cc -std=c11 -O1 -I firmware/components/viz/include \
   -I firmware/components/seq/include -I tools/hostshim \
   -o /tmp/zine_art tools/zine_art.c firmware/components/viz/viz.c
mkdir -p /tmp/zine_art_cells && /tmp/zine_art /tmp/zine_art_cells
python3 tools/zine.py /tmp/zine_art_cells zine      # needs Pillow
```

`tools/zine_pages.py` is the layout — as code, since the instrument is text —
and `tools/zine_refs.py` the short-form references it numbers.

## What the numbers rest on

Every figure in the zine is measured on the deck or read from its source:

| claim | where |
|---|---|
| USB MIDI jitter 0.03 ms at the host | [README.md](../README.md), *The instrument* |
| 51 of 51 corpus patterns play what Strudel 1.2.6 plays | [docs/STRUDEL.md](../docs/STRUDEL.md), *Measured*; `tools/test_corpus.c` in CI |
| 96 ticks a beat, 16 lanes, 37 verbs | `seq.h`, `SEQ_MAX_LANES`; the verb table in `builtins.c` |
| 16 steps against 12 come home every 3 bars | lcm(16, 12) = 48 steps |
| a value survives a lost phone | [docs/MAP.md](../docs/MAP.md) §9.8 |

## References

Verified 2026-09-27 against publisher records (Crossref, DOIs), proceedings,
archives and the authors' own pages; notes say where a claim needed care.

1. Aaron, S., & Blackwell, A. F. (2013). From Sonic Pi to Overtone: Creative musical experiences with domain-specific and functional languages. *Proceedings of the First ACM SIGPLAN Workshop on Functional Art, Music, Modeling & Design (FARM '13)*, Boston, 35–46. https://doi.org/10.1145/2505341.2505346
2. Berthaut, F., Marshall, M. T., Subramanian, S., & Hachet, M. (2013). Rouages: Revealing the mechanisms of digital musical instruments to the audience. *Proceedings of NIME 2013*, Daejeon, 164–169. https://doi.org/10.5281/zenodo.1178478
3. Burland, K., & McLean, A. (2016). Understanding live coding events. *International Journal of Performance Arts and Digital Media*, 12(2), 139–151. https://doi.org/10.1080/14794713.2016.1227596 — a questionnaire study of live-coding audiences and the projected code.
4. Collins, N., McLean, A., Rohrhuber, J., & Ward, A. (2003). Live coding in laptop performance. *Organised Sound*, 8(3), 321–330. https://doi.org/10.1017/S135577180300030X
5. Collins, N., & McLean, A. (2014). Algorave: Live performance of algorithmic electronic dance music. *Proceedings of NIME 2014*, London, 355–358. https://doi.org/10.5281/zenodo.1178734
6. Eno, B. (1996). *Generative Music 1 with SSEYO Koan Software* [floppy disk: twelve Koan pieces and the Koan Plus player]. SSEYO; pieces © Opal Music. https://intermorphic.com/archive/sseyo/koan/generativemusic1/
7. Gibson, W. (1984). *Neuromancer*. New York: Ace Books. The hacker's "cyberspace deck" is in chapter 1.
8. Jack, O. (2017). *Hydra* [live-coding video synthesiser, software]. https://github.com/hydra-synth/hydra — and Jack, O. (2019). Hydra: Live coding networked visuals. *ICLC 2019*, Madrid. https://doi.org/10.5281/zenodo.3946269 — a presentation and demo proposal; there is no full peer-reviewed paper on Hydra.
9. Lovelace, A. A. (1843). Notes by the translator. In L. F. Menabrea, Sketch of the Analytical Engine invented by Charles Babbage, Esq. (A. A. Lovelace, Trans.). In R. Taylor (Ed.), *Scientific Memoirs*, 3, art. XXIX, 666–731 (notes 691–731). London: Richard & John E. Taylor. On p. 694 the engine "might compose elaborate and scientific pieces of music of any degree of complexity or extent".
10. Mathews, M. V. (1963). The digital computer as a musical instrument. *Science*, 142(3592), 553–557. https://doi.org/10.1126/science.142.3592.553
11. McCartney, J. (2002). Rethinking the computer music language: SuperCollider. *Computer Music Journal*, 26(4), 61–68. https://doi.org/10.1162/014892602320991383
12. McLean, A. (2014). Making programming languages to dance to: Live coding with Tidal. *Proceedings of the 2nd ACM SIGPLAN International Workshop on Functional Art, Music, Modeling & Design (FARM '14)*, Gothenburg, 63–70. https://doi.org/10.1145/2633638.2633647
13. Moon, T. (1977). Untitled drawing, "…now form a band". *Sideburns*, 1 (January 1977), 2. Attributed in Savage, J. (2002). *England's Dreaming* (rev. ed.), p. 281; Worley, M. (2015). *History Workshop Journal*, 79, 76–106, https://doi.org/10.1093/hwj/dbu043. The drawing is Sideburns'; the zine takes four words of it and none of the folklore about where else it appeared, which has no reliable source.
14. Obarski, K. (1987). *The Ultimate Soundtracker* [Amiga software]. EAS. Goriunova, O. (2012). *Art Platforms and Cultural Production on the Internet*. Routledge, p. 102 — the origin of the tracker. Not the origin of the step grid: it drew on Chris Hülsbeck's *Soundmonitor* for the C64.
15. Reeves, S., Benford, S., O'Malley, C., & Fraser, M. (2005). Designing the spectator experience. *Proceedings of CHI 2005*, Portland, 741–750. https://doi.org/10.1145/1054972.1055074 — the secretive, expressive, magical and suspenseful strategies.
16. Reich, S. (2002 [1968]). Music as a gradual process. In P. Hillier (Ed.), *Writings on Music, 1965–2000* (pp. 34–36). New York: Oxford University Press. First printed in *Anti-Illusion: Procedures/Materials* (Whitney Museum of American Art, 1969), 56–57.
17. Riley, T. (1964). *In C*. First performed 4 November 1964, San Francisco Tape Music Center. See Carl, R. (2009). *Terry Riley's In C*. Oxford University Press. https://doi.org/10.1093/acprof:oso/9780195325287.001.0001
18. Roos, F., & McLean, A. (2023). Strudel: Live coding patterns on the web. *Proceedings of the 7th International Conference on Live Coding (ICLC 2023)*, Utrecht. https://doi.org/10.5281/zenodo.7842142 — first commit 22 January 2022; now at https://strudel.cc.
19. Schloss, W. A. (2003). Using contemporary technology in live performance: The dilemma of the performer. *Journal of New Music Research*, 32(3), 239–242. https://doi.org/10.1076/jnmr.32.3.239.16866 — cause and effect between gesture and sound matter to an audience; checked against the author's pre-press copy.
20. Spiegel, L. (1986). *Music Mouse – An Intelligent Instrument* [Macintosh software]. Revision history (archived): https://web.archive.org/web/20221218033319/http://retiary.org/ls/progs/mm_revision_history.html
21. TOPLAP (2004). ManifestoDraft ("Lubeck 04"). https://toplap.org/wiki/ManifestoDraft.html — printed in Ward, A., Rohrhuber, J., Olofsson, F., McLean, A., Griffiths, D., Collins, N., & Alexander, A. (2004). Live algorithm programming and a temporary organisation for its promotion. In O. Goriunova & A. Shulgin (Eds.), *read_me: Software Art & Cultures* (pp. 243–261). Aarhus: Digital Aesthetics Research Centre. https://doi.org/10.5281/zenodo.7139247 — "Show us your screens", p. 247.
22. Toussaint, G. (2005). The Euclidean algorithm generates traditional musical rhythms. In R. Sarhangi & R. V. Moody (Eds.), *Renaissance Banff: Mathematics, Music, Art, Culture* (Bridges 2005), 47–56. https://archive.bridgesmathart.org/2005/bridges2005-47.html — Euclidean rhythm is still an open item in this language ([docs/MAP.md](../docs/MAP.md) §9.2).
23. Weingart, W. (2000). *Typography: My Way to Typography / Wege zur Typographie*. Baden: Lars Müller. That his Basel work is sometimes called Swiss Punk: Sauer, Z. (2026, 13 April). Legacies of Swiss Style, Part 2 — Wolfgang Weingart. Letterform Archive. The zine says "sometimes", because no stronger source says more.
24. Xenakis, I. (1971). *Formalized Music: Thought and Mathematics in Composition*. Bloomington: Indiana University Press. Revised edition, Pendragon Press, 1992.
