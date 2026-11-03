# LLM Collaboration Log

**Project**: zynq-sobel-accelerator
**Board**: PYNQ-Z2 (Zynq-7000 XC7Z020)
**LLM Tool**: MorphMind AI

This document records key AI-assisted development episodes, including prompts used,
model responses, self-correction trajectories, and lessons extracted.

---

## 1. HLS Implementation — Adding AXI4-Lite Threshold Parameter

**Context**: The original `sobel_filter` IP had no runtime-configurable parameter.
The goal was to add a `threshold` port controllable from Python without changing the Block Design.

**Prompt (paraphrased)**:
> "I want to add a threshold parameter to sobel.cpp so I can control edge sensitivity
> from Python at runtime. The interface must not change Block Design wiring."

**Model response**: Provided the complete modified `sobel.cpp` and `sobel.h`, adding:
```cpp
void sobel_filter(
    hls::stream<pkt_t> &src_stream,
    hls::stream<pkt_t> &dst_stream,
    int rows, int cols,
    int threshold          // new parameter
) {
#pragma HLS INTERFACE s_axilite port=threshold bundle=CTRL
```
Also explained that adding an `s_axilite` port only requires re-exporting the IP
and re-synthesizing in Vivado; the AXI Stream and DMA connections remain unchanged.

**Self-correction episode**:
After synthesis, `print(sobel_ip.register_map)` showed only `rows` and `cols` —
no `threshold`. First suspicion was an HLS pragma error. Model prompted:
> "Check whether the Block Design still uses the old IP version.
> Remove and re-add the IP, then reconnect and regenerate the bitstream."

After re-adding the IP in Vivado and regenerating, `register_map` showed:
```
rows = Register(rows=write-only)
cols = Register(cols=write-only)
threshold = Register(threshold=write-only)   # appeared after fix
```

**Lesson**: When `register_map` is missing an expected field, the issue is almost
always a stale IP in Block Design, not an HLS pragma error. Always remove and
re-add the IP after modifying its interface.

---

## 2. AXI DMA Debugging — Transfer Order and ap_start Timing

**Context**: During `threshold_test.ipynb` development, running three consecutive
threshold values caused the second and third transfers to produce all-zero output.

**Prompt (paraphrased)**:
> "The first threshold=5 transfer gives correct output, but threshold=30 and threshold=80
> both return all-zero arrays. I'm reusing the same in_buf and out_buf."

**Model diagnosis**:
> "The all-zero output is caused by reading `out_buf` before the DMA transfer completes,
> or by `out_buf` being overwritten by the next transfer before you copy it.
> Add `result = np.copy(out_buf)` immediately after `dma.recvchannel.wait()`."

**Fix applied**:
```python
dma.recvchannel.wait()
results[thresh] = np.copy(out_buf)   # copy immediately, before next iteration
```

**Second issue**: On a fresh kernel restart, `recvchannel` occasionally never
became idle. Model identified the root cause:
> "IP `ap_start` must be written before or immediately after DMA starts.
> If data arrives at the IP input before `ap_start=1`, the IP stalls and
> DMA recvchannel hangs indefinitely."

**Fix applied** (Pattern A — manual ap_start):
```python
sobel_ip.write(0x00, 0x01)           # ap_start first
dma.recvchannel.transfer(out_buf)
dma.sendchannel.transfer(in_buf)
dma.sendchannel.wait()
dma.recvchannel.wait()
```

**Lesson**: `np.copy()` after every `recvchannel.wait()` is mandatory in loops.
`ap_start` timing relative to DMA start affects whether the IP consumes the stream.

---

## 3. 3x3 vs 5x5 Kernel Comparison — IP Instance Name Error

**Context**: After exporting the 5x5 bitstream and loading it in `sobel_3v5_hw.ipynb`,
accessing `ol_5x5.sobel_filter_0` raised an `AttributeError`.

**Prompt (paraphrased)**:
> "I get AttributeError: Could not find IP or hierarchy sobel_filter_0 in overlay."

**Model response**:
> "The HLS top-level function name changed to `sobel_filter_5x5`, so Vivado
> named the IP instance `sobel_filter_5x5_0`. Run:
> `print(list(ol_5x5.ip_dict.keys()))` to find the actual instance name."

**Output**:
```python
['sobel_filter_5x5_0', 'axi_dma_0', 'processing_system7_0']
```

**Fix**: Changed `ol_5x5.sobel_filter_0` to `ol_5x5.sobel_filter_5x5_0`.

**Lesson**: When the HLS top-function name changes, the Vivado IP instance name
changes accordingly. Always verify with `ip_dict.keys()` when loading a new bitstream.

---

## 4. Visualization — Matplotlib Chinese Character Rendering

**Context**: Dashboard and comparison plots displayed Chinese labels as hollow squares
on the PYNQ board (no CJK font installed).

**Prompt (paraphrased)**:
> "All Chinese text in my matplotlib plots shows as empty boxes."

**Model response**: Identified missing CJK font on the PYNQ system, recommended
replacing all Chinese strings with English equivalents rather than installing fonts
(simpler and more portable).

**Fix applied** (example):
```python
# Before
axes[i].set_title(f'threshold={thresh}  边缘像素={np.count_nonzero(...)}')

# After
axes[i].set_title(f'Threshold={thresh}  Edge Pixels={np.count_nonzero(...)}')
```

**Lesson**: PYNQ venv has no CJK font by default. All plot labels, titles, and
axis text should use English for portability.

---

## 5. PYNQ Global State Error — Loading Base Overlay After Custom Bitstream

**Context**: After running custom Sobel notebooks, loading the PYNQ base overlay
in a new notebook raised `RuntimeError: Unable to parse metadata`, with the traceback
pointing to a missing `system_wrapper.bit`.

**Prompt (paraphrased)**:
> "Loading base.bit raises RuntimeError. The traceback mentions
> /home/xilinx/jupyter_notebooks/pynq_sobel/system_wrapper.bit which no longer exists."

**Model diagnosis**:
> "PYNQ caches the last loaded bitstream path in `global_pl_state.json`.
> When that file no longer exists, the cache check raises a FileNotFoundError
> that propagates and blocks any new overlay from loading.
> Find and delete the stale cache file."

**Commands used**:
```bash
find / -name "*.json" -path "*/pynq/*" 2>/dev/null
# Found: /usr/local/share/pynq-venv/lib/python3.10/site-packages/pynq/pl_server/global_pl_state.json

sudo rm -f /usr/local/.../global_pl_state.json
```

**Lesson**: If `Overlay()` raises `RuntimeError: Unable to parse metadata` after
renaming or moving a bitstream file, delete `global_pl_state.json` and reload.
This is a PYNQ state-cache bug, not an HLS or Vivado issue.

---

## Summary: AI Collaboration Workflow

| Phase | AI Role | Key Contribution |
|-------|---------|-----------------|
| HLS design | Code generation | Complete `sobel.cpp` / `sobel5.cpp` with correct pragmas |
| Interface debug | Root cause analysis | Identified stale IP in Block Design as cause of missing register |
| DMA debug | Pattern recommendation | `np.copy()` discipline + `ap_start` timing explanation |
| Kernel comparison | Error diagnosis | `ip_dict.keys()` lookup pattern for instance name changes |
| Visualization | Fix recommendation | English-only labels for PYNQ portability |
| System debug | State-cache identification | `global_pl_state.json` deletion procedure |

The AI was most effective when given **specific error messages and exact output**
rather than vague descriptions. Each debugging cycle followed the pattern:
observe symptom → provide exact output → receive targeted fix → verify.
