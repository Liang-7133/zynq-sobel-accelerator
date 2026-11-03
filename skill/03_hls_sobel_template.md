# Skill 03: HLS Streaming IP Template (Line Buffer + Sliding Window)

## Applicable Scenarios
Any HLS IP that processes 2D image data pixel-by-pixel via AXI Stream.
Template supports different kernel sizes by adjusting line buffer depth.

## Key Design Pattern

```cpp
// Line buffer stores previous (KERNEL-1) rows
pixel_t line_buf[KERNEL-1][IMG_WIDTH];
#pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1

// Sliding window stores current NxN neighborhood
pixel_t win[KERNEL][KERNEL];
#pragma HLS ARRAY_PARTITION variable=win complete

// Pipeline inner loop at II=1
#pragma HLS PIPELINE II=1
```

## Interface Pragma Template

```cpp
#pragma HLS INTERFACE axis port=src_stream register_mode=both
#pragma HLS INTERFACE axis port=dst_stream register_mode=both
#pragma HLS INTERFACE s_axilite port=rows      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=cols      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=threshold bundle=CTRL
#pragma HLS INTERFACE s_axilite port=return    bundle=CTRL
```

## Kernel Size Comparison (hardware measured on PYNQ-Z2, 640x480)

| Kernel | Line Buffers | Window Registers | HLS Function Name | PYNQ IP Instance Name | Latency | FPS | Edge Pixels |
|--------|-------------|------------------|-------------------|-----------------------|---------|-----|-------------|
| 3x3    | 2           | 9                | sobel_filter      | sobel_filter_0        | 4.13 ms | 242 | 125,270     |
| 5x5    | 4           | 25               | sobel_filter_5x5  | sobel_filter_5x5_0    | 4.19 ms | 239 | 152,914     |

5x5 adds only 1.5% latency due to FPGA pipeline parallelism, while detecting 22% more edge pixels.

**Note:** HLS function name determines the PYNQ IP instance name.
Changing the function name from `sobel_filter` to `sobel_filter_5x5` changes
the instance name in the overlay from `sobel_filter_0` to `sobel_filter_5x5_0`.
Always run `print(list(ol.ip_dict.keys()))` after loading a new overlay to confirm the instance name.

## Validated Effect

Both versions synthesized with II=1 in Vitis HLS 2022.2.
Deployed and hardware-measured on PYNQ-Z2 (Zynq-7000 XC7Z020).

## Failure Origin

After loading system_wrapper_5x5.bit, accessing `ol.sobel_filter_0` raised:
`AttributeError: Could not find IP or hierarchy sobel_filter_0 in overlay`

Root cause: the 5x5 HLS function is named `sobel_filter_5x5`, so PYNQ
automatically names the instance `sobel_filter_5x5_0`.
Fix: run `print(list(ol.ip_dict.keys()))` to find the correct name,
then use `ol.sobel_filter_5x5_0` instead.
