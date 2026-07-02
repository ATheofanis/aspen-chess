ifndef EXE
    EXE = aspen
endif

# If a different eval file is passed then copy it over to the default network path
ifdef EVALFILE
    _:=$(shell mkdir -p NetworkFiles && cp $(EVALFILE) NetworkFiles/aspen-net562-8.bin)
endif

# Default build is native (march=native flag)
TYPE ?= NATIVE

NATIVE_FLAGS = -march=native
AVX2_FLAGS = -march=x86-64-v3

CXX ?= g++

# Base release flags for the executable
RELEASE_FLAGS = -Ofast -std=c++20 -static -flto -fomit-frame-pointer -funroll-loops -DNDEBUG "-Wa,-Isrc/nnue"

# Combine the base flags into the engines flags
ENGINE_FLAGS = $(RELEASE_FLAGS)

# Only two build types for now native and avx2
ifeq ($(TYPE), NATIVE)
	ENGINE_FLAGS += $(NATIVE_FLAGS)
else ifeq ($(TYPE), AVX2)
	ENGINE_FLAGS += $(AVX2_FLAGS)
else
    $(error Unknown build type)
endif

# All the .cpp and .h files from the src folder
FILES = $(wildcard src/*.cpp) $(wildcard src/nnue/*.cpp)

$(EXE): $(FILES)
	$(CXX) $(ENGINE_FLAGS) $(FILES) -o $(EXE)

NATIVE:
	$(MAKE) TYPE=NATIVE

AVX2:
	$(MAKE) TYPE=AVX2
