#!/usr/bin/env python3
"""
sobel_main.py — PS-side main control script for Zynq-7000 Sobel Accelerator
Hardware: AMD Zynq-7000 (PYNQ-Z2)
Interface: AXI4-Stream (pixel data) + AXI4-Lite (control registers)
Overlay:   system_wrapper2.bit (Sobel IP with dynamic threshold support)

Register Map (AXI4-Lite, bundle=CTRL):
  0x00 — AP_CTRL  : bit0=ap_start, bit1=ap_done, bit2=ap_idle
  0x10 — rows     : image height (e.g. 480)
  0x18 — cols     : image width  (e.g. 640)
  0x20 — threshold: edge detection threshold (0–255)
"""

import numpy as np
import time
from PIL import Image
from pynq import Overlay, allocate

# ── Configuration ──────────────────────────────────────────────────────────────
BIT_FILE   = "/home/xilinx/jupyter_notebooks/pynq_sobel/system_wrapper2.bit"
INPUT_IMG  = "/home/xilinx/jupyter_notebooks/pynq_sobel/input.png"
OUTPUT_IMG = "/home/xilinx/jupyter_notebooks/pynq_sobel/output_sobel.png"

ROWS      = 480
COLS      = 640
THRESHOLD = 30   # adjustable: lower = more edges, higher = fewer edges
REPEAT    = 100  # timing iterations for stable average

# ── Load Overlay ───────────────────────────────────────────────────────────────
ol       = Overlay(BIT_FILE)
dma      = ol.axi_dma_0
sobel_ip = ol.sobel_filter_0

# ── Prepare Input Image ────────────────────────────────────────────────────────
img    = Image.open(INPUT_IMG).convert("L").resize((COLS, ROWS))
src_np = np.array(img, dtype=np.uint8)

in_buf  = allocate(shape=(ROWS, COLS), dtype=np.uint8)
out_buf = allocate(shape=(ROWS, COLS), dtype=np.uint8)
in_buf[:] = src_np

# ── Configure IP via AXI4-Lite ─────────────────────────────────────────────────
sobel_ip.write(0x10, ROWS)       # rows register
sobel_ip.write(0x18, COLS)       # cols register
sobel_ip.write(0x20, THRESHOLD)  # threshold register

# ── Hardware Inference ─────────────────────────────────────────────────────────
times = []
for _ in range(REPEAT):
    out_buf[:] = 0
    sobel_ip.write(0x00, 0x01)           # ap_start
    dma.recvchannel.transfer(out_buf)
    dma.sendchannel.transfer(in_buf)
    t0 = time.perf_counter()
    dma.sendchannel.wait()
    dma.recvchannel.wait()
    times.append(time.perf_counter() - t0)

hw_ms  = np.mean(times) * 1000
hw_fps = 1000 / hw_ms

# ── Save Result ────────────────────────────────────────────────────────────────
Image.fromarray(out_buf).save(OUTPUT_IMG)

# ── Report ─────────────────────────────────────────────────────────────────────
print(f"Resolution : {COLS}x{ROWS}")
print(f"Threshold  : {THRESHOLD}")
print(f"HW Latency : {hw_ms:.2f} ms")
print(f"HW FPS     : {hw_fps:.1f}")
print(f"Edge Pixels: {np.count_nonzero(out_buf)}")
print(f"Output saved to: {OUTPUT_IMG}")

# ── Release Buffers ────────────────────────────────────────────────────────────
in_buf.freebuffer()
out_buf.freebuffer()