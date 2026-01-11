# Faust Spectrogram Architecture - COMPLETE IMPLEMENTATION ✅

## Files

- `spectrogram.cpp`: Complete Faust architecture for generating spectrograms
- `test.dsp`: Test Faust DSP (simple oscillator)
- `faust-spectrogram-spec.md`: Complete project specification
- `compile_and_test.sh`: Compilation and test script

## Implementation Status

### ✅ COMPLETE

1. **Architecture Structure**
   - Self-contained architecture (defines `UI`, `Meta`, `dsp` locally)
   - Compatible with standard Faust workflow

2. **DSP Parameter Capture**
   - `SpectrogramUI` class for gate/freq/gain
   - Complete validation with error messages
   - Automatic clamping with warnings

3. **CLI Parser**
   - All positional arguments
   - All options (FFT, Mel, Image, Amplitude)
   - Layout presets (full, minimal, scientific, raw)

4. **Audio Synthesis**
   - Audio generation with temporal gate control
   - Sample by sample for precise control

5. **STFT (Short-Time Fourier Transform)**
   - Windowing: Hann, Hamming, Blackman
   - FFT with FFTW3 (float precision)
   - Configurable hop size

6. **Mel Conversion**
   - Hz ↔ Mel conversion
   - Mel triangular filter bank
   - Application on FFT spectrum

7. **PNG Generation**
   - PNG export with libpng
   - Colormaps: viridis, magma, hot, gray
   - Scaling (global, horizontal, vertical)
   - Optional dB conversion
   - Automatic normalization

### ⚠️ Annotations (structure present but minimal rendering)

The following functions are partially implemented:
- Time and frequency axes
- Gate line
- Colorbar
- Title and legend
- PNG metadata

The generated PNG contains the raw spectrogram. Annotations can be added later.

## Dependencies

### Prerequisites

```bash
# Installation on macOS with Homebrew
brew install fftw libpng

# Installation on Ubuntu/Debian
sudo apt-get install libfftw3-dev libpng-dev

# Installation on Fedora/RHEL
sudo dnf install fftw-devel libpng-devel
```

## Compilation and Usage

### Automatic Compilation

```bash
# Make the script executable
chmod +x compile_and_test.sh

# Compile and test
./compile_and_test.sh
```

### Manual Compilation

```bash
# 1. Compile the DSP with Faust
faust -a spectrogram.cpp test.dsp -o test.cpp

# 2. Compile the C++ with libraries
g++ test.cpp -o test -std=c++11 -O3 -lfftw3f -lpng -lm

# Or with clang++
clang++ test.cpp -o test -std=c++11 -O3 -lfftw3f -lpng -lm
```

**Note**: Use `-lfftw3f` (float) and not `-lfftw3` (double) for better performance.

### Usage

```bash
# Basic
./test 2 0.5 440 0.9

# With options
./test 2 0.5 440 0.9 -sr 48000 -fft 4096 -mel 256

# High resolution
./test 5 1.0 880 0.8 -scale 2.0 -layout scientific

# dB scale
./test 2 0.5 440 0.9 -db -dbmin -100

# Custom colormap
./test 2 0.5 440 0.9 -cmap magma -layout minimal
```

## Output

The program generates:
- A PNG file with the mel spectrogram
- Automatic name: `test-YYYYMMDD-HHMMSS.png`
- Or custom name with `-o <file>`

### Console Output Example

```
DSP Parameters:
  gate: button
  freq: nentry [20, 20000]
  gain: hslider [0, 1]

Synthesizing audio...
  Generated 88200 samples

Generating spectrogram...
  Audio samples: 88200
  FFT size: 2048
  Hop size: 512
  Mel bands: 128
  Computing STFT...
  Creating mel filterbank...
  Applying mel filterbank...
  Normalizing...
  Writing PNG: test-20260111-184530.png
✓ Spectrogram saved to: test-20260111-184530.png
```

## Available Options

See `faust-spectrogram-spec.md` for the complete list of options.

### Main Options

| Option | Description | Default |
|--------|-------------|--------|
| `-sr <rate>` | Sample rate | 44100 |
| `-fft <size>` | FFT size | 2048 |
| `-hop <size>` | Hop size | 512 |
| `-mel <bands>` | Mel bands | 128 |
| `-window <type>` | hann\|hamming\|blackman | hann |
| `-cmap <type>` | viridis\|magma\|hot\|gray | viridis |
| `-layout <type>` | full\|minimal\|scientific\|raw | full |
| `-scale <f>` | Global scale | 1.0 |
| `-db` | Use dB scale | false |
| `-o <file>` | Output file | auto |

## Usage Examples

### 1. Simple Oscillator Analysis
```bash
./test 2 0.5 440 0.9
```
Generates 2s of audio with gate=1 for 0.5s, frequency 440Hz, gain 0.9

### 2. High Resolution Analysis
```bash
./test 5 1.0 880 0.8 -fft 4096 -hop 256 -mel 256 -scale 2.0
```
5 seconds, FFT 4096, 256 mel bands, image 2× larger

### 3. dB Scale for Weak Signals
```bash
./test 2 0.5 440 0.9 -db -dbmin -120
```
Display in dB with -120dB threshold

### 4. Alternative Colormap
```bash
./test 2 0.5 440 0.9 -cmap magma -layout scientific
```
Magma colormap, scientific layout

## Different DSP Tests

### FM Oscillator
```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 440, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

// FM synthesis
carrier = os.osc(freq);
modulator = os.osc(freq * 2) * 100;
process = os.osc(freq + modulator) * gate * gain;
```

### Resonant Filter
```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 1000, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

noise = no.noise;
filtered = fi.resonlp(freq, 10, 1);
process = noise : filtered * gate * gain;
```

## MCP Integration (next step)

The architecture is ready to be integrated into a dockerized MCP server allowing Claude to:
1. Generate Faust code
2. Compile with this architecture
3. Visualize the result via spectrogram
4. Iterate on the design

## Future Improvements

1. **Complete Annotations**
   - Axes with precise graduations
   - Stylized gate line
   - Colorbar with scale
   - Title and metadata

2. **PNG Metadata**
   - tEXt chunks with parameters
   - Complete traceability

3. **Additional Formats**
   - SVG export
   - Raw data export (CSV/JSON)

4. **Performance**
   - FFT parallelization
   - SIMD optimizations

5. **Advanced Features**
   - Comparative spectrograms
   - GIF animation
   - Multi-channel support

## License

GPL v3 (as specified in the architecture header)

## Support

For any questions or issues:
- Verify that FFTW3 and libpng are properly installed
- Verify that the DSP properly exposes `gate`, `freq`, `gain`
- Consult the complete specification in `faust-spectrogram-spec.md`
