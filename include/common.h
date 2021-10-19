#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>

#define SERV_PORT	6666

typedef struct {
	uint32_t chksum;
	uint32_t seq;
	uint16_t dlen;
	char data[0];
}__attribute__((packed))tch_t;
#define DATA_POS(p)	(((tch_t *)p)->data)

#endif
