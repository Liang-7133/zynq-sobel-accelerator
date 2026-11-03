# Skill 02: AXI4-Lite Register Control in PYNQ

## Applicable Scenarios
Any HLS IP with s_axilite interface exposed through PYNQ overlay.
Always verify register names via register_map before hardcoding offsets.

## Usage

```python
# Step 1: Check available registers (run this first after loading overlay)
print(sobel_ip.register_map)

# Step 2: Write by offset (confirm offsets from xsobel_filter_hw.h)
sobel_ip.write(0x00, 0x01)    # ap_start
sobel_ip.write(0x10, rows)    # image height
sobel_ip.write(0x18, cols)    # image width
sobel_ip.write(0x20, thresh)  # edge detection threshold

# Step 3: Read back to verify
val = sobel_ip.read(0x20)
```

## Register Offset Reference (from xsobel_filter_hw.h)

| Offset | Register  | Description |
|--------|-----------|-------------|
| 0x00   | AP_CTRL   | bit0=ap_start, bit1=ap_done, bit2=ap_idle |
| 0x10   | rows      | Image height |
| 0x18   | cols      | Image width |
| 0x20   | threshold | Edge detection threshold (0-255) |

## Common Failures and Diagnosis

| Symptom | Cause | Fix |
|---------|-------|-----|
| register_map missing threshold field | HLS pragma not added, or old IP version in block design | Add `#pragma HLS INTERFACE s_axilite port=threshold bundle=CTRL` in HLS source, re-export IP |
| AttributeError: sobel_filter_0 | IP instance name changed in Vivado | Run `print(list(ol.ip_dict.keys()))` to find actual name |
| Writing threshold has no effect | Old bit+hwh files still loaded | Re-upload both .bit and .hwh, then reload overlay |

## Validated Effect

Dynamic threshold control confirmed on system_wrapper2.bit (threshold applied to same 640x480 image):

| Threshold | Edge Pixels |
|-----------|-------------|
| 5         | 216,512     |
| 30        | 125,270     |
| 80        | 64,772      |

## Failure Origin

register_map initially showed only rows/cols — threshold was absent.
Root cause: Vivado block design contained the old IP version without the threshold port.

Fix procedure:
1. In Vivado block design, delete the old sobel_filter IP instance
2. Re-add IP from the updated HLS export
3. Reconnect all ports (AXI Stream, AXI4-Lite, clock, reset)
4. Re-run synthesis, implementation, and generate new bitstream
5. Upload new .bit and .hwh together — mismatched pairs cause silent failures
