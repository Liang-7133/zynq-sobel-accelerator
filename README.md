# zynq-sobel-accelerator

Sobel edge detection hardware accelerator on Zynq-7000 SoC (PYNQ-Z2),
implemented with Vitis HLS + AXI DMA + AXI4-Lite control.
Supports dynamic threshold adjustment, multi-resolution benchmarking,
and hardware comparison between 3x3 and 5x5 Sobel kernels.

---

## Background

Edge detection is a fundamental step in real-time vision systems such as autonomous
driving, industrial inspection, and medical imaging. Software-only implementations
on ARM cores typically achieve 10-15 FPS at 640x480, which is insufficient for
real-time applications (>=30 FPS). This project offloads the Sobel computation to
FPGA programmable logic via a custom HLS IP, achieving ~20x speedup while keeping
the control interface accessible from Python through PYNQ.

The key design challenge is the pixel streaming pipeline: each pixel must be
processed at II=1 (one pixel per clock) while maintaining a sliding window over
the 2D image neighborhood. This requires a line buffer + sliding window architecture
implemented in HLS with full array partitioning.

---

## System Architecture

```
Python (PS / ARM)
        |
        | AXI4-Lite (control: rows, cols, threshold)
        |
+-------+--------+
|  sobel_filter  |  <-- HLS IP (PL / FPGA fabric)
|  Line Buffer   |      3x3 or 5x5 Sobel kernel
|  Sliding Win   |      II=1 pipeline
+-------+--------+
        |
        | AXI Stream (pixel data)
        |
   AXI DMA 0
   /         \
in_buf      out_buf   <-- Physically contiguous buffers (pynq.allocate)
(input      (output
 image)      edges)
```

**Software/Hardware partition rationale**:
- **PL (FPGA)**: Sobel convolution — memory-bandwidth-bound, benefits from
  pixel-level parallelism and pipelining. Fixed kernel logic maps efficiently
  to LUT/FF resources.
- **PS (ARM)**: Buffer management, threshold control, timing measurement,
  visualization — control-flow-heavy tasks unsuitable for hardware.

---

## Project Highlights

- **Hardware acceleration**: 242 FPS @ 640x480 vs ~12 FPS software (~20x speedup)
- **Dynamic threshold**: AXI4-Lite register control, adjustable at runtime without re-synthesis
- **Dual kernel comparison**: 3x3 (242 FPS, 125,270 edge pixels) vs 5x5 (239 FPS, 152,914 edge pixels)
- **Multi-resolution**: 1050 FPS @ 160x120 to 248 FPS @ 640x480, linear scaling confirmed
- **Power efficiency**: measured from Vivado Power Report, energy-per-frame computed

---

## Repository Structure

```
zynq-sobel-accelerator/
├── LICENSE
├── README.md                          # Project overview + reproduction steps
├── src/
│   ├── hls/                           # Vitis HLS source
│   │   ├── sobel.h                    # 3x3 kernel header (with threshold)
│   │   ├── sobel.cpp                  # 3x3 kernel implementation
│   │   ├── sobel5.h                   # 5x5 kernel header
│   │   ├── sobel5.cpp                 # 5x5 kernel implementation
│   │   └── sobel_tb.cpp               # HLS testbench (C simulation)
│   └── ps/
│       └── sobel_main.py              # Unified PS-side driver script
├── sim/
│   ├── README.md                      # Simulation purpose + how to re-run
│   └── csim_pass.png                  # C Simulation console screenshot (PASS, 0 errors)
├── build/
│   ├── README.md                      # HLS + Vivado build steps (reproducible guide)
│   ├── hls_synthesis_report_3x3.png   # Synthesis summary for 3x3 kernel
│   └── hls_synthesis_report_5x5.png   # Synthesis summary for 5x5 kernel
├── board/
│   ├── bit/
│   │   ├── README.md                  # Bitstream-to-notebook mapping
│   │   ├── system_wrapper1.bit        # Basic Sobel (no threshold)
│   │   ├── system_wrapper1.hwh
│   │   ├── system_wrapper2.bit        # Sobel + AXI4-Lite threshold control
│   │   ├── system_wrapper2.hwh
│   │   ├── system_wrapper_3x3.bit     # 3x3 kernel for hardware comparison
│   │   ├── system_wrapper_3x3.hwh
│   │   ├── system_wrapper_5x5.bit     # 5x5 kernel for hardware comparison
│   │   └── system_wrapper_5x5.hwh
│   ├── notebooks/
│   │   ├── benchmark.ipynb            # Speedup benchmark (4 images, avg FPS)
│   │   ├── threshold_test.ipynb       # Dynamic threshold comparison
│   │   ├── resolution_benchmark.ipynb # Multi-resolution throughput
│   │   ├── sobel_3v5_hw.ipynb         # 3x3 vs 5x5 hardware comparison
│   │   └── pynq_hdmi.ipynb            # HDMI live display
│   └── output/                        # Captured result images and CSV
│       ├── benchmark.csv
│       ├── comparison.png
│       ├── dashboard.png
│       ├── kernel_compare_hw.png
│       ├── output_sobel.png
│       ├── power_efficiency.png
│       ├── resolution_benchmark.png
│       └── threshold_compare.png
├── data/
│   ├── input_images/                  # img1.png ~ img4.png (480x640 grayscale)
│   │   ├── img1.png
│   │   ├── img2.png
│   │   ├── img3.png
│   │   ├── img4.png
│   │   └── input.png
│   └── reference/                     # reference images generated on-the-fly in benchmark.ipynb
├── skill/                             # Reusable skill package
│   ├── README.md
│   ├── 01_pynq_dma_transfer.md
│   ├── 02_axilite_register_control.md
│   └── 03_hls_sobel_template.md
└── report/                            # Design report + LLM collaboration log
    ├── design_report.docx             # Full design report (Word)
    ├── design_report.pdf              # Full design report (PDF)
    └── llm_collaboration_log.md       # AI collaboration episodes, prompts, self-correction log
```

---

## Environment Requirements

| Item | Version / Spec |
|------|----------------|
| Board | PYNQ-Z2 |
| PYNQ image | v2.7 |
| Vitis HLS | 2022.2 |
| Vivado | 2022.2 |
| Python | 3.10 (PYNQ venv) |
| Key packages | pynq, numpy, Pillow, matplotlib |

---

## Reproduction Steps

### Step 1 — Upload files to the board

Connect to PYNQ-Z2 via browser (`http://<board-ip>:9090`, password: `xilinx`).

Upload to `/home/xilinx/jupyter_notebooks/pynq_sobel/`:

- All `.bit` and `.hwh` files from `board/bit/`
- All `.ipynb` files from `board/notebooks/`
- All images from `data/input_images/`

### Step 2 — Run benchmark (speedup measurement)

Open `benchmark.ipynb`. It loads `system_wrapper2.bit` automatically.

Run all cells in order. Expected output:

```
Image     SW(ms)  HW(ms)  Speedup  HW FPS
img1.png  76.31   3.86    19.79x   259.3
img2.png  79.36   3.87    20.52x   258.6
img3.png  76.41   3.86    19.79x   258.9
img4.png  76.23   3.88    19.66x   257.9
```

### Step 3 — Run dynamic threshold test

Open `threshold_test.ipynb`. It loads `system_wrapper2.bit` automatically.

Run all cells. Three output images are generated side by side:

```
threshold=5  -> 216,512 edge pixels
threshold=30 -> 125,270 edge pixels
threshold=80 ->  64,772 edge pixels
```

### Step 4 — Run multi-resolution benchmark

Open `resolution_benchmark.ipynb`. It loads `system_wrapper2.bit` automatically.

Run all cells. Expected output:

```
160x120:  ~1050 FPS,  edge pixels=10,611
320x240:   ~638 FPS,  edge pixels=36,366
640x480:   ~248 FPS,  edge pixels=125,270
```

### Step 5 — Run 3x3 vs 5x5 hardware comparison

Open `sobel_3v5_hw.ipynb`. The notebook loads both bitstreams sequentially.

- Cell block 1: loads `system_wrapper_3x3.bit`, runs 3x3 inference
- Cell block 2: loads `system_wrapper_5x5.bit`, runs 5x5 inference
- Cell block 3: side-by-side comparison plot

Expected output:

```
Metric            3x3       5x5
Latency (ms)      4.13      4.19
FPS               242.3     238.7
Edge Pixels       125,270   152,914
Latency Ratio     1.00x     1.02x
```

### Step 6 — HDMI live display (optional)

Open `pynq_hdmi.ipynb`. It loads `system_wrapper1.bit`.

Connect an HDMI display to the board. Run all cells to stream live edge-detected video.

---

## Bitstream Version Notes

See `board/bit/README.md` for the full mapping.

| Bitstream | Threshold Control | Used In |
|-----------|------------------|---------|
| system_wrapper1 | No | pynq_hdmi.ipynb |
| system_wrapper2 | Yes (AXI4-Lite offset 0x20) | benchmark.ipynb, threshold_test.ipynb, resolution_benchmark.ipynb |
| system_wrapper_3x3 | Yes | sobel_3v5_hw.ipynb |
| system_wrapper_5x5 | Yes | sobel_3v5_hw.ipynb |

---

## Key Results

| Metric | Value |
|--------|-------|
| HW latency @ 640x480 | 3.87 ms avg |
| SW latency @ 640x480 | ~77 ms avg |
| Speedup | ~20x |
| Peak throughput | 1050 FPS @ 160x120 |
| Batch throughput | ~203 FPS (20-frame avg) |
| 5x5 vs 3x3 latency overhead | +1.5% |
| 5x5 additional edge pixels | +22% |
| Total on-chip power | 1.415 W (Vivado Power Report) |

---

## LLM Collaboration

This project was developed with AI assistance throughout the design process,
including HLS implementation, AXI interface debugging, and performance analysis.

Key collaboration episodes documented in `report/llm_collaboration_log.md`:

| Episode | Problem | Outcome |
|---------|---------|---------|
| HLS threshold port | `register_map` missing `threshold` after synthesis | Identified stale IP in Block Design; re-add IP to fix |
| DMA all-zero output | Second/third transfers return zeros in loop | `np.copy()` discipline after every `recvchannel.wait()` |
| DMA channel hang | `recvchannel` never becomes idle | `ap_start` must be written before DMA transfer starts |
| IP instance name | `AttributeError: sobel_filter_0` on 5x5 overlay | Use `ip_dict.keys()` to find actual instance name |
| PYNQ state cache | `RuntimeError: Unable to parse metadata` on base overlay | Delete stale `global_pl_state.json` |

Reusable skills extracted from this process are in `skill/`.
