#pragma once
#include <cstdint>

/**
 * @brief Kapselt die Mel-Filterbank (40 Bänder × 129 FFT-Bins).
 *
 * Die Filterbank liegt im Flash (const), wird nicht verändert
 * und ist damit perfekt für Embedded-Systeme.
 */
class MelFilterbank
{
public:
    static constexpr int MEL_BANDS = 40;
    static constexpr int FFT_BINS  = 129;

    MelFilterbank();

    /**
     * @brief Liefert Zeiger auf ein Mel-Band.
     */
    const float* band(int index) const { return filterbank[index]; }

private:
    static const float filterbank[MEL_BANDS][FFT_BINS];
};
