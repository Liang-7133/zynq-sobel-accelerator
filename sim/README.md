# Simulation — C Simulation (CSIM)

## Purpose

Verify the functional correctness of the Sobel HLS kernel (`sobel_filter`)
before synthesis. The testbench generates a known checkerboard pattern at
640x480, runs it through the HLS function, and compares the output against
a software reference pixel-by-pixel.

## Testbench: `src/hls/sobel_tb.cpp`

- Generates a 640x480 checkerboard grayscale image as input
- Runs `sobel_filter()` via AXI Stream simulation (HLS stream model)
- Computes software reference Sobel output
- Reports max pixel error and mismatch count

## How to Re-run

1. Open Vitis HLS 2022.2
2. Open the project (`src/hls/` as source directory, `sobel_tb.cpp` as testbench)
3. Click **Project → Run C Simulation** (or press the green play button)
4. Check the Console output for `PASS` and `CSim done with 0 errors`

## Result

```
INFO: [SIM 2] *************** CSIM start ***************
INFO: [SIM 4] CSIM will launch GCC as the compiler.
   Compiling sobel_tb.cpp in debug mode
   Compiling sobel.cpp in debug mode
   Generating csim.exe
[TB] Max error: 0  |  Mismatched pixels: 0 / 307200
[TB] *** PASS: DUT output matches reference exactly ***
INFO: [SIM 1] CSim done with 0 errors.
INFO: [SIM 3] *************** CSIM finish ***************
```

- **Max pixel error**: 0
- **Mismatched pixels**: 0 / 307,200 (640x480 = 100% match)
- **Verdict**: PASS

See `csim_pass.png` for the full console screenshot.
