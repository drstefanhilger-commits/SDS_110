/*
 * npy_writer.hpp – schreibt eine 2D-Matrix float32 (C-Reihenfolge) als NumPy-.npy (Format 1.0).
 */
#pragma once
#include <cstdio>
#include <string>
#include <vector>

inline bool writeNpy(const std::string& path, const std::vector<float>& data, size_t rows, size_t cols)
{
    std::string h = "{'descr': '<f4', 'fortran_order': False, 'shape': (" + std::to_string(rows) + ", " + std::to_string(cols) + "), }";
    const size_t pre = 10;                                          // Magic(6) + Version(2) + Headerlänge(2)
    while ((pre + h.size() + 1) % 64) h += ' ';
    h += '\n';
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const unsigned char magic[8] = { 0x93, 'N', 'U', 'M', 'P', 'Y', 1, 0 };
    const unsigned short hl = static_cast<unsigned short>(h.size());
    std::fwrite(magic, 1, 8, f);
    const unsigned char hlb[2] = { static_cast<unsigned char>(hl & 0xFF), static_cast<unsigned char>(hl >> 8) };
    std::fwrite(hlb, 1, 2, f);
    std::fwrite(h.data(), 1, h.size(), f);
    std::fwrite(data.data(), sizeof(float), data.size(), f);
    return std::fclose(f) == 0;
}
