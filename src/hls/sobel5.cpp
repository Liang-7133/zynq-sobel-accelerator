#include "sobel.h"   // 复用 pkt_t 定义

// 5×5 Sobel 卷积核
// Gx:
// -1  -2   0   2   1
// -4  -8   0   8   4
// -6 -12   0  12   6
// -4  -8   0   8   4
// -1  -2   0   2   1
//
// Gy（Gx 转置 × -1）:
// -1  -4  -6  -4  -1
// -2  -8 -12  -8  -2
//  0   0   0   0   0
//  2   8  12   8   2
//  1   4   6   4   1

void sobel_filter_5x5(
    hls::stream<pkt_t> &src_stream,
    hls::stream<pkt_t> &dst_stream,
    int rows,
    int cols,
    int threshold
) {
#pragma HLS INTERFACE axis port=src_stream register_mode=both
#pragma HLS INTERFACE axis port=dst_stream register_mode=both
#pragma HLS INTERFACE s_axilite port=rows      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=cols      bundle=CTRL
#pragma HLS INTERFACE s_axilite port=threshold bundle=CTRL
#pragma HLS INTERFACE s_axilite port=return    bundle=CTRL

    // 行缓冲：存前 4 行
    pixel_t line_buf[4][IMG_WIDTH];
#pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1

    // 滑窗：5 行 × 5 列
    pixel_t win[5][5];
#pragma HLS ARRAY_PARTITION variable=win complete

    ROW_LOOP:
    for (int row = 0; row < rows; row++) {
        COL_LOOP:
        for (int col = 0; col < cols; col++) {
#pragma HLS PIPELINE II=1

            // 读入当前像素
            pkt_t pkt_in = src_stream.read();
            pixel_t cur = (pixel_t)pkt_in.data;

            // 更新行缓冲（滚动）
            pixel_t p0 = line_buf[0][col];
            pixel_t p1 = line_buf[1][col];
            pixel_t p2 = line_buf[2][col];
            pixel_t p3 = line_buf[3][col];
            line_buf[0][col] = p1;
            line_buf[1][col] = p2;
            line_buf[2][col] = p3;
            line_buf[3][col] = cur;

            // 滑窗右移
            for (int r = 0; r < 5; r++) {
#pragma HLS UNROLL
                win[r][0] = win[r][1];
                win[r][1] = win[r][2];
                win[r][2] = win[r][3];
                win[r][3] = win[r][4];
            }
            win[0][4] = p0;
            win[1][4] = p1;
            win[2][4] = p2;
            win[3][4] = p3;
            win[4][4] = cur;

            pixel_t result = 0;

            if (row >= 4 && col >= 4) {
                // 5×5 Sobel Gx
                int gx =
                    -1*win[0][0] -  2*win[0][1] + 0 +  2*win[0][3] + 1*win[0][4]
                    -4*win[1][0] -  8*win[1][1] + 0 +  8*win[1][3] + 4*win[1][4]
                    -6*win[2][0] - 12*win[2][1] + 0 + 12*win[2][3] + 6*win[2][4]
                    -4*win[3][0] -  8*win[3][1] + 0 +  8*win[3][3] + 4*win[3][4]
                    -1*win[4][0] -  2*win[4][1] + 0 +  2*win[4][3] + 1*win[4][4];

                // 5×5 Sobel Gy
                int gy =
                    -1*win[0][0] - 4*win[0][1] -  6*win[0][2] - 4*win[0][3] - 1*win[0][4]
                    -2*win[1][0] - 8*win[1][1] - 12*win[1][2] - 8*win[1][3] - 2*win[1][4]
                    + 2*win[3][0] + 8*win[3][1] + 12*win[3][2] + 8*win[3][3] + 2*win[3][4]
                    + 1*win[4][0] + 4*win[4][1] +  6*win[4][2] + 4*win[4][3] + 1*win[4][4];

                int mag = (gx < 0 ? -gx : gx) + (gy < 0 ? -gy : gy);
                // 归一化（5×5 权重和约为 3×3 的 8 倍）
                mag = mag / 8;
                int thresh_val = (threshold == 0) ? 30 : threshold;
                result = (mag > thresh_val) ? (pixel_t)255 : (pixel_t)0;
            }

            pkt_t pkt_out;
            pkt_out.data = result;
            pkt_out.last = (row == rows-1 && col == cols-1) ? 1 : 0;
            pkt_out.keep = -1;
            pkt_out.strb = -1;
            dst_stream.write(pkt_out);
        }
    }
}