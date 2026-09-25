/*
 * SDRAMSelfTest.hpp  (Infrastructure/Driver)
 *
 * Selbsttest des externen SDRAM vor der ersten Nutzung (SDS110_Init, vor jedem
 * Konstruktor eines Objekts in .sdram_data):
 *  1. FMC-Handle muss READY sein (sonst wird das SDRAM nicht angefasst -> kein BusFault).
 *  2. Muster an Offset 0, 2^k Wörtern und am letzten Wort des Bereichs schreiben
 *     (je Adresse eigener Wert -> erkennt auch hängende/verbundene Adressleitungen),
 *     D-Cache leeren + verwerfen, zurücklesen; dann dasselbe invertiert (hängende Datenbits).
 *  Der D-Cache-Schritt ist nötig, weil das SDRAM per MPU Write-Back-cachebar ist
 *  (MPUDriver.h): ohne ihn würde der Test aus dem Cache lesen und immer bestehen.
 *  Der Bereich ist .sdram_data (NOLOAD, zu diesem Zeitpunkt unbenutzt) – der Test
 *  darf ihn überschreiben.
 */
#pragma once
#include <cstdint>
#include "stm32f7xx_hal.h"

namespace sds110 {

bool sdramSelfTest(const SDRAM_HandleTypeDef& hsdram, uint32_t* begin, uint32_t* end);

} // namespace sds110
