# Build Guide

This directory contains HLS synthesis reports for both kernel versions.
Follow the steps below to reproduce the bitstreams from source.

## Prerequisites

| Tool | Version |
|------|---------|
| Vitis HLS | 2022.2 |
| Vivado | 2022.2 |
| Target board | PYNQ-Z2 (xc7z020clg400-1) |

Source files are in `src/hls/`.

---

## Part 1 — HLS Synthesis (Vitis HLS)

Repeat for each kernel version (3x3 and 5x5).

### Step 1 — Create a new HLS project

1. Open **Vitis HLS 2022.2**
2. Click **File → New Project**
3. Set project name: `sobel_3x3` (or `sobel_5x5`)
4. Set location to any working directory
5. Click **Next**

### Step 2 — Add source files

| Kernel | Top Function | Source Files | Testbench |
|--------|-------------|--------------|-----------|
| 3x3 | `sobel_filter` | `sobel.h`, `sobel.cpp` | `sobel_tb.cpp` |
| 5x5 | `sobel_filter_5x5` | `sobel5.h`, `sobel5.cpp` | `sobel_tb.cpp` |

1. Add design files from `src/hls/` (exclude testbench here)
2. Click **Next**, add `sobel_tb.cpp` as testbench file
3. Set **Top Function** to `sobel_filter` (or `sobel_filter_5x5`)
4. Click **Next**

### Step 3 — Set part

1. Click **Part Selection**
2. Search for `xc7z020clg400-1`, select it
3. Set clock period: **10 ns** (100 MHz)
4. Click **Finish**

### Step 4 — Run C Simulation (optional verification)

1. Click **Project → Run C Simulation**
2. Verify console shows:
   ```
   [TB] *** PASS: DUT output matches reference exactly ***
   INFO: [SIM 1] CSim done with 0 errors.
   ```
   See `sim/csim_pass.png` for reference output.

### Step 5 — Run Synthesis

1. Click **Solution → Run C Synthesis → Active Solution**
2. Wait for synthesis to complete (~2-5 minutes)
3. Review the synthesis report — key targets:
   - **II = 1** on the inner pixel loop (COL_LOOP)
   - LUT usage within device limits
4. Reference screenshots: `hls_synthesis_report_3x3.png` / `hls_synthesis_report_5x5.png`

### Step 6 — Export IP

1. Click **Solution → Export RTL**
2. Format: **Vivado IP** (.zip)
3. Note the export path (default: `<project>/solution1/impl/export.zip`)

---

## Part 2 — Vivado Implementation

### Step 1 — Create Vivado project

1. Open **Vivado 2022.2**
2. Click **Create Project**, set name and location
3. Select **RTL Project**, tick "Do not specify sources at this time"
4. Part: `xc7z020clg400-1`

### Step 2 — Add HLS IP to IP catalog

1. Click **IP Catalog** in the left panel
2. Right-click → **Add Repository**
3. Navigate to the HLS export folder containing the unpacked IP
4. The `sobel_filter` (or `sobel_filter_5x5`) IP will appear in the catalog

### Step 3 — Build Block Design

1. Click **Create Block Design**, name it `system`
2. Add the following IPs by clicking the **+** button:
   - `ZYNQ7 Processing System`
   - `AXI DMA`
   - `sobel_filter` (or `sobel_filter_5x5`)
3. Click **Run Block Automation** and **Run Connection Automation** to auto-connect PS interfaces
4. Connect manually:
   - `AXI DMA M_AXIS_MM2S` → `sobel_filter src_stream`
   - `sobel_filter dst_stream` → `AXI DMA S_AXIS_S2MM`
   - `AXI DMA S_AXI_LITE` → PS `M_AXI_GP0` (via AXI Interconnect)
   - `sobel_filter s_axi_CTRL` → PS `M_AXI_GP0` (via AXI Interconnect)
5. Verify no unconnected ports remain (DRC clean)

### Step 4 — Generate bitstream

1. In Sources panel, right-click the block design → **Create HDL Wrapper**
2. Select **Let Vivado manage wrapper**
3. Click **Generate Bitstream** (Flow Navigator, bottom)
4. Wait ~10-20 minutes for synthesis + implementation + bitstream

### Step 5 — Export hardware files

1. Click **File → Export → Export Hardware**
2. Tick **Include bitstream**
3. Collect:
   - `system_wrapper.bit` — bitstream
   - `system_wrapper.hwh` — hardware handoff (generated in project folder)
4. Rename and copy to `board/bit/` following the naming convention in `board/bit/README.md`

---

## Synthesis Reports

| File | Description |
|------|-------------|
| `hls_synthesis_report_3x3.png` | Vitis HLS synthesis summary for 3x3 kernel |
| `hls_synthesis_report_5x5.png` | Vitis HLS synthesis summary for 5x5 kernel |

Key metrics from reports:

| Metric | 3x3 | 5x5 |
|--------|-----|-----|
| Target clock | 10 ns (100 MHz) | 10 ns (100 MHz) |
| Inner loop II | 1 | 1 |
| Part | xc7z020clg400-1 | xc7z020clg400-1 |
