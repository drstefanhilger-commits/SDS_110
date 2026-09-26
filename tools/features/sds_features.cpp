/*
 * sds_features – Merkmale je Analyse-Frame aus einer WAV-Datei, berechnet mit dem unveränderten
 * Board-Code von SDS_110 (Arbeitspaket 1 des Trainingskonzepts ML124, ML_Test/docs).
 *
 * Kette wie auf dem Board: 24-bit-PCM linksbündig im 32-bit-Slot (wie ADAU7118/SAI) ->
 * Microphone_Array_114::pushBlock() in DMA-Blöcken -> Hop -> Pre_Processor_118 -> Frame_Assembler
 * (64 ms, 50 %) -> Feature_Extraction_Module_122 (Referenzkanal). Optional (--hbd) zusätzlich der
 * heutige HBD-Zustand und dessen s(t) aus Machine_Learning_Module_124 als Vergleichsgrundlage.
 *
 * Aufruf:  sds_features [--hbd] [--channel N] <eingabe.wav> <ausgabe-präfix>
 *          sds_features [--hbd] --label <drohne.wav> <umwelt.wav> <ausgabe-präfix>
 *
 * Label-Modus (Arbeitspaket 3): Beide Anteile (mono, 48 kHz) werden als 24-bit-Werte addiert
 * (Gemisch = Drohne + Umwelt exakt, übersteuerte Samples gezählt). Die Merkmale stammen aus dem
 * Gemisch. Für die Labels laufen beide Anteile durch dieselbe Kette: eigener 118 nur mit Bandpass
 * (linear), darauf exakt die NS·AGC-Verstärkungsrampe, die 118 im Gemisch berechnet hat, dann
 * eigener Frame_Assembler und eigene 122. Zusätzliche Spalten snr_db_00…63 = 10·log10(P_b(Drohne) /
 * P_b(Umwelt)) je Band nach der Kette (auf ±80 dB begrenzt). Das JSON enthält den größten Fehler
 * |Gemisch − (Drohne + Umwelt)| nach 118 als Selbstkontrolle.
 *   Eingabe: 48 kHz; PCM 16/24/32 Bit oder float32; mono oder mehrkanalig. Bei NUM_MICS (8)
 *            Kanälen werden alle Kanäle eingespeist, sonst Kanal N (Standard 0) als Referenzkanal,
 *            die übrigen Mikrofone mit 0.
 *   Ausgabe: <präfix>.npy  float32 [Frames, Spalten] – Spalte 0 = Zeit des ersten Samples [s]
 *            <präfix>.json Spaltennamen, Merkmalsversion, Parameter der Kette, Eingabedaten
 * Jede Datei in einem eigenen Prozess verarbeiten: Zustände (AGC, Filter, Puffer) beginnen so bei
 * jeder Datei neu wie nach einem Board-Start. Ein unvollständiger letzter DMA-Block/Hop entfällt.
 * Exit-Code: 0 ok, 1 Aufruf, 2 Eingabe, 3 Ausgabe.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <string>
#include <vector>
#include "SDS_110_Config.hpp"
#include "Sensor_Unit_112/Microphone_Array_114.hpp"
#include "Sensor_Unit_112/Pre_Processor_118.hpp"
#include "Sensor_Unit_112/Frame_Assembler.hpp"
#include "Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.hpp"
#include "Processing_Module_120/Machine_Learning_Module_124/Machine_Learning_Module_124.hpp"
#include "wav_io.hpp"
#include "npy_writer.hpp"

#ifndef FEATURE_VERSION
#define FEATURE_VERSION "unbekannt"
#endif
#ifndef SDS110_GIT
#define SDS110_GIT "unbekannt"
#endif
#define TOOL_VERSION "1.1"

using namespace sds110;

static Pre_Processor_118 pre;                   // groß (Filterzustände) -> statisch
static Frame_Assembler fa;
static Feature_Extraction_Module_122 feat;
static Machine_Learning_Module_124 ml;
static Spectrum refSpec;
// Label-Modus: getrennte Kette je Anteil (Drohne d, Umwelt n)
static Pre_Processor_118 preD, preN;
static Frame_Assembler faD, faN;
static Feature_Extraction_Module_122 featD, featN;
static MicFrame hopD, hopN;
static Spectrum specD, specN;

static std::string jsonStr(const std::string& s)
{
    std::string o = "\"";
    for (char c : s) { if (c == '"' || c == '\\') o += '\\'; o += c; }
    return o + "\"";
}

int main(int argc, char** argv)
{
    bool withHbd = false, label = false; int channel = 0; std::vector<std::string> pos;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--hbd")) withHbd = true;
        else if (!std::strcmp(argv[i], "--label")) label = true;
        else if (!std::strcmp(argv[i], "--channel") && i + 1 < argc) channel = std::atoi(argv[++i]);
        else pos.push_back(argv[i]);
    }
    if (pos.size() != (label ? 3u : 2u)) {
        std::fprintf(stderr, "Aufruf: sds_features [--hbd] [--channel N] <eingabe.wav> <ausgabe-präfix>\n"
                             "        sds_features [--hbd] --label <drohne.wav> <umwelt.wav> <ausgabe-präfix>\n");
        return 1;
    }
    const std::string in = pos[0], inN = label ? pos[1] : std::string(), out = pos.back();

    auto load = [&](const std::string& f, WavData& d) {
        d = readWav(f);
        if (!d.error.empty()) { std::fprintf(stderr, "%s: %s\n", f.c_str(), d.error.c_str()); return false; }
        if (d.sampleRate != SAMPLE_RATE_HZ) {
            std::fprintf(stderr, "%s: Abtastrate %u Hz, erwartet %u Hz (Daten zuerst umtasten)\n", f.c_str(), d.sampleRate, SAMPLE_RATE_HZ);
            return false;
        }
        return true;
    };
    WavData w, wD, wN;
    size_t clipped = 0;
    if (!label) {
        if (!load(in, w)) return 2;
    } else {                                                             // Gemisch = Drohne + Umwelt (24 Bit, exakt)
        if (!load(in, wD) || !load(inN, wN)) return 2;
        if (wD.channels != 1 || wN.channels != 1) { std::fprintf(stderr, "Label-Modus: nur Mono-Anteile\n"); return 2; }
        const size_t n = std::min(wD.pcm24[0].size(), wN.pcm24[0].size());
        wD.pcm24[0].resize(n); wN.pcm24[0].resize(n);
        w = wD; w.pcm24[0].assign(n, 0);
        for (size_t i = 0; i < n; ++i) {
            int64_t v = int64_t(wD.pcm24[0][i]) + wN.pcm24[0][i];
            if (v > 8388607 || v < -8388608) { ++clipped; v = v > 0 ? 8388607 : -8388608; }
            w.pcm24[0][i] = int32_t(v);
        }
    }
    const bool allMics = (w.channels == NUM_MICS);
    if (!allMics && (channel < 0 || channel >= w.channels)) { std::fprintf(stderr, "Kanal %d nicht vorhanden\n", channel); return 1; }
    const size_t nSamples = w.pcm24.empty() ? 0 : w.pcm24[0].size();

    auto& arr = Microphone_Array_114::instance();
    pre.init(); fa.reset(); feat.init(); if (withHbd) ml.init();
    if (label) {
        preD.init(); preN.init();
        for (Pre_Processor_118* p : { &preD, &preN }) { p->enableNoiseSuppression(false); p->enableAgc(false); }   // nur Bandpass
        faD.reset(); faN.reset(); featD.init(); featN.init();
        std::memset(&hopD, 0, sizeof(hopD)); std::memset(&hopN, 0, sizeof(hopN));
    }

    const size_t nFeat = NUM_BANDS + MEL_BANDS + 1 + NUM_BANDS;
    const size_t nHbd = withHbd ? 5 + NUM_BANDS : 0;
    const size_t nCols = 1 + nFeat + nHbd + (label ? NUM_BANDS : 0);
    size_t hopIdx = 0; double maxRecErr = 0.0, maxAbsMix = 0.0;
    FeatureVector fvD{}, fvN{};
    std::vector<float> data;
    size_t frames = 0;
    FeatureVector fv{}; AcousticState st{};
    static int32_t block[DMA_BLOCK_SAMPLES * NUM_MICS];

    const size_t nBlocks = nSamples / DMA_BLOCK_SAMPLES;
    for (size_t b = 0; b < nBlocks; ++b) {
        const size_t s0 = b * DMA_BLOCK_SAMPLES;
        std::memset(block, 0, sizeof(block));
        for (uint32_t s = 0; s < DMA_BLOCK_SAMPLES; ++s)
            for (uint32_t m = 0; m < NUM_MICS; ++m) {
                int32_t v = 0;
                if (allMics) v = w.pcm24[m][s0 + s];
                else if (m == REF_MIC) v = w.pcm24[channel][s0 + s];
                block[s * NUM_MICS + m] = static_cast<int32_t>(static_cast<uint32_t>(v) << 8);   // 24 Bit linksbündig
            }
        arr.pushBlock(block, DMA_BLOCK_SAMPLES, static_cast<uint64_t>(s0) * 1000000ULL / SAMPLE_RATE_HZ);

        while (MicFrame* h = arr.acquireReadable()) {                   // wie Sensor_Unit_112::nextFrame()
            pre.process(*h);
            bool fullD = false;
            if (label) {                                                // Anteile: Bandpass + Verstärkungsrampe des Gemischs
                const float g0 = pre.frameCenterGain(REF_MIC), g1 = pre.appliedGain(REF_MIC);
                const float step = (g1 - g0) / static_cast<float>(HOP_SAMPLES);
                constexpr float scale = 1.0f / 8388608.0f;              // wie 114: pcm24 · 2^-23
                for (auto* c : { &hopD, &hopN }) { c->frame_id = h->frame_id; c->time_utc_us = h->time_utc_us; c->writeIndex = HOP_SAMPLES; }
                for (uint32_t i = 0; i < HOP_SAMPLES; ++i) {
                    hopD.data[REF_MIC][i] = static_cast<float>(wD.pcm24[0][hopIdx * HOP_SAMPLES + i]) * scale;
                    hopN.data[REF_MIC][i] = static_cast<float>(wN.pcm24[0][hopIdx * HOP_SAMPLES + i]) * scale;
                }
                preD.process(hopD); preN.process(hopN);
                for (uint32_t i = 0; i < HOP_SAMPLES; ++i) {
                    const float g = g0 + step * static_cast<float>(i + 1);   // identisch zu Pre_Processor_118::process
                    hopD.data[REF_MIC][i] *= g; hopN.data[REF_MIC][i] *= g;
                    const double e = std::fabs(double(h->data[REF_MIC][i]) - (double(hopD.data[REF_MIC][i]) + hopN.data[REF_MIC][i]));
                    if (e > maxRecErr) maxRecErr = e;
                    maxAbsMix = std::fmax(maxAbsMix, std::fabs(double(h->data[REF_MIC][i])));
                }
                fullD = faD.push(hopD); faN.push(hopN);
            }
            ++hopIdx;
            const bool full = fa.push(*h);
            arr.release(h);
            if (!full) continue;
            if (label && !fullD) { std::fprintf(stderr, "interner Fehler: Anteile nicht synchron\n"); return 3; }
            const AnalysisFrame& f = fa.frame();
            feat.process(f, refSpec, fv);
            data.push_back(static_cast<float>(static_cast<double>(f.time_utc_us) * 1e-6));
            data.insert(data.end(), fv.band_log_power, fv.band_log_power + NUM_BANDS);
            data.insert(data.end(), fv.mel, fv.mel + MEL_BANDS);
            data.push_back(fv.spectral_flux);
            data.insert(data.end(), fv.band_am_depth, fv.band_am_depth + NUM_BANDS);
            if (withHbd) {
                ml.infer(feat.magnitude(), fv, st);
                const HBD_State& hb = ml.hbd();
                data.push_back(hb.f0Hz); data.push_back(hb.score); data.push_back(hb.globalSnrAvgDb);
                data.push_back(static_cast<float>(hb.consistentBands)); data.push_back(hb.droneDetected ? 1.0f : 0.0f);
                data.insert(data.end(), st.p, st.p + NUM_BANDS);
            }
            if (label) {
                featD.process(faD.frame(), specD, fvD); featN.process(faN.frame(), specN, fvN);
                constexpr float kDb = 4.3429448f;                      // 10 / ln(10): ln(P) -> dB
                for (uint32_t b = 0; b < NUM_BANDS; ++b) {
                    float snr = kDb * (fvD.band_log_power[b] - fvN.band_log_power[b]);
                    data.push_back(std::fmax(-80.0f, std::fmin(80.0f, snr)));
                }
            }
            ++frames;
        }
    }
    if (arr.droppedFrames() != 0) { std::fprintf(stderr, "interner Fehler: %u Hops verworfen\n", arr.droppedFrames()); return 3; }

    if (!writeNpy(out + ".npy", data, frames, nCols)) { std::fprintf(stderr, "%s.npy nicht schreibbar\n", out.c_str()); return 3; }

    // Spaltennamen
    std::vector<std::string> cols = { "t_s" };
    char nb[32];
    for (uint32_t i = 0; i < NUM_BANDS; ++i) { std::snprintf(nb, sizeof nb, "band_log_power_%02u", i); cols.push_back(nb); }
    for (uint32_t i = 0; i < MEL_BANDS; ++i) { std::snprintf(nb, sizeof nb, "mel_%02u", i); cols.push_back(nb); }
    cols.push_back("spectral_flux");
    for (uint32_t i = 0; i < NUM_BANDS; ++i) { std::snprintf(nb, sizeof nb, "band_am_depth_%02u", i); cols.push_back(nb); }
    if (withHbd) {
        for (const char* c : { "hbd_f0_hz", "hbd_score", "hbd_snr_db", "hbd_consistent", "hbd_detected" }) cols.push_back(c);
        for (uint32_t i = 0; i < NUM_BANDS; ++i) { std::snprintf(nb, sizeof nb, "hbd_s_p_%02u", i); cols.push_back(nb); }
    }
    if (label)
        for (uint32_t i = 0; i < NUM_BANDS; ++i) { std::snprintf(nb, sizeof nb, "snr_db_%02u", i); cols.push_back(nb); }
    FILE* j = std::fopen((out + ".json").c_str(), "w");
    if (!j) { std::fprintf(stderr, "%s.json nicht schreibbar\n", out.c_str()); return 3; }
    std::fprintf(j, "{\n  \"tool\": \"sds_features\", \"tool_version\": \"%s\",\n", TOOL_VERSION);
    std::fprintf(j, "  \"feature_version\": \"%s\", \"sds110_git\": \"%s\",\n", FEATURE_VERSION, SDS110_GIT);
    if (label)
        std::fprintf(j, "  \"mode\": \"label\", \"input_drone\": %s, \"input_noise\": %s, \"clipped_samples\": %zu,\n  \"max_reconstruction_error\": %.3g, \"max_abs_mixture_after_118\": %.4g,\n",
                     jsonStr(in).c_str(), jsonStr(inN).c_str(), clipped, maxRecErr, maxAbsMix);
    else
        std::fprintf(j, "  \"mode\": \"features\",\n");
    std::fprintf(j, "  \"input\": %s, \"input_sample_rate_hz\": %u, \"input_channels\": %u, \"input_bits\": %u, \"input_float\": %s,\n",
                 jsonStr(in).c_str(), w.sampleRate, w.channels, w.bits, w.isFloat ? "true" : "false");
    std::fprintf(j, "  \"channel_mode\": %s, \"input_samples\": %zu, \"used_samples\": %zu,\n",
                 allMics ? "\"alle 8 Mikrofone\"" : jsonStr("Kanal " + std::to_string(channel) + " -> Referenzmikrofon").c_str(),
                 nSamples, nBlocks * DMA_BLOCK_SAMPLES);
    std::fprintf(j, "  \"frames\": %zu, \"columns_count\": %zu, \"with_hbd\": %s,\n", frames, nCols, withHbd ? "true" : "false");
    std::fprintf(j, "  \"chain\": {\"sample_rate_hz\": %u, \"frame_samples\": %u, \"hop_samples\": %u, \"n_fft\": %u, \"ref_mic\": %u,\n",
                 SAMPLE_RATE_HZ, FRAME_SAMPLES, HOP_SAMPLES, N_FFT, REF_MIC);
    std::fprintf(j, "            \"num_bands\": %u, \"band_lo_hz\": %.3f, \"band_width_hz\": %.3f, \"mel_bands\": %u, \"mel_lo_hz\": %.1f, \"mel_hi_hz\": %.1f,\n",
                 NUM_BANDS, BAND_LO_HZ, BAND_WIDTH_HZ, MEL_BANDS, MEL_LO_HZ, MEL_HI_HZ);
    std::fprintf(j, "            \"bandpass_hz\": [%.1f, %.1f], \"am_history_frames\": %u, \"time\": \"t_s = Zeit des ersten Samples des Frames\"},\n",
                 BANDPASS_LO_HZ, BANDPASS_HI_HZ, AM_HISTORY_FRAMES);
    std::fprintf(j, "  \"columns\": [");
    for (size_t i = 0; i < cols.size(); ++i) std::fprintf(j, "%s%s", i ? ", " : "", jsonStr(cols[i]).c_str());
    std::fprintf(j, "]\n}\n");
    std::fclose(j);
    return 0;
}
