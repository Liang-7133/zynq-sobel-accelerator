#include "sobel.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    // 用静态一维数组代替二维数组
    static pixel_t src[IMG_HEIGHT * IMG_WIDTH];

    // HLS stream 对象
    hls::stream<pkt_t> src_stream("src_stream");
    hls::stream<pkt_t> dst_stream("dst_stream");

    // 生成测试图像（简单棋盘格）
    for (int r = 0; r < IMG_HEIGHT; r++) {
        for (int c = 0; c < IMG_WIDTH; c++) {
            src[r * IMG_WIDTH + c] = ((r / 8 + c / 8) % 2) ? 255 : 0;
        }
    }

    // 将图像写入 src_stream
    for (int r = 0; r < IMG_HEIGHT; r++) {
        for (int c = 0; c < IMG_WIDTH; c++) {
            pkt_t pkt;
            pkt.data = src[r * IMG_WIDTH + c];
            pkt.last = (r == IMG_HEIGHT - 1 && c == IMG_WIDTH - 1) ? 1 : 0;
            pkt.keep = -1;
            pkt.strb = -1;
            src_stream.write(pkt);
        }
    }

    // 调用 HLS 函数（threshold = 128）
    sobel_filter(src_stream, dst_stream, IMG_HEIGHT, IMG_WIDTH, 128);

    // 从 dst_stream 读取结果
    int edge_count = 0;
    for (int r = 0; r < IMG_HEIGHT; r++) {
        for (int c = 0; c < IMG_WIDTH; c++) {
            pkt_t pkt_out = dst_stream.read();
            if (pkt_out.data > 0) edge_count++;
        }
    }

    printf("边缘像素数量: %d / %d\n", edge_count, IMG_HEIGHT * IMG_WIDTH);

    if (edge_count > 0) {
        printf("*** PASS: 检测到边缘，输出正常 ***\n");
        return 0;
    } else {
        printf("*** FAIL: 无边缘输出，请检查代码 ***\n");
        return 1;
    }
}
