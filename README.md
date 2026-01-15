# faust2spectrogram

Generate spectrograms from Faust DSP programs in one command.

## Quick Start

```bash
# Create a simple DSP
cat > osc.dsp << EOF
import("stdfaust.lib");
gate = button("gate");
freq = nentry("freq", 440, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);
process = os.osc(freq) * gate * gain;
EOF

# Generate spectrogram
faust2spectrogram osc.dsp 2 0.5 440 0.9
```

This creates a PNG showing a 2-second spectrogram with gate=1 for 0.5s, frequency 440Hz, gain 0.9.

## Installation

### 1. Install Dependencies

```bash
# macOS with Homebrew
brew install faust fftw libpng

# macOS with MacPorts
sudo port install faust fftw-3 fftw-3-single libpng

# Ubuntu/Debian
sudo apt-get install faust libfftw3-dev libpng-dev

# Fedora/RHEL
sudo dnf install faust fftw-devel libpng-devel
```

### 2. Install faust2spectrogram

Once you have faust isntalled, you can run:

```bash
sudo make install
```


## Usage

```bash
faust2spectrogram [OPTIONS] file.dsp duration gate_duration frequency gain [SPECTROGRAM_OPTIONS]
```

### Required Arguments

| Argument | Description |
|----------|-------------|
| `file.dsp` | Faust DSP file (must expose `gate`, `freq`, `gain`) |
| `duration` | Total duration in seconds |
| `gate_duration` | Duration during which gate=1 (from 0 to gate_duration) |
| `frequency` | Frequency in Hz |
| `gain` | Gain value |

### Script Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Display help |
| `-v, --verbose` | Verbose mode (show compilation details) |
| `-k, --keep` | Keep intermediate files (.cpp, executable) |
| `-o, --output <file>` | Output PNG filename |

### Spectrogram Options

| Option | Description | Default |
|--------|-------------|---------|
| `-sr <rate>` | Sample rate in Hz | 44100 |
| `-fft <size>` | FFT size (power of 2) | 2048 |
| `-hop <size>` | Hop size in samples | 512 |
| `-mel <bands>` | Number of mel bands | 128 |
| `-window <type>` | Window type: hann, hamming, blackman | hann |
| `-cmap <type>` | Colormap: viridis, magma, hot, gray | viridis |
| `-layout <type>` | Layout preset: full, minimal, scientific, raw | full |
| `-scale <factor>` | Global scale factor | 1.0 |
| `-hscale <factor>` | Horizontal scale (time axis) | 1.0 |
| `-vscale <factor>` | Vertical scale (frequency axis) | 1.0 |
| `-db` | Display in decibels | off |
| `-dbmin <val>` | Minimum dB value | -80 |

## Examples

### Basic Usage

```bash
faust2spectrogram synth.dsp 2 0.5 440 0.9
```

### High Resolution Analysis

```bash
faust2spectrogram synth.dsp 5 1.0 880 0.8 -fft 4096 -mel 256 -scale 2.0
```

### dB Scale for Weak Signals

```bash
faust2spectrogram filter.dsp 2 0.5 1000 0.9 -db -dbmin -120
```

### Scientific Layout

```bash
faust2spectrogram osc.dsp 2 0.5 440 0.9 -layout scientific -cmap magma -o analysis.png
```

### Batch Processing

```bash
# Analyze different frequencies
for freq in 220 440 880 1760; do
    faust2spectrogram synth.dsp 2 0.5 $freq 0.9 -o "synth_${freq}hz.png"
done
```

## DSP Requirements

Your Faust DSP **must** expose exactly 3 parameters with these labels:

- `gate`: button or checkbox
- `freq`: nentry, hslider, or vslider
- `gain`: nentry, hslider, or vslider

### Valid DSP Example

```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 440, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

process = os.osc(freq) * gate * gain;
```

### FM Synthesis Example

```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 440, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

modulator = os.osc(freq * 2) * 200;
process = os.osc(freq + modulator) * gate * gain;
```

### Filtered Noise Example

```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 1000, 20, 20000, 1);
gain = hslider("gain", 0.5, 0, 1, 0.01);

process = no.noise : fi.resonlp(freq, 5, 1) * gate * gain;
```

## Layout Presets

### `full` (default)
Complete visualization with title, axes, colorbar, gate line, and metadata.

### `minimal`
Essential elements: spectrogram, axes, gate line. No title, colorbar, or metadata.

### `scientific`
Publication-ready: spectrogram, clear axes, colorbar with units, subtle gate line, technical metadata.

### `raw`
Pure spectrogram pixels with no annotations.

## Troubleshooting

### Error: fftw3.h not found

```bash
# macOS
brew install fftw

# Linux
sudo apt-get install libfftw3-dev
```

### Error: Parameter "gate" not found

Your DSP must expose the three required parameters (`gate`, `freq`, `gain`). Check your DSP code.

### Use Verbose Mode for Diagnosis

```bash
faust2spectrogram -v synth.dsp 2 0.5 440 0.9
```

## How It Works

1. **Compile**: Faust compiles your DSP with the `spectrogram.cpp` architecture
2. **Link**: G++ links with FFTW3 and libpng libraries
3. **Synthesize**: The program generates audio based on your parameters
4. **Analyze**: Computes Short-Time Fourier Transform (STFT) and mel-scale conversion
5. **Visualize**: Exports a PNG with the spectrogram and optional annotations
6. **Cleanup**: Removes temporary files (unless `-k` flag is used)

## Advantages

| Manual Compilation | faust2spectrogram |
|--------------------|-------------------|
| `faust -a spectrogram.cpp foo.dsp -o foo.cpp` | ✓ Automatic |
| `g++ foo.cpp -o foo -I... -L... -lfftw3f -lpng` | ✓ Automatic |
| `./foo 2 0.5 440 0.9` | ✓ Automatic |
| Clean temporary files | ✓ Automatic |
| **4+ commands** | **1 command** |

## Output

The program generates a PNG file containing:
- Mel-scale spectrogram
- Time axis (seconds)
- Frequency axis (Hz, mel-scaled)
- Gate transition line (optional)
- Colorbar with amplitude scale (optional)
- Metadata footer (optional)

Default filename: `<dsp-name>-YYYYMMDD-HHMMSS.png`

## License

GPL v3 (as specified in the architecture header)

## Credits

Part of the Faust ecosystem. Built with FFTW3 and libpng.
