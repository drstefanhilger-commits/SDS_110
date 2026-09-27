/*
 * DspOptimize.hpp  (Infrastructure/Utils)
 *
 * Als ERSTES #include in den rechenintensiven Modulen (Simulator, 118, 122, 124, 126): übersetzt
 * sie auf dem Board auch im Debug-Build (-O0) mit -O2. Im Debug-Build lag der ProcessingTask
 * sonst bei ~380 ms je 32-ms-Hop. Diese Dateien lassen sich dann nur eingeschränkt
 * schrittweise debuggen; mit -DSDS110_DSP_NO_OPT gilt wieder die Optimierung des Builds.
 * Host-Tests (x86) bleiben unverändert. Die Zeile geht nicht in die Merkmalsversion ein
 * (tools/features/Makefile filtert sie).
 */
#pragma once
#if defined(__arm__) && defined(__GNUC__) && !defined(__clang__) && !defined(SDS110_DSP_NO_OPT)
#pragma GCC optimize ("O2")
#endif
