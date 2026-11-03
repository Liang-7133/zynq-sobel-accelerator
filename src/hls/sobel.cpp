#include "sobel.h"

void sobel_filter(
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

    // 本地行缓冲（存前两行）
    pixel_t line_buf[2][IMG_WIDTH];
#pragma HLS ARRAY_PARTITION variable=line_buf complete dim=1

    // 列缓冲：暂存当前列左右像素（3列滑窗）
    pixel_t col_buf[3][3];
#pragma HLS ARRAY_PARTITION variable=col_buf complete

    ROW_LOOP:
    for (int row = 0; row < rows; row++) {
        COL_LOOP:
        for (int col = 0; col < cols; col++) {
#pragma HLS PIPELINE II=1

            // 从 stream 读取当前像素
            pkt_t pkt_in = src_stream.read();
            pixel_t cur = (pixel_t)pkt_in.data;

            // 更新行缓冲（滚动存储）
            pixel_t p_prev2 = line_buf[0][col];  // row-2 行
            pixel_t p_prev1 = line_buf[1][col];  // row-1 行
            line_buf[0][col] = p_prev1;
            line_buf[1][col] = cur;

            // 滑窗：将三行数据右移一列
            col_buf[0][0] = col_buf[0][1]; col_buf[0][1] = col_buf[0][2]; col_buf[0][2] = p_prev2;
            col_buf[1][0] = col_buf[1][1]; col_buf[1][1] = col_buf[1][2]; col_buf[1][2] = p_prev1;
            col_buf[2][0] = col_buf[2][1]; col_buf[2][1] = col_buf[2][2]; col_buf[2][2] = cur;

            pixel_t result = 0;

            if (row >= 2 && col >= 2) {
                int p00 = col_buf[0][0]; int p01 = col_buf[0][1]; int p02 = col_buf[0][2];
                int p10 = col_buf[1][0];                           int p12 = col_buf[1][2];
                int p20 = col_buf[2][0]; int p21 = col_buf[2][1]; int p22 = col_buf[2][2];

                int gx = -p00 + p02 - 2*p10 + 2*p12 - p20 + p22;
                int gy = -p00 - 2*p01 - p02 + p20 + 2*p21 + p22;

                int mag = (gx < 0 ? -gx : gx) + (gy < 0 ? -gy : gy);
                // 二值化输出：超过阈值为255，否则为0
                result = (mag > threshold) ? (pixel_t)255 : (pixel_t)0;
            }

            // 写入输出 stream，最后一个像素置 TLAST=1
            pkt_t pkt_out;
            pkt_out.data = result;
            pkt_out.last = (row == rows - 1 && col == cols - 1) ? 1 : 0;
            pkt_out.keep = -1;
            pkt_out.strb = -1;
            dst_stream.write(pkt_out);
        }
    }
}
