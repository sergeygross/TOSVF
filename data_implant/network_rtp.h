#ifndef STC_MRTP_ARRAY_
#define STC_MRTP_ARRAY_

#include <stdint.h>

#define DEBUG_MRTP 1

struct __attribute__((__packed__)) mrtpGSC // пакет телеметрии в потоке h264 с камеры на ГСК
{
	// Заголовок
	uint8_t prefix_byte1; 	  		//  A5
	uint8_t prefix_byte2; 	  		//  75
	uint8_t prefix_byte3; 	  		//  3A

	uint16_t package_length; 	  		// длина SDP
	uint8_t data[SDP_MAX_SIZE];     // SDP

	uint32_t CRC32; 				// CRC32
};

#endif	// STC_MRTP_ARRAY_
