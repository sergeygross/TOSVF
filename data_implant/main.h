#ifndef DATA_IMPLANT_H_
#define DATA_IMPLANT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavutil/avutil.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
}
#endif

#include "defines.h"
#include "other/list.h"
#include "filter_data_format.h"


typedef struct _output_data {
    char stream[MAX_STREAM_NAME];
    int port;
} target_stream_data;


void add_client_list(linklist client_ip_list, int client_port, const  char *ipaddr);
int h264nal2rtp_send(int framerate, uint8_t *pstStream, int nalu_len, linklist client_ip_list);
int h264_data_implant(uint8_t *in_array, int in_size, uint8_t *out_array, struct CodeImplant *array_implant, const int padding, int no_memory_control);
int set_sample_sei_data(struct CodeImplant *array_implant);

#endif /* DATA_IMPLANT_H_ */
