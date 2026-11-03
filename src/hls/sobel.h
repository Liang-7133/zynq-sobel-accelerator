#ifndef SOBEL_H
#define SOBEL_H

#include "hls_stream.h"
#include "ap_axi_sdata.h"
#include "ap_int.h"

#define IMG_WIDTH  640
#define IMG_HEIGHT 480

typedef ap_uint<8> pixel_t;
typedef ap_axiu<8,1,1,1> pkt_t;

void sobel_filter(
    hls::stream<pkt_t> &src_stream,
    hls::stream<pkt_t> &dst_stream,
    int rows,
    int cols,
    int threshold
);

#endif
