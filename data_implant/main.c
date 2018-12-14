#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <unistd.h>

#define MRTP_WRITE_DELAY 10000  	// 10 ms
#define MRTP_DATA_FREQUENCY 150		// 1 write for XX packets
#define MAX_TARGET_OUTPUT 4

#include "main.h"

// =====================================

int main(int argc, char **argv)
{
    AVFormatContext *ifmt_ctx = NULL;
    AVPacket pkt;
    const char *in_filename; // *out_filename;
    int ret, send_filtered_size, no_memory_control;
    target_stream_data dest[MAX_TARGET_OUTPUT];

    // bsf filtering
	struct CodeImplant sample_array;
    AVPacket filtered_pkt;
    linklist client_ip_list;

	// init
    ret = 0;
	send_filtered_size = 0;
	no_memory_control = 0;
	int k = 1;

#ifdef FILE_OUTPUT_DEBUG
	FILE *fp_stream;
	fp_stream = fopen("test.264", "wb");
#endif

    if (argc < 3) {
        printf("Encryptor v1.0.\n" \
               "./encode_stream <input> <output1> <out_port1> \n"  \
               "Encryptor gets the <input> stream (any kind) and feeds the telemetry on port 50500 and outputs the RTP-stream with implanted telemetry data to <output><port> address.\n"  \
			   "\nexample: ./encrypt 'rtsp://192.168.0.120:554/user=admin&password=&channel=1&stream=0.sdp?&overrun_nonfatal=1' " \
               "\n");
        return 1;
    }

	in_filename = argv[1];
    strncpy(dest[0].stream, argv[2], MAX_STREAM_NAME);
    dest[0].port = atoi(argv[3]);
    printf("%s:%d\n", dest[0].stream, dest[0].port);

#ifdef BSF_NO_MEMORY_CONTROL
		av_new_packet(&filtered_pkt, BIGGEST_PACKET);
		no_memory_control = 1;
#else
		av_init_packet(&filtered_pkt);
		no_memory_control = 0;
#endif

    client_ip_list = create_null_list_link();
   	add_client_list(client_ip_list, dest[0].port, dest[0].stream);

    av_register_all();

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
        fprintf(stderr, "Could not open input file '%s'", in_filename);
        goto end;
    }
    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	printf("\nsizeof (capsule) = %d bytes\n\n", (int) sizeof(struct CodeImplant) );
	set_sample_sei_data(&sample_array);

    while (1) {
        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0) break;

		if((sample_array.ObjectStruct[0].x_pos > 300) || (sample_array.ObjectStruct[0].y_pos > 200) ) {
			k = -1;
		}
		else if ( (sample_array.ObjectStruct[0].x_pos < 10) || (sample_array.ObjectStruct[0].y_pos < 10) ) {
			k=1;
		}
		else {} 
		sample_array.ObjectStruct[0].x_pos+=k; 
		sample_array.ObjectStruct[1].y_pos+=k;


			if ((send_filtered_size = h264_data_implant(pkt.data, pkt.size, filtered_pkt.data, &sample_array, AV_INPUT_BUFFER_PADDING_SIZE, no_memory_control)) < 10) {
				printf("send unfiltered (no 0001 border)\n");
				if (send_filtered_size == -1) {
					printf("Implant error!\n\n");					
					return -1;
				}
#ifdef FILE_OUTPUT_DEBUG
				fwrite (pkt.data , pkt.size*sizeof(uint8_t), 1, fp_stream);
#else
				ret = h264nal2rtp_send(25, pkt.data, pkt.size, client_ip_list);
				if (ret != -1)  usleep(3000);
#endif				
			}
		    else {
				printf("FILTERED old=%d, new=%d\n", pkt.size, send_filtered_size);
#ifdef FILE_OUTPUT_DEBUG
				fwrite (filtered_pkt.data , filtered_pkt.size*sizeof(uint8_t), 1, fp_stream);
#else
				ret = h264nal2rtp_send(25, filtered_pkt.data, send_filtered_size, client_ip_list);
        		if (ret != -1)  usleep(3000);
#endif		
			}

        av_packet_unref(&pkt);
    }

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

end:
    avformat_close_input(&ifmt_ctx);
    av_packet_unref(&filtered_pkt);
  
#ifdef FILE_OUTPUT_DEBUG
				fclose (fp_stream);
#endif

    if (ret < 0 && ret != AVERROR_EOF) {
        // fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
