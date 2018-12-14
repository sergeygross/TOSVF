#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "filter_data_format.h"
#include "defines.h"


// ===========================================================

int set_sample_sei_data(struct CodeImplant *array_implant){

	memset ((uint8_t *) array_implant, 0, sizeof(struct CodeImplant) );
	
	array_implant->prefix_byte1 = 0xB1; 	  		//  B1 B3 B5 SEI-2 signature
	array_implant->prefix_byte2 = 0xB3;
	array_implant->prefix_byte3 = 0xB5; 

 	array_implant->ObjectStruct[0].x_pos = 12; 
 	array_implant->ObjectStruct[0].y_pos = 12; 
 	array_implant->ObjectStruct[0].x_center = 35; 
 	array_implant->ObjectStruct[0].y_center = 32; 
 	array_implant->ObjectStruct[0].width = 40; 
 	array_implant->ObjectStruct[0].height = 50; 
 	memcpy(array_implant->ObjectStruct[0].desc, "car\0", MAX_DESC_SIZE); 

 	array_implant->ObjectStruct[1].x_pos = 22; 
 	array_implant->ObjectStruct[1].y_pos = 22; 
 	array_implant->ObjectStruct[1].x_center = 45; 
 	array_implant->ObjectStruct[1].y_center = 42; 
 	array_implant->ObjectStruct[1].width = 34; 
 	array_implant->ObjectStruct[1].height = 35; 
 	memcpy(array_implant->ObjectStruct[1].desc, "bus\0", MAX_DESC_SIZE); 

 	array_implant->ObjectStruct[2].x_pos = 32; 
 	array_implant->ObjectStruct[2].y_pos = 32; 
 	array_implant->ObjectStruct[2].x_center = 45; 
 	array_implant->ObjectStruct[2].y_center = 42; 
 	array_implant->ObjectStruct[2].width = 54; 
 	array_implant->ObjectStruct[2].height = 25; 
 	memcpy(array_implant->ObjectStruct[2].desc, "person\0", MAX_DESC_SIZE); 

	array_implant->object_number = 3;

	return 0;
}


// ===========================================================

int grow_packet(uint8_t *buffer, int buffer_size, int grow_by, int padding)
{
    int new_size;

    if ((unsigned)grow_by > (unsigned)(1000000 - (buffer_size + padding)) )			// 1000000 instead of INT_MAX (reduce)
        return -1;

    new_size = buffer_size + grow_by + padding;

    if (buffer) {
        uint8_t *old_data = buffer;
		buffer = realloc(&buffer, new_size + padding);
		if (buffer == NULL) {
		       buffer = old_data;
		       return -1;
		 }
    }

    buffer_size += grow_by;
    memset(buffer + buffer_size, 0, padding);

    return 0;
}


// ===========================================================
// set output array copy
// full-size: in_size + telemetry_size (sei_header_t + telemetry) + padding
// 1) copy new sei_header_t
// 2) copy new telemetry
// 3) copy last in-data data (0001 to end)

int h264_data_implant(uint8_t *in_array, int in_size, uint8_t *out_array, struct CodeImplant *array_implant, const int padding, int no_memory_control)
{
	int i, err;
	int total_size, last_indata;
	const uint8_t nalu_header[4] = { 0, 0, 0, 1 };
	uint8_t *pointer;
	uint8_t *p_zero_buf[3];
	struct sei_header_t sei_header;

    // random ID number generated according to ISO-11578
    static const uint8_t uuid[16] = {
        0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };

    static int additional_size = sizeof(struct CodeImplant) + sizeof(struct sei_header_t);
    total_size = in_size + additional_size + padding;

	// do not use big packets for data implant
	if (BIGGEST_PACKET < total_size) return 0;

	p_zero_buf[0] = in_array;
	p_zero_buf[1] = 0;    																		// place_of_0001_origin
	p_zero_buf[2] = in_array + in_size - 1;

	// 1. Scan buf for 0001 and save it`s positions to buf
	for (i=0; i < in_size; i++) {
		if( !memcmp(in_array + i, nalu_header, sizeof(nalu_header) ) ) {
			 p_zero_buf[1] = in_array + i;
			 break;
			}
         }
    if (!p_zero_buf[1]) return 0;


    // prepare structs
    sei_header.F_NRI_TYPE = 1 + (15 << 3);
	sei_header.payloadType = 5;
	sei_header.payloadSize = sizeof(struct CodeImplant) - 3;
	memcpy(sei_header.uuid, uuid, 16);
	memcpy(sei_header.null_header, nalu_header, 4);

   // 2. Alloc out space for in data (if needed)
	if(!no_memory_control) {
		err = grow_packet(out_array, in_size, total_size - in_size, padding);
		if (err < 0) {
				printf("av_grow_packet error! Buffer stays the same. Exiting..\n\n");				
				goto err;
			}
	}

	pointer = out_array;

    //form output: copy new sei_header_t
    memcpy(pointer, &sei_header, sizeof(sei_header));
    pointer += sizeof(sei_header);

	// copy data to SEI 
    memcpy(pointer, array_implant, sizeof(struct CodeImplant));
    pointer += sizeof(struct CodeImplant);

   // form output 3: copy last in-data data (0001 to end)
   last_indata = p_zero_buf[2] - p_zero_buf[1] + 1;
   memcpy(pointer, (uint8_t *)p_zero_buf[1], last_indata);

   return total_size;

  err:
    return -1;
}
