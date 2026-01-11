#!/bin/bash

# Script de compilation et test de l'architecture spectrogram.cpp
# Usage: ./compile_and_test.sh

set -e  # Exit on error

echo "=== Compilation du DSP Faust avec l'architecture spectrogram.cpp ==="

# Chemin vers Faust
FAUST="faust"

# Compiler le DSP avec notre architecture
echo "Compiling test.dsp with spectrogram.cpp architecture..."
$FAUST -a spectrogram.cpp test.dsp -o test.cpp

# Vérifier que test.cpp a été généré
if [ ! -f "test.cpp" ]; then
    echo "Error: test.cpp was not generated"
    exit 1
fi

echo "✓ test.cpp generated successfully"

# Compiler le C++ avec FFTW3 et libpng
echo ""
echo "Compiling test.cpp to executable..."
clang++-mp-18 test.cpp -o test -I/opt/local/include -L/opt/local/lib -std=c++11 -O3 -lfftw3f -lpng -lm

if [ ! -f "test" ]; then
    echo "Error: test executable was not generated"
    exit 1
fi

echo "✓ test executable generated successfully"

# Test l'exécutable
echo ""
echo "=== Testing executable ==="
echo "Running: ./test 2 0.5 440 0.9"
echo ""
./test 2 0.5 440 0.9

echo ""
echo "=== Test complete ==="
echo ""
echo "The spectrogram should be saved as: test-YYYYMMDD-HHMMSS.png"
echo ""
echo "Try different options:"
echo "  ./test 2 0.5 440 0.9 -mel 256 -cmap magma"
echo "  ./test 5 1.0 880 0.8 -scale 2.0 -layout scientific"
echo "  ./test 2 0.5 440 0.9 -db -dbmin -100"
