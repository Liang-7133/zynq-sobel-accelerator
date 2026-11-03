#ifndef SOBEL5_H
#define SOBEL5_H

#include <hls_stream.h>
#include <ap_int.h>

#define IMG_WIDTH  640
#define IMG_HEIGHT 480

typedef unsigned char pixel_t;
typedef struct {
    pixel_t data;
    int      last;
    int      keep;
    int      strb;
} pkt5_t;

#pragma HLS STREAM variable=pkt5_t

void sobel_filter_5x5(
    hls::stream<pkt5_t> &src_stream,
    hls::stream<pkt5_t> &dst_stream,
    int rows,
    int cols,
    int threshold
);

#endif