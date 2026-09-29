/*
 * The view node's own drawing, run on this computer: view/deckview/deckview.ino
 * built against tools/nodesim/PicoDVI.h, fed the deck's frames exactly as the
 * deck packs them (firmware/main/view_wire.h), and the screens it draws written
 * out as pictures - at 2x, as HDMI shows the 320 x 240 buffer. Not a mock-up of
 * the node: the node's code.
 *
 *   node_sim CELLS_DIR PATTERN MODE OUT.ppm [par0..par7]
 *
 * CELLS_DIR holds tools/mock_frames.c's frames, named by PATTERN with the
 * frame's number in it: 'night%02d-53x20.cells'. MODE is plain, scan, riso,
 * poster or code. The parameters, if given, are what controllers 1-8 on channel
 * 16 would be (255 unset). OUT with a %02d in it gets every screen; without,
 * the last.
 *
 *   c++ -std=c++17 -O1 -I tools/nodesim -I view/deckview -o node_sim tools/node_sim.cpp
 */
#include "PicoDVI.h"
uint32_t g_millis = 10000;
NodeSerial Serial;
int adafruit_feather_dvi_cfg;

#include "../view/deckview/deckview.ino"
#include "../firmware/main/view_wire.h"
#include <stdlib.h>
#include <vector>

static void dump(const char *path)
{
    FILE *o = fopen(path, "wb");
    if (!o) return;
    fprintf(o, "P6\n640 480\n255\n");
    const uint8_t *fb = display.front();
    const uint16_t *pal = display.front_pal();
    for (int y = 0; y < 480; y++)
        for (int x = 0; x < 640; x++) {
            const uint16_t c = pal[fb[(y / 2) * 320 + x / 2]];
            const uint8_t px[3] = { (uint8_t)((c >> 8) & 0xF8), (uint8_t)((c >> 3) & 0xFC),
                                    (uint8_t)((c << 3) & 0xF8) };
            fwrite(px, 1, 3, o);
        }
    fclose(o);
}

int main(int argc, char **argv)
{
    if (argc < 5) { fprintf(stderr, "node_sim CELLS PATTERN MODE OUT.ppm [p0..p7]\n"); return 2; }
    const char *names[] = { "plain", "scan", "", "", "riso", "poster", "code" };
    int mode = -1;
    for (int m = 0; m < 7; m++) if (names[m][0] && strcmp(argv[3], names[m]) == 0) mode = m;
    if (mode < 0) { fprintf(stderr, "no mode %s\n", argv[3]); return 2; }
    uint8_t par[VIEW_PARAMS];
    for (int i = 0; i < VIEW_PARAMS; i++) par[i] = (uint8_t)(5 + i < argc ? atoi(argv[5 + i]) : 255);
    const view_line_t lines[] = {
        { "ORBITALS", 0, 0 }, { "II first light", 0, 0 }, { "124 bpm  dmin", 0, 0 },
        { "kick 9...8...9...8...", 7, 8 }, { "box 2...1...2...1...", 6, 7 },
        { "disc:x 8876532111235678", 9, 10 }, { "hat ..3...3...3...4.", 6, 7 },
    };
    const view_line_t code[] = {
        { "orbitals", 0, 0 }, { "124 bpm  dmin", 0, 0 }, { "-- II first light", 0, 0 },
        { ">send view plain", 0, 0 }, { ">kick 5...4...5...4...", 7, 8 },
        { ">box 2...1...2...1...", 6, 7 }, { ">scale dmin", 0, 0 },
        { ">disc 3", 0, 0 }, { ">disc:x 8876532111235678", 9, 10 },
    };
    setup();
    const bool each = strchr(argv[4], '%') != NULL;
    int drawn = 0;
    for (int k = 0; k < 64; k++) {
        char name[256], path[512];
        snprintf(name, sizeof name, argv[2], k);
        snprintf(path, sizeof path, "%s/%s", argv[1], name);
        FILE *f = fopen(path, "rb");
        if (!f) break;
        uint8_t wh[2];
        if (fread(wh, 1, 2, f) != 2) { fclose(f); break; }
        std::vector<uint8_t> cells((size_t)wh[0] * wh[1]);
        if (fread(cells.data(), 1, cells.size(), f) != cells.size()) { fclose(f); break; }
        fclose(f);
        const uint32_t tick = (uint32_t)k * 24;
        uint8_t ctl[VIEW_CTL2_HEAD_LEN + VIEW_TEXT_MAX + 1];
        const size_t cn = mode == 5 ? view_wire_pack_ctl2(ctl, sizeof ctl, tick, mode, par, lines, 7)
                        : mode == 6 ? view_wire_pack_ctl2(ctl, sizeof ctl, tick, mode, par, code, 9)
                        : view_wire_pack_ctl2(ctl, sizeof ctl, tick, mode, par, NULL, 0);
        std::vector<uint8_t> stream(ctl, ctl + cn);
        std::vector<uint8_t> pic(VIEW_HEAD_LEN + cells.size() + 1);
        const size_t pn = view_wire_pack(pic.data(), pic.size(), tick, wh[0], wh[1], cells.data());
        stream.insert(stream.end(), pic.data(), pic.data() + pn);
        Serial.in = stream.data(); Serial.n = (int)stream.size(); Serial.at = 0;
        g_millis += 100000;                   // past the mode's label
        loop();
        drawn++;
        if (each) {
            char out[512];
            snprintf(out, sizeof out, argv[4], k);
            dump(out);
        }
    }
    if (!drawn) { fprintf(stderr, "no frames in %s\n", argv[1]); return 1; }
    if (!each) dump(argv[4]);
    printf("%s: %d frames, %s, %lu refused, last %ux%u\n", argv[4], drawn, argv[3],
           (unsigned long)s_rd.refused, s_rd.w, s_rd.h);
    return s_rd.refused != 0;
}
