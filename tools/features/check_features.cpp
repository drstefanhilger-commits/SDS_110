/*
 * check_features – Prüfung des Merkmalswerkzeugs (make -C tools/features check)
 *
 * 1. Gleichheit mit der Board-Kette: Simulator (DroneStatic) -> 114 -> 118 -> Frame_Assembler -> 122
 *    im Prozess berechnen; parallel die Rohwerte des Referenzkanals als 24-bit-WAV schreiben.
 *    sds_features auf diese WAV muss bitgleiche Merkmale und Zeitstempel liefern.
 * 2. Plausibilität: 1-kHz-Sinus -> Maximum von band_log_power in Band 14 ((1000 − 80) / 62,5 = 14,7).
 * 3. Reproduzierbarkeit: zweiter Lauf auf dieselbe WAV ergibt eine identische .npy.
 * Aufruf: check_features <pfad-zu-sds_features> <arbeitsordner>
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>
#include "Harness/Signal_Simulator.hpp"
#include "Sensor_Unit_112/Pre_Processor_118.hpp"
#include "Sensor_Unit_112/Frame_Assembler.hpp"
#include "Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.hpp"

using namespace sds110;
static Pre_Processor_118 pre; static Frame_Assembler fa; static Feature_Extraction_Module_122 feat; static Spectrum sp;

static void writeWav24(const std::string& path, const std::vector<int32_t>& s)
{
    FILE* f = std::fopen(path.c_str(), "wb");
    const uint32_t n = s.size(), data = n * 3, sr = SAMPLE_RATE_HZ, br = sr * 3;
    auto w32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); }; auto w16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f); w32(36 + data); std::fwrite("WAVEfmt ", 1, 8, f); w32(16); w16(1); w16(1); w32(sr); w32(br); w16(3); w16(24);
    std::fwrite("data", 1, 4, f); w32(data);
    for (int32_t v : s) { const uint8_t b[3] = { uint8_t(v), uint8_t(v >> 8), uint8_t(v >> 16) }; std::fwrite(b, 1, 3, f); }
    std::fclose(f);
}

static std::vector<float> readNpy(const std::string& path, size_t& rows, size_t& cols)
{
    FILE* f = std::fopen(path.c_str(), "rb"); std::vector<float> d; rows = cols = 0;
    if (!f) return d;
    unsigned char pre[10];
    if (std::fread(pre, 1, 10, f) != 10) { std::fclose(f); return d; }
    const size_t hl = pre[8] | (pre[9] << 8);
    std::string h(hl, ' ');
    if (std::fread(&h[0], 1, hl, f) != hl) { std::fclose(f); return d; }
    std::sscanf(h.c_str() + h.find("(") + 1, "%zu, %zu", &rows, &cols);
    d.resize(rows * cols);
    if (std::fread(d.data(), sizeof(float), d.size(), f) != d.size()) d.clear();
    std::fclose(f);
    return d;
}

static int run(const std::string& tool, const std::string& wav, const std::string& out)
{
    return std::system((tool + " " + wav + " " + out + " > /dev/null").c_str());
}

int main(int argc, char** argv)
{
    if (argc != 3) { std::fprintf(stderr, "Aufruf: check_features <sds_features> <arbeitsordner>\n"); return 1; }
    const std::string tool = argv[1], dir = argv[2];
    int fail = 0;

    // --- 1. Gleichheit mit der Board-Kette
    auto& sim = Signal_Simulator::instance(); auto& arr = Microphone_Array_114::instance();
    SimParams p; p.scenario = SimScenario::DroneStatic; p.snr_db = 10; sim.init(p);
    pre.init(); fa.reset(); feat.init();
    std::vector<int32_t> ref; std::vector<float> expect; FeatureVector fv{};
    const int hops = 200;                                            // 6,4 s
    for (int k = 0; k < hops; ++k) {
        sim.generateHop(static_cast<uint64_t>(k) * HOP_SAMPLES * 1000000ULL / SAMPLE_RATE_HZ);
        MicFrame* h = arr.acquireReadable();
        for (uint32_t i = 0; i < HOP_SAMPLES; ++i)                   // Rohwert des Referenzkanals (exakt 24 Bit)
            ref.push_back(static_cast<int32_t>(std::lround(h->data[REF_MIC][i] * 8388608.0)));
        pre.process(*h); const bool full = fa.push(*h); arr.release(h);
        if (!full) continue;
        feat.process(fa.frame(), sp, fv);
        expect.push_back(static_cast<float>(static_cast<double>(fa.frame().time_utc_us) * 1e-6));
        expect.insert(expect.end(), fv.band_log_power, fv.band_log_power + NUM_BANDS);
        expect.insert(expect.end(), fv.mel, fv.mel + MEL_BANDS);
        expect.push_back(fv.spectral_flux);
        expect.insert(expect.end(), fv.band_am_depth, fv.band_am_depth + NUM_BANDS);
    }
    writeWav24(dir + "/sim_ref.wav", ref);
    size_t r = 0, c = 0;
    const int rc = run(tool, dir + "/sim_ref.wav", dir + "/sim_ref");
    std::vector<float> got = readNpy(dir + "/sim_ref.npy", r, c);
    const size_t nExp = expect.size() / (1 + 2 * NUM_BANDS + MEL_BANDS + 1);
    size_t diff = 0; float maxd = 0;
    if (got.size() == expect.size())
        for (size_t i = 0; i < got.size(); ++i) if (got[i] != expect[i]) { ++diff; maxd = std::fmax(maxd, std::fabs(got[i] - expect[i])); }
    bool ok = rc == 0 && r == nExp && got.size() == expect.size() && diff == 0;
    std::printf("Board-Kette vs. Werkzeug: %zu Frames x %zu Spalten, abweichende Werte %zu (max %.3g)  %s\n", r, c, diff, maxd, ok ? "OK" : "FEHLER");
    fail += !ok;

    // --- 2. 1-kHz-Sinus
    std::vector<int32_t> sine(SAMPLE_RATE_HZ * 3);
    for (size_t i = 0; i < sine.size(); ++i) sine[i] = static_cast<int32_t>(std::lround(0.1 * 8388607.0 * std::sin(2 * M_PI * 1000.0 * i / SAMPLE_RATE_HZ)));
    writeWav24(dir + "/sine1k.wav", sine);
    run(tool, dir + "/sine1k.wav", dir + "/sine1k");
    std::vector<float> s = readNpy(dir + "/sine1k.npy", r, c);
    int bmax = -1; float vmax = -1e30f;
    if (r > 0) { const float* row = &s[(r - 1) * c + 1]; for (uint32_t b = 0; b < NUM_BANDS; ++b) if (row[b] > vmax) { vmax = row[b]; bmax = b; } }
    ok = bmax == 14;
    std::printf("1-kHz-Sinus: Maximum band_log_power in Band %d (erwartet 14)  %s\n", bmax, ok ? "OK" : "FEHLER");
    fail += !ok;

    // --- 3. Reproduzierbarkeit
    run(tool, dir + "/sim_ref.wav", dir + "/sim_ref2");
    std::vector<float> g2 = readNpy(dir + "/sim_ref2.npy", r, c);
    ok = g2 == got && !got.empty();
    std::printf("Zweiter Lauf identisch: %s\n", ok ? "OK" : "FEHLER");
    fail += !ok;
    return fail;
}
