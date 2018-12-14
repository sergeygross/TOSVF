#ifndef TELEMETRY_ARRAY_
#define TELEMETRY_ARRAY_


#include <stdint.h>

#define MAX_OBJ_NUMBER 12
#define MAX_DESC_SIZE 8

// SEI ADDITION ROUTINE
struct  __attribute__((__packed__)) sei_header_t
{
	uint8_t null_header[4];
	uint8_t F_NRI_TYPE;			// F - 1 byte, NRI - 2 bytes, Type - 5 bytes; MUST BE 6 << 3
	uint8_t payloadType; 		// MUST BE 5;
	uint8_t payloadSize;			// sizeof(sei_header_t) + sizeof(telemetryGSC) - 3;
	uint8_t uuid[16]; 
};


struct __attribute__((__packed__)) _ObjectStruct
{
	uint16_t x_pos; 	
	uint16_t y_pos; 
	uint16_t width; 
	uint16_t height; 	
	uint16_t x_center; 
	uint16_t y_center; 
	uint8_t desc[MAX_DESC_SIZE];				// описание объекта
};


// current size is 244 bytes (256 is max due to uint8_t payloadSize)
struct __attribute__((__packed__)) CodeImplant 
{
	// Заголовок
	uint8_t prefix_byte1; 	  		//  B1
	uint8_t prefix_byte2; 	  		//  B3
	uint8_t prefix_byte3; 	  		//  B5
	uint8_t object_number;
	struct _ObjectStruct ObjectStruct[MAX_OBJ_NUMBER];				// ключ шифрования
};


#endif
