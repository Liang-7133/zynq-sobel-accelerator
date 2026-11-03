# Skill 01: AXI DMA Transfer Workflow

## Applicable Scenarios
Any PYNQ project using AXI DMA to transfer data between PS and PL streaming IP.
Works across different board types and different HLS IPs.

## Usage

Two valid orderings exist depending on whether IP uses auto_restart or manual ap_start.

### Pattern A — Manual ap_start before DMA (used in benchmark.ipynb / threshold_test.ipynb)

```python
from pynq import allocate
import numpy as np

in_buf  = allocate(shape=(H, W), dtype=np.uint8)
out_buf = allocate(shape=(H, W), dtype=np.uint8)
in_buf[:] = your_numpy_array

# Trigger IP first, then start DMA
sobel_ip.write(0x00, 0x01)           # ap_start
dma.recvchannel.transfer(out_buf)
dma.sendchannel.transfer(in_buf)
dma.sendchannel.wait()
dma.recvchannel.wait()

# Copy immediately before next transfer overwrites buffer
result = np.copy(out_buf)
```

### Pattern B — DMA first, then ap_start (used in pynq_hdmi.ipynb)

```python
dma.recvchannel.transfer(out_buf)
dma.sendchannel.transfer(in_buf)
sobel_ip.write(0x00, 0x01)           # ap_start after DMA initiated
dma.sendchannel.wait()
dma.recvchannel.wait()
```

Use Pattern A for repeated frame processing loops. Pattern B works for single-shot transfers.

## Common Failures and Diagnosis

| Symptom | Cause | Fix |
|---------|-------|-----|
| recvchannel never becomes idle | IP not consuming stream (ap_start not triggered before data arrives) | Ensure ap_start is written before or immediately after DMA starts |
| RuntimeError: DMA channel not idle | Previous transfer unfinished, or kernel restarted mid-transfer | Restart kernel, reload overlay, reallocate buffers |
| All-zero output | out_buf overwritten before copy | Add `result = np.copy(out_buf)` immediately after wait() |
| Transfer hangs after kernel interrupt | Old DMA state persists in PL | Power cycle board or reload overlay |

## Validated Performance

Confirmed on PYNQ-Z2 with AXI DMA + HLS Sobel IP (system_wrapper2.bit):
- Single image average: **258 FPS** at 640x480 (benchmark.ipynb, 10-run average)
- Batch throughput: **~200 FPS** over 20 frames (4 images x 5 rounds)
- Resolution scaling: 1050 FPS at 160x120 -> 248 FPS at 640x480
