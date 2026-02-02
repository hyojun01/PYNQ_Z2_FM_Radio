# PYNQ_Z2_FM_Radio

Hardware Acceleration of FM Demodulation with PYNQ-Z2 & RTL-SDR

## 📋 Overview

This project implements a hardware-accelerated FM radio receiver using the PYNQ-Z2 FPGA board and RTL-SDR dongle. The signal processing pipeline is implemented in hardware using Xilinx HLS (High-Level Synthesis), enabling real-time FM demodulation with high performance and low latency.

### System Architecture

```
RTL-SDR → PYNQ-Z2 (FPGA) → Audio Output
            │
            ├── FIR Decimation Filter (5x decimation)
            ├── Quadrature Demodulator (FM demod)
            ├── Low Pass Filter 1st (4x decimation)
            └── Low Pass Filter 2nd (2x decimation)
```

**Sample Rate Flow:**
- Input: 1.92 MHz (RTL-SDR)
- After FIR Decimation: 384 kHz (÷5)
- After Quadrature Demod: 384 kHz
- After LPF 1st: 96 kHz (÷4)
- After LPF 2nd: 48 kHz (÷2) → Audio Output

## 📁 Project Structure

```
PYNQ_Z2_FM_Radio/
├── hls/                          # HLS source code for signal processing blocks
│   ├── fir_decimation_filter/    # FIR filter with 5x decimation (polyphase)
│   ├── low_pass_filter_first/    # Low-pass filter with 4x decimation
│   ├── low_pass_filter_second/   # Low-pass filter with 2x decimation
│   └── quadrature_demodulator/   # FM quadrature demodulator
├── pynq/                         # PYNQ Jupyter notebook examples
│   ├── plot_test.ipynb           # Signal visualization test
│   ├── pynq_fm_test.ipynb        # RTL-SDR connection & HW/SW comparison
│   ├── pynq_fm_streaming.ipynb   # Real-time FM streaming example
│   └── pynq_fm_streaming_version_2.ipynb  # Async pipeline streaming (improved)
└── vivado/                       # Vivado project files
    ├── pynq_fm.bit               # FPGA bitstream
    ├── pynq_fm.hwh               # Hardware handoff file
    ├── pynq_fm.tcl               # Vivado TCL script
    └── tb_fm_radio_stream.v      # Verilog testbench
```

## 🔧 HLS Modules

### 1. FIR Decimation Filter
- **File:** [hls/fir_decimation_filter/fir_decimation_filter.cpp](hls/fir_decimation_filter/fir_decimation_filter.cpp)
- **Function:** Initial filtering and 5x decimation using polyphase FIR structure
- **Interface:** AXI4-Stream (8-bit input → 32-bit output)
- **Taps:** 25 taps (5 phases × 5 coefficients)

### 2. Quadrature Demodulator
- **File:** [hls/quadrature_demodulator/quadrature_demodulator.cpp](hls/quadrature_demodulator/quadrature_demodulator.cpp)
- **Function:** FM demodulation using complex signal differentiation
- **Algorithm:** `output = I * d(Q)/dt - Q * d(I)/dt`
- **Interface:** Dual AXI4-Stream input (real/imag) → Single AXI4-Stream output

### 3. Low Pass Filter (1st Stage)
- **File:** [hls/low_pass_filter_first/low_pass_filter_first.cpp](hls/low_pass_filter_first/low_pass_filter_first.cpp)
- **Function:** Audio low-pass filtering with 4x decimation
- **Interface:** AXI4-Stream (32-bit fixed-point)
- **Taps:** 44 taps (4 phases × 11 coefficients)

### 4. Low Pass Filter (2nd Stage)
- **File:** [hls/low_pass_filter_second/low_pass_filter_second.cpp](hls/low_pass_filter_second/low_pass_filter_second.cpp)
- **Function:** Final audio filtering with 2x decimation
- **Interface:** AXI4-Stream (32-bit input → 16-bit output)
- **Taps:** 44 taps (2 phases × 22 coefficients)

## 🚀 Getting Started

### Prerequisites
- **Hardware:**
  - PYNQ-Z2 Board
  - RTL-SDR USB Dongle
  - USB Audio Output (optional)

- **Software:**
  - PYNQ v2.x or later
  - Vivado/Vitis HLS (for rebuilding IP)
  - Python packages: `rtlsdr`, `numpy`, `scipy`, `matplotlib`

### Installation

1. **Clone the repository:**
   ```bash
   git clone https://github.com/hyojun01/PYNQ_Z2_FM_Radio.git
   ```

2. **Copy files to PYNQ-Z2:**
   ```bash
   scp -r pynq/* xilinx@<pynq-ip>:/home/xilinx/jupyter_notebooks/fm_radio/
   scp vivado/pynq_fm.* xilinx@<pynq-ip>:/home/xilinx/jupyter_notebooks/fm_radio/overlays/
   ```

3. **Install RTL-SDR driver on PYNQ:**
   ```bash
   sudo apt-get update
   sudo apt-get install librtlsdr-dev rtl-sdr
   echo "blacklist dvb_usb_rtl28xxu" | sudo tee /etc/modprobe.d/blacklist-rtl.conf
   sudo pip3 install pyrtlsdr
   sudo reboot
   ```

### Usage

1. Connect RTL-SDR dongle to PYNQ-Z2 USB port
2. Open Jupyter Notebook on PYNQ
3. Run `pynq_fm_streaming.ipynb` for real-time FM streaming
4. Adjust `fc` (center frequency) to tune to your local FM station

```python
# Example: Tune to 95.9 MHz
fc = 95900000  # Center frequency in Hz
fs = 1920000   # Sample rate
```

## 📊 Performance

| Metric | Value |
|--------|-------|
| Total Decimation Ratio | 40x |
| Input Sample Rate | 1.92 MHz |
| Output Audio Rate | 48 kHz |
| Fixed-Point Precision | 8-bit input, 16-bit output |
| HLS Target Clock | 100 MHz |

## � Async Pipeline Architecture (Version 2)

The `pynq_fm_streaming_version_2.ipynb` implements an efficient asynchronous pipeline using Python's `asyncio` for improved streaming performance.

### Pipeline Structure

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│   Reader    │────▶│  Processor  │────▶│   Player    │
│   (Task 1)  │     │   (Task 2)  │     │   (Task 3)  │
└─────────────┘     └─────────────┘     └─────────────┘
       │                   │                   │
   RTL-SDR            FPGA DMA            Audio Output
   Streaming          Processing          Playback
```

### Task Descriptions

| Task | Role | Description |
|------|------|-------------|
| **Reader** | Producer | Reads raw IQ data from RTL-SDR asynchronously |
| **Processor** | Transformer | Converts data format and transfers to FPGA via DMA |
| **Player** | Consumer | Plays processed audio output |

### Key Features

- **Async Queue**: Uses `asyncio.Queue` with `maxsize=3` for backpressure control
- **Double Buffering**: Queue-based architecture provides smooth data flow
- **Non-blocking I/O**: Each task runs concurrently without blocking others
- **Memory Safety**: Audio data is copied before passing between tasks

```python
# Pipeline execution example
raw_queue = asyncio.Queue(maxsize=3)
audio_queue = asyncio.Queue(maxsize=3)

task1 = asyncio.create_task(reader_task(raw_queue, ITERATION_NUM))
task2 = asyncio.create_task(processor_task(raw_queue, audio_queue))
task3 = asyncio.create_task(player_task(audio_queue))

await asyncio.gather(task1, task2, task3)
```

## �🙏 Acknowledgments

- [fm-demod-rtlsdr-pynqz2](https://github.com/hfwang132/fm-demod-rtlsdr-pynqz2) - Reference implementation
- [PYNQ Project](http://www.pynq.io/)
- [RTL-SDR](https://www.rtl-sdr.com/)
- Xilinx Vitis HLS
