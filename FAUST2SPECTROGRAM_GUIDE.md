# faust2spectrogram - User Guide

## Installation

### 1. Copy Necessary Files

```bash
# Copy the architecture
sudo cp spectrogram.cpp /usr/local/share/faust/architecture/
# Or to your local folder
cp spectrogram.cpp ~/.faust/architecture/

# Copy the script
sudo cp faust2spectrogram /usr/local/bin/
# Or add to your PATH
chmod +x faust2spectrogram
export PATH=$PATH:$(pwd)
```

### 2. Verify Dependencies

```bash
# macOS with MacPorts
sudo port install fftw-3 fftw-3-single libpng

# macOS with Homebrew
brew install fftw libpng

# Ubuntu/Debian
sudo apt-get install libfftw3-dev libpng-dev

# Fedora/RHEL
sudo dnf install fftw-devel libpng-devel
```

## Basic Usage

### Syntax

```bash
faust2spectrogram [OPTIONS] file.dsp duration gate_duration frequency gain [SPECTROGRAM_OPTIONS]
```

### Required Arguments

| Argument | Description |
|----------|-------------|
| `file.dsp` | Faust DSP file (must expose `gate`, `freq`, `gain`) |
| `duration` | Total duration in seconds |
| `gate_duration` | Duration during which gate=1 |
| `frequency` | Frequency in Hz |
| `gain` | Gain |

### Script Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Display help |
| `-v, --verbose` | Verbose mode |
| `-k, --keep` | Keep intermediate files (.cpp, executable) |
| `-o, --output` | Output PNG filename |

### Spectrogram Options

All options from `spectrogram.cpp` are supported:

| Option | Description | Default |
|--------|-------------|--------|
| `-sr <rate>` | Sample rate | 44100 |
| `-fft <size>` | FFT size | 2048 |
| `-hop <size>` | Hop size | 512 |
| `-mel <bands>` | Mel bands | 128 |
| `-window <type>` | hann\|hamming\|blackman | hann |
| `-cmap <type>` | viridis\|magma\|hot\|gray | viridis |
| `-layout <type>` | full\|minimal\|scientific\|raw | full |
| `-scale <f>` | Global scale factor | 1.0 |
| `-hscale <f>` | Horizontal scale | 1.0 |
| `-vscale <f>` | Vertical scale | 1.0 |
| `-db` | Display in decibels | false |
| `-dbmin <val>` | Minimum dB | -80 |

## Examples

### 1. Basic Usage

```bash
faust2spectrogram synth.dsp 2 0.5 440 0.9
```

Generates a 2-second spectrogram with gate=1 for 0.5s, frequency 440Hz, gain 0.9.

### 2. High Resolution

```bash
faust2spectrogram synth.dsp 5 1.0 880 0.8 -fft 4096 -mel 256 -scale 2.0
```

5 seconds, FFT 4096, 256 mel bands, image enlarged 2×.

### 3. dB Scale

```bash
faust2spectrogram filter.dsp 2 0.5 1000 0.9 -db -dbmin -120
```

Display in decibels with -120dB threshold.

### 4. Scientific Layout

```bash
faust2spectrogram osc.dsp 2 0.5 440 0.9 -layout scientific -cmap magma
```

Clean layout with magma colormap.

### 5. Custom Output File

```bash
faust2spectrogram --output analysis.png synth.dsp 2 0.5 440 0.9
```

Or with short syntax:

```bash
faust2spectrogram synth.dsp 2 0.5 440 0.9 -o analysis.png
```

### 6. Verbose Mode with File Preservation

```bash
faust2spectrogram -v -k synth.dsp 2 0.5 440 0.9
```

Displays compilation details and preserves `synth.cpp` and the `synth` executable.

## DSP Examples

### Simple Oscillator

```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 440, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

process = os.osc(freq) * gate * gain;
```

```bash
faust2spectrogram osc.dsp 2 0.5 440 0.9
```

### FM Synthesis

```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 440, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

modulator = os.osc(freq * 2) * 200;
process = os.osc(freq + modulator) * gate * gain;
```

```bash
faust2spectrogram fm.dsp 3 1.0 220 0.8 -mel 256
```

### Filtered Noise

```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 1000, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

process = no.noise : fi.resonlp(freq, 5, 1) * gate * gain;
```

```bash
faust2spectrogram filter.dsp 4 2.0 1000 0.7 -db -dbmin -100
```

### ADSR Envelope

```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 440, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

envelope = en.adsr(0.01, 0.1, 0.7, 0.5, gate);
process = os.sawtooth(freq) * envelope * gain;
```

```bash
faust2spectrogram adsr.dsp 3 0.5 440 0.9 -scale 2.0
```

## Typical Workflow

### 1. Iterative Development

```bash
# Quick test
faust2spectrogram synth.dsp 1 0.3 440 0.5

# Adjust the DSP...

# Test with more details
faust2spectrogram synth.dsp 2 0.5 440 0.9 -mel 256 -db

# Final high-quality version
faust2spectrogram synth.dsp 10 2.0 440 0.9 \
    -fft 4096 -mel 512 -scale 3.0 -layout scientific \
    -o final_analysis.png
```

### 2. Comparative Analysis

```bash
# Analyze different frequencies
for freq in 220 440 880 1760; do
    faust2spectrogram synth.dsp 2 0.5 $freq 0.9 -o "synth_${freq}hz.png"
done

# Analyze different timbres
for dsp in sine.dsp square.dsp saw.dsp; do
    faust2spectrogram $dsp 2 0.5 440 0.9 -o "${dsp%.dsp}_spectrum.png"
done
```

## Troubleshooting

### Error: Architecture file not found

```bash
# Solution 1: Copy to current directory
cp /path/to/spectrogram.cpp .

# Solution 2: Install globally
sudo cp spectrogram.cpp /usr/local/share/faust/architecture/
```

### Error: fftw3.h not found

```bash
# macOS MacPorts
sudo port install fftw-3-single

# macOS Homebrew
brew install fftw

# Linux
sudo apt-get install libfftw3-dev
```

### Error: Parameter "gate" not found

Your DSP must expose exactly these 3 parameters:
- `gate`: button or checkbox
- `freq`: nentry, hslider, or vslider
- `gain`: nentry, hslider, or vslider

### Verbose Mode for Diagnosis

```bash
faust2spectrogram -v synth.dsp 2 0.5 440 0.9
```

This will display all compilation details.

## Workflow Integration

### Makefile

```makefile
%.png: %.dsp
	faust2spectrogram $< 2 0.5 440 0.9 -o $@

all: osc.png fm.png filter.png

clean:
	rm -f *.png
```

### Batch Script

```bash
#!/bin/bash
for dsp in *.dsp; do
    echo "Processing $dsp..."
    faust2spectrogram "$dsp" 2 0.5 440 0.9
done
```

## Advantages Over Manual Compilation

| Manual | faust2spectrogram |
|--------|-------------------|
| `faust -a spectrogram.cpp foo.dsp -o foo.cpp` | ✓ Automatic |
| `g++ foo.cpp -o foo -I... -L... -lfftw3f -lpng` | ✓ Automatic |
| `./foo 2 0.5 440 0.9` | ✓ Automatic |
| Clean temporary files | ✓ Automatic |
| 4 commands | **1 command** |

## License

GPL v3 (like spectrogram.cpp architecture)
