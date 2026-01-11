# Faust Architecture: Command-Line Spectrogram

## Overview

This specification describes a custom Faust architecture (`spectrogram.cpp`) that generates command-line applications capable of:
1. Synthesizing audio from a Faust program
2. Computing a mel spectrogram
3. Exporting the result as an annotated PNG image

## Concept and Architecture

### Principle
- **DSP/Architecture Separation**: Faust code describes the audio algorithm, the architecture handles execution and visualization
- **Standardized Interface**: The Faust DSP must expose 3 parameters: `gate`, `freq`, `gain`
- **Automated Workflow**: From audio synthesis to spectrogram in a single command

### Compilation Workflow

```bash
# 1. Generate the application's C++ code
faust -a spectrogram.cpp foo.dsp -o foo.cpp

# 2. Compile the application
g++ foo.cpp -o foo [linking options]

# 3. Execute
./foo 2 0.5 440 0.9
```

## Faust Interface Contract

### Required Labels

The Faust program **must** expose exactly 3 parameters with the following labels:

| Label | Description | Accepted Types | Constraints |
|-------|-------------|----------------|-------------|
| `"gate"` | Gate signal (on/off) | `button`, `checkbox` | Value 0 or 1 |
| `"freq"` | Frequency in Hz | `nentry`, `hslider`, `vslider` | Range [min, max] defined by the widget |
| `"gain"` | Linear gain | `nentry`, `hslider`, `vslider` | Range [min, max] defined by the widget |

### Valid DSP Example

```faust
import("stdfaust.lib");

gate = button("gate");
freq = nentry("freq", 440, 20, 20000, 1);    // min=20Hz, max=20kHz
gain = hslider("gain", 0.5, 0, 1, 0.01);     // min=0, max=1

process = os.osc(freq) * gate * gain;
```

### Parameter Capture Mechanism

The `spectrogram.cpp` architecture implements a `UI` class that:

1. **Captures memory pointers** of parameters via `buildUserInterface()`
2. **Stores the ranges** (min, max) for `freq` and `gain`
3. **Associates labels** with memory addresses
4. **Validates the presence** of the 3 required labels at startup

### CLI Value Clamping

Values provided on the command line are **constrained** within the ranges defined by the Faust widgets:

```bash
# DSP defines: freq in [20, 20000], gain in [0, 1]
./foo 2 0.5 50000 1.5

# Output:
Warning: freq=50000 exceeds range [20, 20000], clamped to 20000
Warning: gain=1.5 exceeds range [0, 1], clamped to 1.0
Generating spectrogram: foo-20260111-143022.png
```

**Behavior**:
- Value < min → clamped to min
- Value > max → clamped to max
- Value within range → used as is
- A warning is displayed for each clamping applied

## Command-Line Interface

### General Syntax

```bash
foo [OPTIONS] <duration> <gate_duration> <frequency> <gain>
```

### Positional Arguments (required)

| Argument | Type | Description |
|----------|------|-------------|
| `duration` | float | Total sound duration in seconds |
| `gate_duration` | float | Duration during which gate=1 (from start to this instant) |
| `frequency` | float | Pitch value in Hz (will be clamped within the range defined by the `"freq"` widget) |
| `gain` | float | Gain value (will be clamped within the range defined by the `"gain"` widget) |

**Gate Behavior**:
- `gate = 1` for `t ∈ [0, gate_duration[`
- `gate = 0` for `t ∈ [gate_duration, duration]`

**Automatic Clamping**:
The `frequency` and `gain` values are automatically constrained within the ranges defined by the corresponding Faust widgets. A warning is displayed if clamping is applied.

### Options

#### Audio

| Option | Type | Default | Description |
|--------|------|--------|-------------|
| `-sr <rate>` | int | 44100 | Sample rate in Hz |

#### FFT

| Option | Type | Default | Description |
|--------|------|--------|-------------|
| `-fft <size>` | int | 2048 | FFT size (power of 2) |
| `-hop <size>` | int | 512 | Hop size in samples |
| `-window <type>` | string | hann | Window type: `hann`, `hamming`, `blackman` |

#### Mel-spectrogram

| Option | Type | Default | Description |
|--------|------|--------|-------------|
| `-mel <bands>` | int | 128 | Number of mel bands |
| `-fmin <hz>` | float | 0 | Minimum frequency for mel scale |
| `-fmax <hz>` | float | sr/2 | Maximum frequency for mel scale |

#### Image - Dimensions

| Option | Type | Default | Description |
|--------|------|--------|-------------|
| `-scale <factor>` | float | 1.0 | Global scale factor |
| `-hscale <factor>` | float | 1.0 | Horizontal scale (time) |
| `-vscale <factor>` | float | 1.0 | Vertical scale (frequency) |

**Automatic dimension calculation**:
- Base width: `n_frames = floor((duration × sr - fft_size) / hop_size) + 1`
- Base height: `mel_bands`
- Final dimensions: `width = n_frames × hscale × scale`, `height = mel_bands × vscale × scale`

#### Image - Appearance

| Option | Type | Default | Description |
|--------|------|--------|-------------|
| `-o <file>` | string | auto | Output filename (see Naming section) |
| `-cmap <type>` | string | viridis | Colormap: `viridis`, `magma`, `hot`, `gray` |
| `-layout <preset>` | string | full | Layout preset: `full`, `minimal`, `scientific`, `raw` |

#### Image - Visual Elements (fine control)

| Option | Description |
|--------|-------------|
| `-colorbar` / `-no-colorbar` | Show/hide color bar (default: shown) |
| `-title` / `-no-title` | Show/hide title (default: shown) |
| `-axes` / `-no-axes` | Show/hide axes (default: shown) |
| `-legend` / `-no-legend` | Show/hide metadata (default: shown) |
| `-gate-line` / `-no-gate-line` | Show/hide gate transition line (default: shown) |

#### Image - Gate Line Style

| Option | Type | Default | Description |
|--------|------|--------|-------------|
| `-gate-color <color>` | string | red | Color: `red`, `white`, `yellow`, `cyan` |
| `-gate-style <style>` | string | dashed | Style: `solid`, `dashed`, `dotted` |

#### Amplitude

| Option | Type | Default | Description |
|--------|------|--------|-------------|
| `-db` | flag | false | Display in decibels instead of linear amplitude |
| `-dbmin <val>` | float | -80 | Minimum dB value for visualization |

## Layout Presets

### `full` (default)
All elements displayed:
- Title with DSP name and main parameters
- Spectrogram with colormap
- Time and frequency axes with graduations
- Gate transition line
- Colorbar with amplitude scale
- Legend with complete metadata

### `minimal`
Essential elements:
- Spectrogram
- Axes with graduations
- Gate line
- No decorative title
- No colorbar
- No footer metadata

### `scientific`
For publications:
- Spectrogram
- Axes with clear graduations
- Colorbar with units
- Subtle gate line (solid style, white color)
- No decorative title
- Technical metadata in footer

### `raw`
Pure spectrogram:
- Only the spectrogram (raw pixels)
- No annotations
- No axes

## Output File Naming

### Automatic Name
If the `-o` option is not specified, the name is automatically generated:

```
<basename>-YYYYMMDD-HHMMSS.png
```

Where:
- `<basename>`: DSP filename without extension (e.g., `foo` for `foo.dsp`)
- `YYYYMMDD`: date in year-month-day format
- `HHMMSS`: time in hour-minute-second format

**Example**:
```bash
./foo 2 0.5 440 0.9
# Generates: foo-20260111-143022.png
```

### Custom Name
The `-o` option allows specifying an explicit name:

```bash
./foo 2 0.5 440 0.9 -o my_analysis.png
# Generates: my_analysis.png
```

## Image Layout (`full` preset)

```
┌──────────────────────────────────────────────────────────┐
│ Title: foo.dsp - 440Hz, gain=0.9                        │
├──────────────────────────────────────────────┬───────────┤
│ ↑                                            │           │
│ │                                            │           │
│ f│        [Mel Spectrogram]                 │ Colorbar  │
│ r│                        │← Gate line      │           │
│ e│                        │   (t=0.5s)      │  [scale]  │
│ q│                                            │           │
│ │                                            │           │
│ ↓                                            │           │
├──────────────────────────────────────────────┴───────────┤
│         0s        0.5s        1.0s        2.0s           │
│                   ↑ gate off                             │
├──────────────────────────────────────────────────────────┤
│ Metadata:                                                │
│ SR: 44100Hz | FFT: 2048 | Hop: 512 | Window: Hann       │
│ Mel bands: 128 | fmin: 0Hz | fmax: 22050Hz              │
│ Date: 2026-01-11 14:30:22                               │
└──────────────────────────────────────────────────────────┘
```

### Detailed Elements

#### 1. Title (header)
- DSP filename (without extension)
- Main parameters: frequency and gain
- Format: `<basename>.dsp - <freq>Hz, gain=<gain>`

#### 2. Spectrogram
- Mel-spectrogram visualization with colormap
- Time on X axis (left → right)
- Frequency on Y axis (bottom → top)

#### 3. Gate transition line
- Vertical line marking `gate_duration`
- Default style: red, dashed
- Optional label: "gate off" or arrow

#### 4. Time axis (X)
- Graduations in seconds
- Special marker at `gate_duration`
- Displayed values: 0s, ..., duration

#### 5. Frequency axis (Y)
- Graduations in Hz
- Mel scale (non-linear)
- A few key values displayed

#### 6. Colorbar
- Vertical color bar on the right
- Amplitude scale (linear or dB)
- Min/max labels

#### 7. Metadata (footer)
Complete technical information:
- Sample rate
- FFT parameters (size, hop, window)
- Mel parameters (number of bands, fmin, fmax)
- Generation timestamp

## PNG Metadata

The following metadata is written to the PNG file (tEXt/iTXt chunks):

| Key | Value |
|-----|--------|
| `Software` | `Faust Spectrogram Generator` |
| `Title` | DSP name |
| `Description` | Complete parameter description |
| `Author` | Optional |
| `Creation Time` | ISO 8601 timestamp |
| `faust:dsp` | DSP filename |
| `faust:duration` | Total duration (s) |
| `faust:gate_duration` | Gate duration (s) |
| `faust:frequency` | Frequency (Hz) |
| `faust:gain` | Gain |
| `audio:sample_rate` | Sample rate |
| `fft:size` | FFT size |
| `fft:hop` | Hop size |
| `fft:window` | Window type |
| `mel:bands` | Number of mel bands |
| `mel:fmin` | Min frequency (Hz) |
| `mel:fmax` | Max frequency (Hz) |

## Usage Examples

### Example 1: Basic usage
```bash
./foo 2 0.5 440 0.9
```
- Duration: 2s
- Gate=1 for 0.5s, then Gate=0
- Frequency: 440 Hz
- Gain: 0.9
- Image: `foo-20260111-143022.png` with full layout

### Example 2: High resolution
```bash
./foo 5 1.0 880 0.8 -sr 48000 -fft 4096 -mel 256 -scale 2.0
```
- 5s sound, gate for 1s
- SR 48kHz, FFT 4096
- 256 mel bands
- Image 2× larger

### Example 3: Scientific publication
```bash
./foo 2 0.5 440 0.9 -layout scientific -gate-color white -o figure1.png
```
- Clean layout for publication
- White gate line (subtle)
- Custom filename

### Example 4: Raw export
```bash
./foo 2 0.5 440 0.9 -layout raw -scale 4.0
```
- Only the spectrogram (no annotations)
- Image enlarged 4× for post-processing

### Example 5: Detailed analysis
```bash
./foo 10 2.0 220 0.7 -mel 256 -fft 4096 -hop 256 -db -dbmin -100 \
     -hscale 2.0 -vscale 1.5 -cmap magma -o detailed_analysis.png
```
- Long sound (10s) with fine analysis
- dB scale with -100dB threshold
- Image stretched horizontally
- Magma colormap

### Example 6: Full customization
```bash
./foo 3 0.8 440 0.9 -no-title -no-legend -colorbar -axes \
     -gate-color cyan -gate-style solid -cmap hot
```
- Fine control of visual elements
- Continuous cyan gate line
- "hot" colormap

## Technical Dependencies

### Required Libraries
- **libpng**: PNG export
- **FFTW3** or **KissFFT**: FFT calculation
- **C++ standard library**: CLI parsing, calculations

### Suggested Compilation
```bash
g++ foo.cpp -o foo -lfftw3 -lpng -lm -std=c++17
```

## Implementation Notes

### UI Class Implementation

The architecture must provide a class inheriting from `UI` to capture the parameters:

```cpp
class ParameterCollector : public UI {
    struct Param {
        FAUSTFLOAT* zone;
        FAUSTFLOAT min, max, init;
        bool found = false;
    };
    
    std::map<std::string, Param> params;
    
public:
    void addButton(const char* label, FAUSTFLOAT* zone) override {
        if (strcmp(label, "gate") == 0) {
            params["gate"] = {zone, 0, 1, 0, true};
        }
    }
    
    void addNumEntry(const char* label, FAUSTFLOAT* zone, 
                     FAUSTFLOAT init, FAUSTFLOAT min, 
                     FAUSTFLOAT max, FAUSTFLOAT step) override {
        if (strcmp(label, "freq") == 0 || strcmp(label, "gain") == 0) {
            params[label] = {zone, min, max, init, true};
        }
    }
    
    // Same for addHorizontalSlider, addVerticalSlider, addCheckButton
    
    bool validate() {
        return params["gate"].found && 
               params["freq"].found && 
               params["gain"].found;
    }
    
    void setParameter(const std::string& name, FAUSTFLOAT value) {
        auto& p = params[name];
        FAUSTFLOAT clamped = std::clamp(value, p.min, p.max);
        
        if (clamped != value && name != "gate") {
            std::cerr << "Warning: " << name << "=" << value 
                     << " exceeds range [" << p.min << ", " << p.max 
                     << "], clamped to " << clamped << std::endl;
        }
        
        *p.zone = clamped;
    }
};
```

**Validation Steps**:
1. Call `dsp->buildUserInterface(&collector)`
2. Verify that the 3 required labels have been found
3. If a label is missing, display an explicit error and exit
4. Store the ranges for subsequent clamping

**Validation Error Messages**:
```bash
# If a label is missing
Error: DSP must expose parameter "freq"
Expected widget: nentry, hslider, or vslider with label "freq"

# If wrong type is used for gate
Error: Parameter "gate" must be a button or checkbox

# If all labels are missing
Error: DSP must expose exactly 3 parameters: "gate", "freq", "gain"
Found: height, volume
```

### Mel Spectrogram Calculation
1. Generate audio with Faust (real-time synthesis)
2. Apply STFT (Short-Time Fourier Transform)
   - Windowing with hop size
   - FFT of each frame
3. Convert to mel scale
   - Mel triangular filter bank
   - Projection of FFT onto mel bands
4. Optional: dB conversion
5. Normalization for visualization

### PNG Rendering
- Use of libpng for writing
- Colormap mapping: value → RGB
- Bilinear interpolation for scaling
- Addition of annotations (text, lines) with vector rendering

### Command-Line Parsing
- Use of getopt or argparse-like
- Type and value range validation
- Explicit error messages

## Possible Future Extensions

- MIDI file support to drive parameters
- Multi-format export (SVG, PDF)
- GIF animation to visualize temporal evolution
- Spectrogram comparison (visual diff)
- Support for more than 3 Faust parameters
- Batch mode: process multiple configurations

---

**Version**: 1.0
**Date**: 2026-01-11
**Author**: Collaborative specification
