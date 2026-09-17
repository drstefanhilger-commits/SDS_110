#!/usr/bin/env bash
# sds110_import.sh – kopiert Core/SDS_110 aus einem gelieferten ZIP in das CubeIDE-Projekt.
#
# Aufruf (WSL Ubuntu):
#   ./sds110_import.sh                      # nimmt die neueste *.zip aus dem Windows-Downloads-Ordner
#   ./sds110_import.sh ~/Downloads/files.zip
#   ./sds110_import.sh files.zip --dry-run  # nur anzeigen, nichts schreiben
#   ./sds110_import.sh files.zip --clean    # zusätzlich Dateien löschen, die im Projekt, aber nicht im ZIP sind
#
# Vor jedem Kopieren wird Core/SDS_110 nach $BACKUP_DIR/SDS_110_<Zeitstempel>/ gesichert
# (AUSSERHALB des Projekts – alles unter Core/ würde CubeIDE mitkompilieren).

set -euo pipefail

PROJECT="/mnt/c/Users/310004/Documents/Projects/Sound_Detection/project/SDS_110"
DOWNLOADS="/mnt/c/Users/310004/Downloads"
SUBDIR="Core/SDS_110"
BACKUP_DIR="$PROJECT/../SDS_110_backup"

# Dateien, die durch die Migration obsolet wurden und auch ohne --clean entfernt werden
OBSOLETE=(
  "Sensor_Unit_112/Feature_Extraction_Module_122.cpp"
  "Sensor_Unit_112/Feature_Extraction_Module_122.hpp"
  "Sensor_Unit_112/MIGRATION_122.md"
  "Infrastructure/Utils/syscalls.c"      # CubeIDE-Version in Core/Src bleibt
  "MIGRATION_TASKS.md"                    # liegt jetzt unter Infrastructure/Tasks
)

ZIP=""
DRY=0
CLEAN=0
for a in "$@"; do
  case "$a" in
    --dry-run) DRY=1 ;;
    --clean)   CLEAN=1 ;;
    *)         ZIP="$a" ;;
  esac
done

# ---------------------------------------------------------------- ZIP finden
if [[ -z "$ZIP" ]]; then
  ZIP=$(ls -t "$DOWNLOADS"/*.zip 2>/dev/null | head -1 || true)
  [[ -n "$ZIP" ]] || { echo "Kein ZIP in $DOWNLOADS gefunden"; exit 1; }
  echo "Verwende neuestes ZIP: $ZIP"
fi
[[ -f "$ZIP" ]] || { echo "ZIP nicht gefunden: $ZIP"; exit 1; }
[[ -d "$PROJECT" ]] || { echo "Projekt nicht gefunden: $PROJECT"; exit 1; }
command -v unzip >/dev/null || { echo "unzip fehlt: sudo apt install unzip"; exit 1; }
command -v rsync >/dev/null || { echo "rsync fehlt: sudo apt install rsync"; exit 1; }

# ---------------------------------------------------------------- entpacken
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
unzip -q "$ZIP" -d "$TMP"
# verschachtelte ZIPs (z. B. "files.zip" aus dem Chat enthält SDS_110_Struktur.zip) mit entpacken
while inner=$(find "$TMP" -type f -name '*.zip' | head -1) && [[ -n "$inner" ]]; do
  unzip -q -o "$inner" -d "${inner%.zip}_x" && rm -f "$inner"
done

DST="$PROJECT/$SUBDIR"

# Quelle finden – ZIP kann SDS_110/Core/SDS_110 oder direkt Core/SDS_110 enthalten
SRC=$(find "$TMP" -type d -path "*/$SUBDIR" | head -1 || true)

# ---------------------------------------------------------------- Fallback: flache Einzeldateien
# Kein Core/SDS_110 im ZIP -> Dateien anhand ihres Namens im bestehenden Projektbaum einsortieren.
if [[ -z "$SRC" ]]; then
  echo "Kein Ordner $SUBDIR im ZIP – Einzeldatei-Modus."
  SRC="$TMP/_flat/$SUBDIR"
  mkdir -p "$SRC"
  # bekannte Ziele für Dateien, die im Projekt noch nicht existieren
  declare -A KNOWN=(
    [SDS_110_Config.hpp]="."         [SDS_110_Wrapper.hpp]="."         [SDS_110_Wrapper.cpp]="."
    [Candidate_Report_140.hpp]="Data_Interface_140"
    [Microphone_Array_114.hpp]="Sensor_Unit_112"   [Microphone_Array_114.cpp]="Sensor_Unit_112"
    [Sampling_Circuitry_116.hpp]="Sensor_Unit_112" [Sampling_Circuitry_116.cpp]="Sensor_Unit_112"
    [Pre_Processor_118.hpp]="Sensor_Unit_112"      [Pre_Processor_118.cpp]="Sensor_Unit_112"
    [Sensor_Unit_112.hpp]="Sensor_Unit_112"        [ADAU7118_Registers.hpp]="Sensor_Unit_112"
    [MIGRATION_112.md]="Sensor_Unit_112"
    [Processing_Module_120.hpp]="Processing_Module_120"
    [Feature_Extraction_Module_122.hpp]="Processing_Module_120/Feature_Extraction_Module_122"
    [Feature_Extraction_Module_122.cpp]="Processing_Module_120/Feature_Extraction_Module_122"
    [MIGRATION_122.md]="Processing_Module_120/Feature_Extraction_Module_122"
    [Machine_Learning_Module_124.hpp]="Processing_Module_120/Machine_Learning_Module_124"
    [Machine_Learning_Module_124.cpp]="Processing_Module_120/Machine_Learning_Module_124"
    [Correlation_Processing_Module_126.hpp]="Processing_Module_120/Correlation_Processing_Module_126"
    [Correlation_Processing_Module_126.cpp]="Processing_Module_120/Correlation_Processing_Module_126"
    [Localisation_Module_128.hpp]="Processing_Module_120/Localisation_Module_128"
    [Localisation_Module_128.cpp]="Processing_Module_120/Localisation_Module_128"
    [Output_Interface_130.hpp]="Processing_Module_120/Output_Interface_130"
    [Output_Interface_130.cpp]="Processing_Module_120/Output_Interface_130"
    [ProcessingTask.hpp]="Infrastructure/Tasks"    [ProcessingTask.cpp]="Infrastructure/Tasks"
    [TaskBase.hpp]="Infrastructure/Tasks"          [TaskBase.cpp]="Infrastructure/Tasks"
    [USBTask.hpp]="Infrastructure/Tasks"           [USBTask.cpp]="Infrastructure/Tasks"
    [LCDTask.hpp]="Infrastructure/Tasks"           [LCDTask.cpp]="Infrastructure/Tasks"
    [LoggerTask.hpp]="Infrastructure/Tasks"        [LoggerTask.cpp]="Infrastructure/Tasks"
    [MIGRATION_TASKS.md]="Infrastructure/Tasks"
    [USBDriver.hpp]="Infrastructure/Driver"        [USBDriver.cpp]="Infrastructure/Driver"
    [LCDDriver.hpp]="Infrastructure/Driver"        [Font8x12.hpp]="Infrastructure/Driver"
    [Font8x12.cpp]="Infrastructure/Driver"         [SDRAMDriver.h]="Infrastructure/Driver"
    [PrintfDriver.h]="Infrastructure/Driver"       [MPUDriver.h]="Infrastructure/Driver"
    [Logger.hpp]="Infrastructure/Utils"            [Logger.cpp]="Infrastructure/Utils"
    [DWT.hpp]="Infrastructure/Utils"               [crc32.hpp]="Infrastructure/Utils"
    [Model.hpp]="Infrastructure/Model"             [SDS_Data.hpp]="Infrastructure/Model"
    [SDS_Data.cpp]="Infrastructure/Model"          [SDS_Params.hpp]="Infrastructure/Model"
    [SDS_Structs.hpp]="Infrastructure/Model"       [MIGRATION_MODEL.md]="Infrastructure/Model"
  )
  while IFS= read -r f; do
    name=$(basename "$f")
    # 1) existiert im Projekt genau einmal -> dorthin
    hits=$(find "$DST" -type f -name "$name" 2>/dev/null | wc -l)
    if [[ $hits -eq 1 ]]; then
      rel=$(find "$DST" -type f -name "$name" | sed "s|^$DST/||")
    elif [[ -n "${KNOWN[$name]:-}" ]]; then
      rel="${KNOWN[$name]}/$name"
    else
      echo "  ??      $name  (Ziel unbekannt, übersprungen)"
      continue
    fi
    mkdir -p "$SRC/$(dirname "$rel")"
    cp "$f" "$SRC/$rel"
  done < <(find "$TMP" -type f ! -path "$TMP/_flat/*" ! -name '*.sh')
fi
[[ -n "$(find "$SRC" -type f 2>/dev/null | head -1)" ]] || { echo "Nichts zu importieren."; exit 1; }

# Windows-Zeilenenden für CubeIDE (optional, aber vermeidet gemischte Dateien im Git-Diff)
if command -v unix2dos >/dev/null; then
  find "$SRC" -type f \( -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name '*.md' \) -exec unix2dos -q {} +
fi

echo "Quelle : $SRC"
echo "Ziel   : $DST"
echo

# ---------------------------------------------------------------- Vorschau
RSYNC_OPTS=(-rci --no-perms --no-owner --no-group --exclude '.gitkeep')
[[ $CLEAN -eq 1 ]] && RSYNC_OPTS+=(--delete)
echo "Änderungen:"
rsync "${RSYNC_OPTS[@]}" --dry-run "$SRC/" "$DST/" | grep -E '^(>f|cd|\*deleting)' | sed 's/^>f[^ ]* /  update  /; s/^cd[^ ]* /  newdir  /; s/^\*deleting /  delete  /' || true
echo

[[ $DRY -eq 1 ]] && { echo "--dry-run: nichts geschrieben."; exit 0; }

# ---------------------------------------------------------------- Backup + Kopie
if [[ -d "$DST" ]]; then
  BK="$BACKUP_DIR/SDS_110_$(date +%Y%m%d_%H%M%S)"
  mkdir -p "$BK"
  cp -r "$DST/." "$BK/"
  echo "Backup: $BK"
fi
mkdir -p "$DST"
rsync "${RSYNC_OPTS[@]}" "$SRC/" "$DST/" >/dev/null

# Altlast aus früherer Script-Version: Backup innerhalb von Core/ wird mitkompiliert
if [[ -d "$PROJECT/Core/_backup" ]]; then
  mkdir -p "$BACKUP_DIR" && mv "$PROJECT/Core/_backup"/* "$BACKUP_DIR"/ 2>/dev/null || true
  rm -rf "$PROJECT/Core/_backup"
  echo "Core/_backup nach $BACKUP_DIR verschoben."
fi
for f in "${OBSOLETE[@]}"; do
  [[ -f "$DST/$f" ]] && { rm -f "$DST/$f"; echo "  delete  $f (obsolet)"; }
done
find "$DST" -type d -empty -delete
echo "Fertig. In CubeIDE: Projekt markieren, F5 (Refresh), dann Build."
