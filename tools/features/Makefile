# sds_features – Merkmalswerkzeug (Arbeitspaket 1, Trainingskonzept ML124), Beschreibung: doc/Merkmalswerkzeug.md
#
#   make -C tools/features           # baut build/tools/sds_features
#   make -C tools/features check     # Gleichheit mit der Board-Kette, Plausibilität, Reproduzierbarkeit
#   make -C tools/features version   # Merkmalsversion ausgeben
#
# Übersetzt den unveränderten Code aus Core/SDS_110 gegen die Stubs aus test/host/shim.

PROJ  := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../..)
HERE  := $(PROJ)/tools/features
S     := $(PROJ)/Core/SDS_110
BUILD ?= $(PROJ)/build/tools
CXX   ?= g++
CC    ?= gcc

# Merkmalsversion: SHA-256 über den Quelltext der Merkmalskette (Config, 114, 118, Frame_Assembler, 122)
FEATURE_SRCS := $(S)/SDS_110_Config.hpp \
                $(S)/Sensor_Unit_112/Microphone_Array_114.hpp $(S)/Sensor_Unit_112/Microphone_Array_114.cpp \
                $(S)/Sensor_Unit_112/Pre_Processor_118.hpp $(S)/Sensor_Unit_112/Pre_Processor_118.cpp \
                $(S)/Sensor_Unit_112/Frame_Assembler.hpp $(S)/Sensor_Unit_112/Frame_Assembler.cpp \
                $(S)/Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.hpp \
                $(S)/Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.cpp
FEATURE_VERSION := $(shell cat $(FEATURE_SRCS) | sha256sum | cut -c1-16)
SDS110_GIT      := $(shell git -C $(PROJ) describe --always --dirty 2>/dev/null || echo unbekannt)

CXXFLAGS := -std=gnu++17 -O2 -Wall -I$(PROJ)/test/host/shim -I$(S) -I$(PROJ)/Core/Lib -I$(HERE)
DEFS     := -DFEATURE_VERSION='"$(FEATURE_VERSION)"' -DSDS110_GIT='"$(SDS110_GIT)"'

CHAIN := $(S)/Sensor_Unit_112/Microphone_Array_114.cpp $(S)/Sensor_Unit_112/Pre_Processor_118.cpp \
         $(S)/Sensor_Unit_112/Frame_Assembler.cpp \
         $(S)/Processing_Module_120/Feature_Extraction_Module_122/Feature_Extraction_Module_122.cpp \
         $(S)/Processing_Module_120/Machine_Learning_Module_124/Machine_Learning_Module_124.cpp \
         $(S)/Processing_Module_120/Machine_Learning_Module_124/HBD.cpp \
         $(PROJ)/test/host/shim/arm_rfft.cpp

all: $(BUILD)/sds_features

$(BUILD)/kiss_fft.o: $(PROJ)/Core/Lib/kiss_fft.c
	@mkdir -p $(BUILD)
	$(CC) -O2 -I$(PROJ)/Core/Lib -c $< -o $@

# Neu bauen, wenn sich Quellen oder die Merkmalsversion ändern
$(BUILD)/.feature_version: FORCE
	@mkdir -p $(BUILD)
	@echo "$(FEATURE_VERSION) $(SDS110_GIT)" | cmp -s - $@ || echo "$(FEATURE_VERSION) $(SDS110_GIT)" > $@

$(BUILD)/sds_features: $(HERE)/sds_features.cpp $(HERE)/wav_io.hpp $(HERE)/npy_writer.hpp $(CHAIN) $(FEATURE_SRCS) $(BUILD)/kiss_fft.o $(BUILD)/.feature_version
	$(CXX) $(CXXFLAGS) $(DEFS) $(HERE)/sds_features.cpp $(CHAIN) $(BUILD)/kiss_fft.o -o $@

$(BUILD)/check_features: $(HERE)/check_features.cpp $(CHAIN) $(S)/Harness/Signal_Simulator.cpp $(BUILD)/kiss_fft.o
	$(CXX) $(CXXFLAGS) $(HERE)/check_features.cpp $(CHAIN) $(S)/Harness/Signal_Simulator.cpp $(BUILD)/kiss_fft.o -o $@

check: $(BUILD)/sds_features $(BUILD)/check_features
	@mkdir -p $(BUILD)/check
	$(BUILD)/check_features $(BUILD)/sds_features $(BUILD)/check

version:
	@echo $(FEATURE_VERSION)

.PHONY: all check version FORCE
