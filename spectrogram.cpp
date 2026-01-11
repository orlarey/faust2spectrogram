/************************************************************************
 IMPORTANT NOTE : this file contains two clearly delimited sections :
 the ARCHITECTURE section (in two parts) and the USER section. Each section
 is governed by its own copyright and license. Please check individually
 each section for license and copyright information.
 *************************************************************************/

/******************* BEGIN spectrogram.cpp ****************/
/************************************************************************
 FAUST Architecture File - Spectrogram Generator
 Copyright (C) 2026 Yann Orlarey
 ---------------------------------------------------------------------
 This Architecture section is free software; you can redistribute it
 and/or modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 3 of
 the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; If not, see <http://www.gnu.org/licenses/>.

 EXCEPTION : As a special exception, you may create a larger work
 that contains this FAUST architecture section and distribute
 that work under terms of your choice, so long as this FAUST
 architecture section is not modified.

 ************************************************************************
 ************************************************************************/

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>
#include <ctime>
#include <fftw3.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <png.h>
#include <sstream>
#include <string>
#include <vector>

#ifndef FAUSTFLOAT
#define FAUSTFLOAT float
#endif

#ifndef FAUSTCLASS
#define FAUSTCLASS mydsp
#endif

#ifdef __APPLE__
#define exp10f __exp10f
#define exp10 __exp10
#endif

#if defined(_WIN32)
#define RESTRICT __restrict
#else
#define RESTRICT __restrict__
#endif

//==============================================================================
// Minimal Faust definitions (from plotarch_header.cpp)
//==============================================================================

// Dummy Soundfile for compatibility
struct Soundfile {
  void *fBuffers;
  int *fLength;
  int *fSR;
  int *fOffset;
  int fChannels;
  int fParts;
  bool fIsDouble;
};

// Abstract UI class
class UI {
public:
  virtual ~UI() {}
  virtual void openTabBox(const char *label) = 0;
  virtual void openHorizontalBox(const char *label) = 0;
  virtual void openVerticalBox(const char *label) = 0;
  virtual void closeBox() = 0;
  virtual void addButton(const char *label, FAUSTFLOAT *zone) = 0;
  virtual void addCheckButton(const char *label, FAUSTFLOAT *zone) = 0;
  virtual void addVerticalSlider(const char *label, FAUSTFLOAT *zone,
                                 FAUSTFLOAT init, FAUSTFLOAT min,
                                 FAUSTFLOAT max, FAUSTFLOAT step) = 0;
  virtual void addHorizontalSlider(const char *label, FAUSTFLOAT *zone,
                                   FAUSTFLOAT init, FAUSTFLOAT min,
                                   FAUSTFLOAT max, FAUSTFLOAT step) = 0;
  virtual void addNumEntry(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init,
                           FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) = 0;
  virtual void addHorizontalBargraph(const char *label, FAUSTFLOAT *zone,
                                     FAUSTFLOAT min, FAUSTFLOAT max) = 0;
  virtual void addVerticalBargraph(const char *label, FAUSTFLOAT *zone,
                                   FAUSTFLOAT min, FAUSTFLOAT max) = 0;
  virtual void addSoundfile(const char *label, const char *filename,
                            Soundfile **sf_zone) = 0;
  virtual void declare(FAUSTFLOAT *zone, const char *key,
                       const char *value) = 0;
};

// Abstract Meta class
class Meta {
public:
  virtual ~Meta() {}
  virtual void declare(const char *key, const char *value) = 0;
};

// Abstract DSP class
class dsp {
public:
  virtual ~dsp() {}
  virtual void buildUserInterface(UI *ui_interface) = 0;
  virtual void compute(int count, FAUSTFLOAT **inputs,
                       FAUSTFLOAT **outputs) = 0;
  virtual void init(int samplingFreq) = 0;
  virtual void instanceClear() = 0;
  virtual void instanceConstants(int samplingFreq) = 0;
  virtual void instanceInit(int samplingFreq) = 0;
  virtual void instanceResetUserInterface() = 0;
  virtual int getNumInputs() = 0;
  virtual int getNumOutputs() = 0;
  virtual int getSampleRate() = 0;
  virtual dsp *clone() = 0;
};

/******************************************************************************
 *******************************************************************************

 VECTOR INTRINSICS

 *******************************************************************************
 *******************************************************************************/

<< includeIntrinsic >>

    /********************END ARCHITECTURE SECTION (part 1/2)****************/

    /**************************BEGIN USER SECTION **************************/

    << includeclass >>

    /***************************END USER SECTION ***************************/

    /*******************BEGIN ARCHITECTURE SECTION (part 2/2)***************/

    //==============================================================================
    // Parameter Collector UI
    //==============================================================================

    class SpectrogramUI : public UI {
public:
  struct Parameter {
    FAUSTFLOAT *zone;
    FAUSTFLOAT min;
    FAUSTFLOAT max;
    FAUSTFLOAT init;
    std::string type;
    bool found;

    Parameter()
        : zone(nullptr), min(0), max(1), init(0), type(""), found(false) {}
  };

private:
  std::map<std::string, Parameter> params_;

public:
  SpectrogramUI() {}

  // UI interface implementation
  virtual void openTabBox(const char *label) {}
  virtual void openHorizontalBox(const char *label) {}
  virtual void openVerticalBox(const char *label) {}
  virtual void closeBox() {}

  virtual void declare(FAUSTFLOAT *zone, const char *key, const char *value) {}

  virtual void addButton(const char *label, FAUSTFLOAT *zone) {
    if (strcmp(label, "gate") == 0) {
      params_["gate"].zone = zone;
      params_["gate"].min = 0;
      params_["gate"].max = 1;
      params_["gate"].init = 0;
      params_["gate"].type = "button";
      params_["gate"].found = true;
    }
  }

  virtual void addCheckButton(const char *label, FAUSTFLOAT *zone) {
    if (strcmp(label, "gate") == 0) {
      params_["gate"].zone = zone;
      params_["gate"].min = 0;
      params_["gate"].max = 1;
      params_["gate"].init = 0;
      params_["gate"].type = "checkbox";
      params_["gate"].found = true;
    }
  }

  virtual void addVerticalSlider(const char *label, FAUSTFLOAT *zone,
                                 FAUSTFLOAT init, FAUSTFLOAT min,
                                 FAUSTFLOAT max, FAUSTFLOAT step) {
    std::string lbl(label);
    if (lbl == "freq" || lbl == "gain") {
      params_[lbl].zone = zone;
      params_[lbl].min = min;
      params_[lbl].max = max;
      params_[lbl].init = init;
      params_[lbl].type = "vslider";
      params_[lbl].found = true;
    }
  }

  virtual void addHorizontalSlider(const char *label, FAUSTFLOAT *zone,
                                   FAUSTFLOAT init, FAUSTFLOAT min,
                                   FAUSTFLOAT max, FAUSTFLOAT step) {
    std::string lbl(label);
    if (lbl == "freq" || lbl == "gain") {
      params_[lbl].zone = zone;
      params_[lbl].min = min;
      params_[lbl].max = max;
      params_[lbl].init = init;
      params_[lbl].type = "hslider";
      params_[lbl].found = true;
    }
  }

  virtual void addNumEntry(const char *label, FAUSTFLOAT *zone, FAUSTFLOAT init,
                           FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step) {
    std::string lbl(label);
    if (lbl == "freq" || lbl == "gain") {
      params_[lbl].zone = zone;
      params_[lbl].min = min;
      params_[lbl].max = max;
      params_[lbl].init = init;
      params_[lbl].type = "nentry";
      params_[lbl].found = true;
    }
  }

  virtual void addHorizontalBargraph(const char *label, FAUSTFLOAT *zone,
                                     FAUSTFLOAT min, FAUSTFLOAT max) {}
  virtual void addVerticalBargraph(const char *label, FAUSTFLOAT *zone,
                                   FAUSTFLOAT min, FAUSTFLOAT max) {}

  virtual void addSoundfile(const char *label, const char *filename,
                            Soundfile **sf_zone) {}

  // Validation
  bool validate(std::string &error_msg) {
    if (!params_["gate"].found) {
      error_msg = "Error: DSP must expose parameter \"gate\"\n"
                  "Expected widget: button or checkbox with label \"gate\"";
      return false;
    }

    if (!params_["freq"].found) {
      error_msg =
          "Error: DSP must expose parameter \"freq\"\n"
          "Expected widget: nentry, hslider, or vslider with label \"freq\"";
      return false;
    }

    if (!params_["gain"].found) {
      error_msg =
          "Error: DSP must expose parameter \"gain\"\n"
          "Expected widget: nentry, hslider, or vslider with label \"gain\"";
      return false;
    }

    return true;
  }

  // Set parameter with clamping
  void setParameter(const std::string &name, FAUSTFLOAT value) {
    if (params_.find(name) == params_.end() || !params_[name].found) {
      return;
    }

    Parameter &p = params_[name];
    FAUSTFLOAT clamped = std::max(p.min, std::min(value, p.max));

    if (clamped != value && name != "gate") {
      std::cerr << "Warning: " << name << "=" << value << " exceeds range ["
                << p.min << ", " << p.max << "], clamped to " << clamped
                << std::endl;
    }

    *p.zone = clamped;
  }

  // Get parameter info
  const Parameter &getParameter(const std::string &name) const {
    static Parameter dummy;
    auto it = params_.find(name);
    return (it != params_.end()) ? it->second : dummy;
  }
};

//==============================================================================
// Command Line Options
//==============================================================================

struct Options {
  // Positional arguments
  float duration;
  float gate_duration;
  float frequency;
  float gain;

  // Audio options
  int sample_rate;

  // FFT options
  int fft_size;
  int hop_size;
  std::string window_type;

  // Mel options
  int mel_bands;
  float fmin;
  float fmax;

  // Image options
  std::string output_file;
  float scale;
  float hscale;
  float vscale;
  std::string colormap;
  std::string layout;

  // Visual elements
  bool colorbar;
  bool title;
  bool axes;
  bool legend;
  bool gate_line;
  std::string gate_color;
  std::string gate_style;

  // Amplitude
  bool use_db;
  float db_min;

  // Constructor with defaults
  Options()
      : duration(0), gate_duration(0), frequency(0), gain(0),
        sample_rate(44100), fft_size(2048), hop_size(512), window_type("hann"),
        mel_bands(128), fmin(0), fmax(-1), output_file(""), scale(1.0),
        hscale(1.0), vscale(1.0), colormap("hot"), layout("full"),
        colorbar(true), title(true), axes(true), legend(true), gate_line(true),
        gate_color("red"), gate_style("dashed"), use_db(false), db_min(-80.0) {}
};

//==============================================================================
// Command Line Parser
//==============================================================================

void printUsage(const char *program_name) {
  std::cerr << "Usage: " << program_name
            << " [OPTIONS] <duration> <gate_duration> <frequency> <gain>\n\n";
  std::cerr << "Positional arguments:\n";
  std::cerr << "  duration        Total duration in seconds\n";
  std::cerr << "  gate_duration   Gate=1 duration in seconds (from start)\n";
  std::cerr << "  frequency       Frequency in Hz\n";
  std::cerr << "  gain            Gain value\n\n";
  std::cerr << "Audio options:\n";
  std::cerr << "  -sr <rate>      Sample rate (default: 44100)\n\n";
  std::cerr << "FFT options:\n";
  std::cerr << "  -fft <size>     FFT size (default: 2048)\n";
  std::cerr << "  -hop <size>     Hop size (default: 512)\n";
  std::cerr << "  -window <type>  Window type: hann|hamming|blackman (default: "
               "hann)\n\n";
  std::cerr << "Mel options:\n";
  std::cerr << "  -mel <bands>    Number of mel bands (default: 128)\n";
  std::cerr << "  -fmin <hz>      Min frequency for mel scale (default: 0)\n";
  std::cerr
      << "  -fmax <hz>      Max frequency for mel scale (default: sr/2)\n\n";
  std::cerr << "Image options:\n";
  std::cerr << "  -o <file>       Output file (default: auto-generated)\n";
  std::cerr << "  -scale <f>      Global scale factor (default: 1.0)\n";
  std::cerr << "  -hscale <f>     Horizontal scale (default: 1.0)\n";
  std::cerr << "  -vscale <f>     Vertical scale (default: 1.0)\n";
  std::cerr << "  -cmap <type>    Colormap: viridis|magma|hot|gray (default: "
               "hot)\n";
  std::cerr << "  -layout <type>  Layout preset: full|minimal|scientific|raw "
               "(default: full)\n\n";
  std::cerr << "Visual elements:\n";
  std::cerr
      << "  -colorbar / -no-colorbar     Show/hide colorbar (default: show)\n";
  std::cerr
      << "  -title / -no-title           Show/hide title (default: show)\n";
  std::cerr
      << "  -axes / -no-axes             Show/hide axes (default: show)\n";
  std::cerr
      << "  -legend / -no-legend         Show/hide legend (default: show)\n";
  std::cerr
      << "  -gate-line / -no-gate-line   Show/hide gate line (default: show)\n";
  std::cerr << "  -gate-color <color>          Gate line color: "
               "red|white|yellow|cyan (default: red)\n";
  std::cerr << "  -gate-style <style>          Gate line style: "
               "solid|dashed|dotted (default: dashed)\n\n";
  std::cerr << "Amplitude:\n";
  std::cerr << "  -db             Display in decibels\n";
  std::cerr << "  -dbmin <val>    Minimum dB value (default: -80)\n\n";
}

bool parseCommandLine(int argc, char *argv[], Options &opts) {
  if (argc < 5) {
    printUsage(argv[0]);
    return false;
  }

  int pos_arg_index = 0;

  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];

    // Check for options
    if (arg[0] == '-') {
      if (arg == "-sr" && i + 1 < argc) {
        opts.sample_rate = atoi(argv[++i]);
      } else if (arg == "-fft" && i + 1 < argc) {
        opts.fft_size = atoi(argv[++i]);
      } else if (arg == "-hop" && i + 1 < argc) {
        opts.hop_size = atoi(argv[++i]);
      } else if (arg == "-window" && i + 1 < argc) {
        opts.window_type = argv[++i];
      } else if (arg == "-mel" && i + 1 < argc) {
        opts.mel_bands = atoi(argv[++i]);
      } else if (arg == "-fmin" && i + 1 < argc) {
        opts.fmin = atof(argv[++i]);
      } else if (arg == "-fmax" && i + 1 < argc) {
        opts.fmax = atof(argv[++i]);
      } else if (arg == "-o" && i + 1 < argc) {
        opts.output_file = argv[++i];
      } else if (arg == "-scale" && i + 1 < argc) {
        opts.scale = atof(argv[++i]);
      } else if (arg == "-hscale" && i + 1 < argc) {
        opts.hscale = atof(argv[++i]);
      } else if (arg == "-vscale" && i + 1 < argc) {
        opts.vscale = atof(argv[++i]);
      } else if (arg == "-cmap" && i + 1 < argc) {
        opts.colormap = argv[++i];
      } else if (arg == "-layout" && i + 1 < argc) {
        opts.layout = argv[++i];
      } else if (arg == "-colorbar") {
        opts.colorbar = true;
      } else if (arg == "-no-colorbar") {
        opts.colorbar = false;
      } else if (arg == "-title") {
        opts.title = true;
      } else if (arg == "-no-title") {
        opts.title = false;
      } else if (arg == "-axes") {
        opts.axes = true;
      } else if (arg == "-no-axes") {
        opts.axes = false;
      } else if (arg == "-legend") {
        opts.legend = true;
      } else if (arg == "-no-legend") {
        opts.legend = false;
      } else if (arg == "-gate-line") {
        opts.gate_line = true;
      } else if (arg == "-no-gate-line") {
        opts.gate_line = false;
      } else if (arg == "-gate-color" && i + 1 < argc) {
        opts.gate_color = argv[++i];
      } else if (arg == "-gate-style" && i + 1 < argc) {
        opts.gate_style = argv[++i];
      } else if (arg == "-db") {
        opts.use_db = true;
      } else if (arg == "-dbmin" && i + 1 < argc) {
        opts.db_min = atof(argv[++i]);
      } else {
        std::cerr << "Unknown option: " << arg << std::endl;
        return false;
      }
    } else {
      // Positional arguments
      switch (pos_arg_index) {
      case 0:
        opts.duration = atof(argv[i]);
        break;
      case 1:
        opts.gate_duration = atof(argv[i]);
        break;
      case 2:
        opts.frequency = atof(argv[i]);
        break;
      case 3:
        opts.gain = atof(argv[i]);
        break;
      default:
        std::cerr << "Too many positional arguments" << std::endl;
        return false;
      }
      pos_arg_index++;
    }
  }

  if (pos_arg_index != 4) {
    std::cerr << "Error: Missing required positional arguments" << std::endl;
    printUsage(argv[0]);
    return false;
  }

  // Apply layout presets
  if (opts.layout == "minimal") {
    opts.title = false;
    opts.colorbar = false;
    opts.legend = false;
  } else if (opts.layout == "scientific") {
    opts.title = false;
    opts.gate_color = "white";
    opts.gate_style = "solid";
  } else if (opts.layout == "raw") {
    opts.colorbar = false;
    opts.title = false;
    opts.axes = false;
    opts.legend = false;
    opts.gate_line = false;
  }

  // Set fmax default if not specified
  if (opts.fmax < 0) {
    opts.fmax = opts.sample_rate / 2.0;
  }

  return true;
}

//==============================================================================
// Utility Functions
//==============================================================================

std::string generateTimestamp() {
  std::time_t now = std::time(nullptr);
  std::tm *ltm = std::localtime(&now);

  std::ostringstream oss;
  oss << std::setfill('0') << std::setw(4) << (1900 + ltm->tm_year)
      << std::setw(2) << (1 + ltm->tm_mon) << std::setw(2) << ltm->tm_mday
      << "-" << std::setw(2) << ltm->tm_hour << std::setw(2) << ltm->tm_min
      << std::setw(2) << ltm->tm_sec;

  return oss.str();
}

std::string generateOutputFilename(const char *program_name,
                                   const Options &opts) {
  if (!opts.output_file.empty()) {
    return opts.output_file;
  }

  // Extract basename without extension
  std::string base = program_name;
  size_t last_slash = base.find_last_of("/\\");
  if (last_slash != std::string::npos) {
    base = base.substr(last_slash + 1);
  }

  return base + "-" + generateTimestamp() + ".png";
}

//==============================================================================
// Audio Synthesis
//==============================================================================

void synthesizeAudio(mydsp &dsp, SpectrogramUI &ui, const Options &opts,
                     std::vector<float> &output) {
  int num_samples = (int)(opts.duration * opts.sample_rate);
  int gate_samples = (int)(opts.gate_duration * opts.sample_rate);

  // Allocate output buffer
  output.resize(num_samples);

  // Set frequency and gain (constant during synthesis)
  ui.setParameter("freq", opts.frequency);
  ui.setParameter("gain", opts.gain);

  // Allocate DSP buffers
  int num_outputs = dsp.getNumOutputs();
  FAUSTFLOAT **outputs = new FAUSTFLOAT *[num_outputs];
  for (int i = 0; i < num_outputs; i++) {
    outputs[i] = new FAUSTFLOAT[1]; // Single sample at a time
  }

  // Synthesis loop (sample by sample)
  for (int i = 0; i < num_samples; i++) {
    // Update gate
    FAUSTFLOAT gate_value = (i < gate_samples) ? 1.0f : 0.0f;
    ui.setParameter("gate", gate_value);

    // Compute single sample
    dsp.compute(1, nullptr, outputs);

    // Store first output channel
    output[i] = outputs[0][0];
  }

  // Cleanup
  for (int i = 0; i < num_outputs; i++) {
    delete[] outputs[i];
  }
  delete[] outputs;
}

//==============================================================================
// DSP and Signal Processing Functions
//==============================================================================

// Window functions
std::vector<float> createWindow(int size, const std::string &type) {
  std::vector<float> window(size);

  for (int i = 0; i < size; i++) {
    float x = (float)i / (size - 1);

    if (type == "hann") {
      window[i] = 0.5f * (1.0f - std::cos(2.0f * M_PI * x));
    } else if (type == "hamming") {
      window[i] = 0.54f - 0.46f * std::cos(2.0f * M_PI * x);
    } else if (type == "blackman") {
      window[i] = 0.42f - 0.5f * std::cos(2.0f * M_PI * x) +
                  0.08f * std::cos(4.0f * M_PI * x);
    } else {
      window[i] = 1.0f; // Rectangular
    }
  }

  return window;
}

// Hz to Mel conversion
float hzToMel(float hz) { return 2595.0f * std::log10(1.0f + hz / 700.0f); }

// Mel to Hz conversion
float melToHz(float mel) {
  return 700.0f * (std::pow(10.0f, mel / 2595.0f) - 1.0f);
}

// Create mel filterbank
std::vector<std::vector<float>> createMelFilterbank(int n_mels, int fft_size,
                                                    int sample_rate, float fmin,
                                                    float fmax) {

  // Convert to mel scale
  float mel_min = hzToMel(fmin);
  float mel_max = hzToMel(fmax);

  // Create mel points (n_mels + 2 for edges)
  std::vector<float> mel_points(n_mels + 2);
  for (int i = 0; i < n_mels + 2; i++) {
    mel_points[i] = mel_min + (mel_max - mel_min) * i / (n_mels + 1);
  }

  // Convert mel points to Hz then to FFT bins
  std::vector<int> bin_points(n_mels + 2);
  int n_fft_bins = fft_size / 2 + 1;
  for (int i = 0; i < n_mels + 2; i++) {
    float hz = melToHz(mel_points[i]);
    bin_points[i] = (int)std::floor((fft_size + 1) * hz / sample_rate);
  }

  // Create triangular filters
  std::vector<std::vector<float>> filterbank(n_mels);
  for (int i = 0; i < n_mels; i++) {
    filterbank[i].resize(n_fft_bins, 0.0f);

    int left = bin_points[i];
    int center = bin_points[i + 1];
    int right = bin_points[i + 2];

    // Rising slope
    for (int j = left; j < center; j++) {
      filterbank[i][j] = (float)(j - left) / (center - left);
    }

    // Falling slope
    for (int j = center; j < right; j++) {
      filterbank[i][j] = (float)(right - j) / (right - center);
    }
  }

  return filterbank;
}

// STFT computation
std::vector<std::vector<float>> computeSTFT(const std::vector<float> &audio,
                                            int fft_size, int hop_size,
                                            const std::vector<float> &window) {

  int n_frames = (audio.size() - fft_size) / hop_size + 1;
  int n_bins = fft_size / 2 + 1;

  std::vector<std::vector<float>> spectrogram(n_frames);

  // Allocate FFTW structures
  float *in = (float *)fftwf_malloc(sizeof(float) * fft_size);
  fftwf_complex *out =
      (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * n_bins);
  fftwf_plan plan = fftwf_plan_dft_r2c_1d(fft_size, in, out, FFTW_ESTIMATE);

  for (int frame = 0; frame < n_frames; frame++) {
    spectrogram[frame].resize(n_bins);

    int offset = frame * hop_size;

    // Apply window and copy to FFT input
    for (int i = 0; i < fft_size; i++) {
      in[i] = audio[offset + i] * window[i];
    }

    // Execute FFT
    fftwf_execute(plan);

    // Compute magnitude spectrum
    for (int i = 0; i < n_bins; i++) {
      float real = out[i][0];
      float imag = out[i][1];
      spectrogram[frame][i] = std::sqrt(real * real + imag * imag);
    }
  }

  // Cleanup
  fftwf_destroy_plan(plan);
  fftwf_free(in);
  fftwf_free(out);

  return spectrogram;
}

// Apply mel filterbank to spectrogram
std::vector<std::vector<float>>
applyMelFilterbank(const std::vector<std::vector<float>> &spectrogram,
                   const std::vector<std::vector<float>> &filterbank) {

  int n_frames = spectrogram.size();
  int n_mels = filterbank.size();

  std::vector<std::vector<float>> mel_spec(n_frames);

  for (int frame = 0; frame < n_frames; frame++) {
    mel_spec[frame].resize(n_mels);

    for (int mel = 0; mel < n_mels; mel++) {
      float sum = 0.0f;
      for (size_t bin = 0; bin < spectrogram[frame].size(); bin++) {
        sum += spectrogram[frame][bin] * filterbank[mel][bin];
      }
      mel_spec[frame][mel] = sum;
    }
  }

  return mel_spec;
}

// Convert to dB scale
void convertToDb(std::vector<std::vector<float>> &mel_spec, float db_min) {
  for (auto &frame : mel_spec) {
    for (auto &val : frame) {
      if (val > 0) {
        val = 20.0f * std::log10(val);
        val = std::max(val, db_min);
      } else {
        val = db_min;
      }
    }
  }
}

// Normalize spectrogram to [0, 1]
void normalizeSpectrogram(std::vector<std::vector<float>> &mel_spec) {
  float min_val = 1e10f;
  float max_val = -1e10f;

  for (const auto &frame : mel_spec) {
    for (float val : frame) {
      min_val = std::min(min_val, val);
      max_val = std::max(max_val, val);
    }
  }

  float range = max_val - min_val;
  if (range > 0) {
    for (auto &frame : mel_spec) {
      for (auto &val : frame) {
        val = (val - min_val) / range;
      }
    }
  }
}

//==============================================================================
// Colormap Functions
//==============================================================================

struct RGB {
  unsigned char r, g, b;
};

RGB applyColormap(float value, const std::string &colormap) {
  // Clamp value to [0, 1]
  value = std::max(0.0f, std::min(1.0f, value));

  RGB color;

  if (colormap == "viridis") {
    // Simplified viridis approximation
    if (value < 0.25f) {
      float t = value / 0.25f;
      color.r = (unsigned char)(68 * (1 - t) + 59 * t);
      color.g = (unsigned char)(1 * (1 - t) + 82 * t);
      color.b = (unsigned char)(84 * (1 - t) + 139 * t);
    } else if (value < 0.5f) {
      float t = (value - 0.25f) / 0.25f;
      color.r = (unsigned char)(59 * (1 - t) + 33 * t);
      color.g = (unsigned char)(82 * (1 - t) + 145 * t);
      color.b = (unsigned char)(139 * (1 - t) + 140 * t);
    } else if (value < 0.75f) {
      float t = (value - 0.5f) / 0.25f;
      color.r = (unsigned char)(33 * (1 - t) + 94 * t);
      color.g = (unsigned char)(145 * (1 - t) + 201 * t);
      color.b = (unsigned char)(140 * (1 - t) + 98 * t);
    } else {
      float t = (value - 0.75f) / 0.25f;
      color.r = (unsigned char)(94 * (1 - t) + 253 * t);
      color.g = (unsigned char)(201 * (1 - t) + 231 * t);
      color.b = (unsigned char)(98 * (1 - t) + 37 * t);
    }
  } else if (colormap == "magma") {
    // Simplified magma
    color.r = (unsigned char)(value * 252);
    color.g = (unsigned char)(value * value * 180);
    color.b = (unsigned char)(std::pow(value, 0.5f) * 200);
  } else if (colormap == "hot") {
    // Hot colormap
    if (value < 0.33f) {
      color.r = (unsigned char)(value / 0.33f * 255);
      color.g = 0;
      color.b = 0;
    } else if (value < 0.66f) {
      color.r = 255;
      color.g = (unsigned char)((value - 0.33f) / 0.33f * 255);
      color.b = 0;
    } else {
      color.r = 255;
      color.g = 255;
      color.b = (unsigned char)((value - 0.66f) / 0.34f * 255);
    }
  } else if (colormap == "gray") {
    // Grayscale
    unsigned char gray = (unsigned char)(value * 255);
    color.r = gray;
    color.g = gray;
    color.b = gray;
  } else {
    // Default to hot
    return applyColormap(value, "hot");
  }

  return color;
}

//==============================================================================
// PNG Generation
//==============================================================================

bool writePNG(const std::string &filename,
              const std::vector<std::vector<float>> &mel_spec,
              const Options &opts, float gate_time) {

  int n_frames = mel_spec.size();
  int n_mels = mel_spec[0].size();

  // Apply scaling
  int width = (int)(n_frames * opts.hscale * opts.scale);
  int height = (int)(n_mels * opts.vscale * opts.scale);

  // Simple check
  if (width <= 0 || height <= 0) {
    std::cerr << "Error: Invalid image dimensions" << std::endl;
    return false;
  }

  // Create image buffer
  std::vector<std::vector<RGB>> image(height);
  for (int y = 0; y < height; y++) {
    image[y].resize(width);
  }

  // Fill image with nearest-neighbor interpolation (simple)
  for (int y = 0; y < height; y++) {
    int mel_idx = (int)((height - 1 - y) * n_mels / height);
    mel_idx = std::min(mel_idx, n_mels - 1);

    for (int x = 0; x < width; x++) {
      int frame_idx = (int)(x * n_frames / width);
      frame_idx = std::min(frame_idx, n_frames - 1);

      float value = mel_spec[frame_idx][mel_idx];
      image[y][x] = applyColormap(value, opts.colormap);
    }
  }

  // Write PNG file
  FILE *fp = fopen(filename.c_str(), "wb");
  if (!fp) {
    std::cerr << "Error: Could not open file " << filename << std::endl;
    return false;
  }

  png_structp png =
      png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png) {
    fclose(fp);
    return false;
  }

  png_infop info = png_create_info_struct(png);
  if (!info) {
    png_destroy_write_struct(&png, NULL);
    fclose(fp);
    return false;
  }

  if (setjmp(png_jmpbuf(png))) {
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return false;
  }

  png_init_io(png, fp);

  // Set image attributes
  png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
               PNG_FILTER_TYPE_DEFAULT);

  png_write_info(png, info);

  // Write image data
  std::vector<png_byte *> row_pointers(height);
  for (int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte *)&image[y][0];
  }

  png_write_image(png, &row_pointers[0]);
  png_write_end(png, NULL);

  // Cleanup
  png_destroy_write_struct(&png, &info);
  fclose(fp);

  return true;
}

//==============================================================================
// Spectrogram Generation
//==============================================================================

void generateSpectrogram(const std::vector<float> &audio, const Options &opts,
                         const std::string &output_file) {
  std::cout << "Generating spectrogram..." << std::endl;
  std::cout << "  Audio samples: " << audio.size() << std::endl;
  std::cout << "  FFT size: " << opts.fft_size << std::endl;
  std::cout << "  Hop size: " << opts.hop_size << std::endl;
  std::cout << "  Mel bands: " << opts.mel_bands << std::endl;

  // Create window
  std::vector<float> window = createWindow(opts.fft_size, opts.window_type);

  // Compute STFT
  std::cout << "  Computing STFT..." << std::endl;
  auto spectrogram = computeSTFT(audio, opts.fft_size, opts.hop_size, window);

  // Create mel filterbank
  std::cout << "  Creating mel filterbank..." << std::endl;
  auto filterbank = createMelFilterbank(opts.mel_bands, opts.fft_size,
                                        opts.sample_rate, opts.fmin, opts.fmax);

  // Apply mel filterbank
  std::cout << "  Applying mel filterbank..." << std::endl;
  auto mel_spec = applyMelFilterbank(spectrogram, filterbank);

  // Convert to dB if requested
  if (opts.use_db) {
    std::cout << "  Converting to dB scale..." << std::endl;
    convertToDb(mel_spec, opts.db_min);
  }

  // Normalize to [0, 1]
  std::cout << "  Normalizing..." << std::endl;
  normalizeSpectrogram(mel_spec);

  // Calculate gate time in frames
  float gate_time = opts.gate_duration;

  // Write PNG
  std::cout << "  Writing PNG: " << output_file << std::endl;
  if (writePNG(output_file, mel_spec, opts, gate_time)) {
    std::cout << "✓ Spectrogram saved to: " << output_file << std::endl;
  } else {
    std::cerr << "✗ Failed to write PNG" << std::endl;
  }
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char *argv[]) {
  // Parse command line
  Options opts;
  if (!parseCommandLine(argc, argv, opts)) {
    return 1;
  }

  // Create DSP instance
  mydsp *dsp = new mydsp();
  if (dsp == nullptr) {
    std::cerr << "Failed to create DSP object" << std::endl;
    return 1;
  }

  // Build UI and validate DSP parameters
  SpectrogramUI ui;
  dsp->buildUserInterface(&ui);

  std::string error_msg;
  if (!ui.validate(error_msg)) {
    std::cerr << error_msg << std::endl;
    delete dsp;
    return 1;
  }

  // Initialize DSP
  dsp->init(opts.sample_rate);

  // Display parameter info
  std::cout << "DSP Parameters:" << std::endl;
  std::cout << "  gate: " << ui.getParameter("gate").type << std::endl;
  std::cout << "  freq: " << ui.getParameter("freq").type << " ["
            << ui.getParameter("freq").min << ", "
            << ui.getParameter("freq").max << "]" << std::endl;
  std::cout << "  gain: " << ui.getParameter("gain").type << " ["
            << ui.getParameter("gain").min << ", "
            << ui.getParameter("gain").max << "]" << std::endl;
  std::cout << std::endl;

  // Synthesize audio
  std::cout << "Synthesizing audio..." << std::endl;
  std::vector<float> audio;
  synthesizeAudio(*dsp, ui, opts, audio);
  std::cout << "  Generated " << audio.size() << " samples" << std::endl;
  std::cout << std::endl;

  // Generate output filename
  std::string output_file = generateOutputFilename(argv[0], opts);

  // Generate spectrogram
  generateSpectrogram(audio, opts, output_file);

  // Cleanup
  delete dsp;

  return 0;
}

/******************* END spectrogram.cpp ****************/
