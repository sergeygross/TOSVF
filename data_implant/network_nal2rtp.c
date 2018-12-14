#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "other/list.h"
#include "network_naltortp.h"
#include "main.h"

// ============================================================

#define BASEALGO 1

#define DEBUG_PRINT 0
#define debug_print(fmt, ...) \
    do { if (DEBUG_PRINT) fprintf(stderr, "-------%s: %d: %s():---" fmt "----\n", \
            __FILE__, __LINE__, __func__, ##__VA_ARGS__);} while (0)

uint16_t DEST_PORT;
linklist CLIENT_IP_LIST;
uint8_t SENDBUFFER[SEND_BUF_SIZE];

// ============================================================

void add_client_list(linklist client_ip_list, int client_port, const char *ipaddr)
{
    struct sockaddr_in server_c;
    pnode pnode_tmp;
    const int on = 1;

    insert_nodulp_node(client_ip_list, ipaddr);

    // strncpy(client_ip_list->node_info.ipaddr, "192.168.1.33", 16);

    pnode_tmp = search_node(client_ip_list, ipaddr);

    printf("client_ip_list->ip = %s\n", pnode_tmp->node_info.ipaddr);

    server_c.sin_family = AF_INET;
    server_c.sin_port = htons(client_port);
    server_c.sin_addr.s_addr = inet_addr(ipaddr);
    pnode_tmp->send_fail_n = 0;
    pnode_tmp->node_info.socket_c = socket(AF_INET, SOCK_DGRAM, 0);

    printf("client_ip_list->socket = %d\n", pnode_tmp->node_info.socket_c);

    if (setsockopt(pnode_tmp->node_info.socket_c, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) {
        fprintf(stderr, "initSvr: Socket options set error.\n");
        exit(errno);
    }

    if ((connect(pnode_tmp->node_info.socket_c, (const struct sockaddr *)&server_c, sizeof(struct sockaddr_in))) == -1) {
        perror("connect");
        exit(-1);
    }

    return;
} 

// ============================================================

void send_data_to_client_list(uint8_t *send_buf, size_t len_sendbuf, linklist client_ip_list)
{
    int ret;
    pnode pnode_tmp0;
    pnode_tmp0 = client_ip_list->next;
    while (pnode_tmp0) {
    debug_print("len is %d", (int)len_sendbuf);

        if ((ret = send(pnode_tmp0->node_info.socket_c, send_buf, len_sendbuf, MSG_DONTWAIT)) < 0) {					// MSG_DONTWAIT
            fprintf(stderr, "----- send fail errno is %d----\n", errno);
            // pnode_tmp->send_fail_n 
            if (errno > 0)
                pnode_tmp0->send_fail_n++;
 
            // send MAX_SEND_FAIL_N  
            if (pnode_tmp0->send_fail_n > MAX_SEND_FAIL_N) {
                close(pnode_tmp0->node_info.socket_c);
                // pnode_tmp0 = delete_this_node(client_ip_list, pnode_tmp0);
            } /* if (pnode_tmp->send_fail_n > 20) */

            perror("send");
        } /* if (send(pnode_tmp0->node_info.socket_c, SENDBUF, send_bytes, 0) == -1) */

        pnode_tmp0 = pnode_tmp0->next;
    } /* while (pnode_tmp0) */

    return;
} 

// ============================================================

int h264nal2rtp_send(int framerate, uint8_t *pstStream, int nalu_len, linklist client_ip_list)
{
    uint8_t *nalu_buf;
    nalu_buf = pstStream;
    static uint32_t ts_current = 0;
    static uint16_t seq_num = 0;
    rtp_header_t *rtp_hdr;
    nalu_header_t *nalu_hdr;
    fu_indicator_t *fu_ind;
    fu_header_t *fu_hdr;
    size_t len_sendbuf;

    int fu_pack_num;     
    int last_fu_pack_size;  
    int fu_seq;             		

    debug_print();
    ts_current += (90000 / framerate);  			// 90000 / 25 = 3600

        if (nalu_len < 1) {     
            return -1;
        }

        if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) {

            // single nal unit
            memset(SENDBUFFER, 0, SEND_BUF_SIZE);


            rtp_hdr = (rtp_header_t *)SENDBUFFER;
            rtp_hdr->csrc_len = 0;
            rtp_hdr->extension = 0;
            rtp_hdr->padding = 0;
            rtp_hdr->version = 2;
            rtp_hdr->payload_type = H264;
		    // rtp_hdr->marker = (pstStream->u32PackCount - 1 == i) ? 1 : 0;  
			rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
            rtp_hdr->timestamp = htonl(ts_current);
            rtp_hdr->ssrc = htonl(SSRC_NUM);

    		debug_print();

#if BASEALGO
            nalu_hdr = (nalu_header_t *)&SENDBUFFER[12];
            nalu_hdr->f = (nalu_buf[0] & 0x80) >> 7;        			// bit0 
            nalu_hdr->nri = (nalu_buf[0] & 0x60) >> 5;      		// bit1~2 
            nalu_hdr->type = (nalu_buf[0] & 0x1f);
    debug_print();
#else
            SENDBUFFER[12] = ((nalu_buf[0] & 0x80))    	// bit0: f 
                | (nalu_buf[0] & 0x60)                 						// bit1~2: nri 
                | (nalu_buf[0] & 0x1f);                						// bit3~7: type 
#endif


    	debug_print();
            memcpy(SENDBUFFER + 13, nalu_buf + 1, nalu_len - 1);  


            len_sendbuf = 12 + nalu_len;
            send_data_to_client_list(SENDBUFFER, len_sendbuf, client_ip_list);
    debug_print();
        } else {    					// nalu_len > RTP_PAYLOAD_MAX_SIZE
    debug_print();

            /*
             * 1. RTP_PAYLOAD_MAX_SIZE BYLE
             */
            fu_pack_num = nalu_len % RTP_PAYLOAD_MAX_SIZE ? (nalu_len / RTP_PAYLOAD_MAX_SIZE + 1) : nalu_len / RTP_PAYLOAD_MAX_SIZE;
            last_fu_pack_size = nalu_len % RTP_PAYLOAD_MAX_SIZE ? nalu_len % RTP_PAYLOAD_MAX_SIZE : RTP_PAYLOAD_MAX_SIZE;
            fu_seq = 0;

            for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) {
                memset(SENDBUFFER, 0, SEND_BUF_SIZE);


                if (fu_seq == 0) { 
                    rtp_hdr = (rtp_header_t *)SENDBUFFER;
                    rtp_hdr->csrc_len = 0;
                    rtp_hdr->extension = 0;
                    rtp_hdr->padding = 0;
                    rtp_hdr->version = 2;
                    rtp_hdr->payload_type = H264;
		            rtp_hdr->marker = 0;   
			        rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
                    rtp_hdr->timestamp = htonl(ts_current);
                    rtp_hdr->ssrc = htonl(SSRC_NUM);

#if BASEALGO
                    fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                    fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                    fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                    fu_ind->type = 28;
#else   
                    SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7 			// bit0: f 
                        | (nalu_buf[0] & 0x60) >> 4             		// bit1~2: nri
                        | 28 << 3;                              		// bit3~7: type
#endif

#if BASEALGO
                    fu_hdr = (fu_header_t *)&SENDBUFFER[13];
                    fu_hdr->s = 1;
                    fu_hdr->e = 0;
                    fu_hdr->r = 0;
                    fu_hdr->type = nalu_buf[0] & 0x1f;
#else
                    SENDBUFFER[13] = 1 | (nalu_buf[0] & 0x1f) << 3;
#endif

                    memcpy(SENDBUFFER + 14, nalu_buf + 1, RTP_PAYLOAD_MAX_SIZE - 1);  

                    len_sendbuf = 12 + 2 + (RTP_PAYLOAD_MAX_SIZE - 1); 
                    send_data_to_client_list(SENDBUFFER, len_sendbuf, client_ip_list);

                } else if (fu_seq < fu_pack_num - 1) { 
                    rtp_hdr = (rtp_header_t *)SENDBUFFER;
                    rtp_hdr->csrc_len = 0;
                    rtp_hdr->extension = 0;
                    rtp_hdr->padding = 0;
                    rtp_hdr->version = 2;
                    rtp_hdr->payload_type = H264;
		            rtp_hdr->marker = 0; 
			        rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
                    rtp_hdr->timestamp = htonl(ts_current);
                    rtp_hdr->ssrc = htonl(SSRC_NUM);

#if BASEALGO
                    fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                    fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                    fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                    fu_ind->type = 28;

                    fu_hdr = (fu_header_t *)&SENDBUFFER[13];
                    fu_hdr->s = 0;
                    fu_hdr->e = 0;
                    fu_hdr->r = 0;
                    fu_hdr->type = nalu_buf[0] & 0x1f;
#else 
                    SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  	// bit0: f 
                        | (nalu_buf[0] & 0x60) >> 4             	// bit1~2: nri
                        | 28 << 3;                              	// bit3~7: type

                    SENDBUFFER[13] = 0 | (nalu_buf[0] & 0x1f) << 3;
#endif
                    memcpy(SENDBUFFER + 14, nalu_buf + RTP_PAYLOAD_MAX_SIZE * fu_seq, RTP_PAYLOAD_MAX_SIZE);

                    len_sendbuf = 12 + 2 + RTP_PAYLOAD_MAX_SIZE;
                    send_data_to_client_list(SENDBUFFER, len_sendbuf, client_ip_list);

                } else { 
                     rtp_hdr = (rtp_header_t *)SENDBUFFER;
                    rtp_hdr->csrc_len = 0;
                    rtp_hdr->extension = 0;
                    rtp_hdr->padding = 0;
                    rtp_hdr->version = 2;
                    rtp_hdr->payload_type = H264;
		            rtp_hdr->marker = 1;    
			        rtp_hdr->seq_no = htons(++seq_num % UINT16_MAX);
                    rtp_hdr->timestamp = htonl(ts_current);
                    rtp_hdr->ssrc = htonl(SSRC_NUM);

#if BASEALGO
                    fu_ind = (fu_indicator_t *)&SENDBUFFER[12];
                    fu_ind->f = (nalu_buf[0] & 0x80) >> 7;
                    fu_ind->nri = (nalu_buf[0] & 0x60) >> 5;
                    fu_ind->type = 28;

                    fu_hdr = (fu_header_t *)&SENDBUFFER[13];
                    fu_hdr->s = 0;
                    fu_hdr->e = 1;
                    fu_hdr->r = 0;
                    fu_hdr->type = nalu_buf[0] & 0x1f;
#else   
                    SENDBUFFER[12] = (nalu_buf[0] & 0x80) >> 7  				// bit0: f
                        | (nalu_buf[0] & 0x60) >> 4             				// bit1~2: nri
                        | 28 << 3;                             					// bit3~7: type

                    SENDBUFFER[13] = 1 << 1 | (nalu_buf[0] & 0x1f) << 3;
#endif

                    memcpy(SENDBUFFER + 14, nalu_buf + RTP_PAYLOAD_MAX_SIZE * fu_seq, last_fu_pack_size);
                    len_sendbuf = 12 + 2 + last_fu_pack_size;
                    send_data_to_client_list(SENDBUFFER, len_sendbuf, client_ip_list);

                } /* else-if (fu_seq == 0) */
            } /* end of for (fu_seq = 0; fu_seq < fu_pack_num; fu_seq++) */

        } /* end of else-if (nalu_len <= RTP_PAYLOAD_MAX_SIZE) */

    debug_print();
#ifndef BASEALGO
        if (nalu_buf) {
            free(nalu_buf);
            nalu_buf = NULL;
        }
#endif

    return 0;
} 
