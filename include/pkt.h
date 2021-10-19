#ifndef __PKT_H__
#define __PKT_H__

#include <stdint.h>
#include <netinet/in.h>
#include "list.h"

#define MAX_LENGTH	2000

// PTR is a ptr of type TYPE, return the whole packet size(size of hdr + payload)
#define TOTAL_SIZE(PTR, TYPE) (PTR->len + sizeof(TYPE))
// PTR is a ptr of type TYPE, return PTR's payload length
//#define PAYLOAD_LEN(PTR, TYPE) (PTR->len - sizeof(TYPE))
// PHDR is a pointer of type ctrl_hdr_t
#define FILL_CTRL_HDR(PHDR, TYPE, LEN) {		\
			PHDR->ver = 1;		\
			PHDR->type = TYPE;		\
			PHDR->len = htons(LEN);		\
		}

enum {
	IPGW_NONE,
	IPGW_PACKET_IN,		// dp --> cp
	IPGW_RULE_ADD,		// dp <-- cp
	IPGW_RULE_RM,		// dp <-- cp
	IPGW_SERVICE,		// dp <-> cp
};
typedef struct {
	uint8_t ver;
	uint8_t type;
	uint16_t len;
}__attribute__((packed))ctrl_hdr_t;

/*top pkt struct between dp & cp
  dp <-> cp*/
/*IPGW_PACKET_IN
  dp --> cp*/
typedef struct {
	uint32_t buf_id;
	uint16_t len;
	uint16_t in_port;
	uint16_t reason;
	uint8_t pad[2];
	uint8_t data[0];
}__attribute__((packed))pkt_in_t;

/*IPGW_RULE
  dp <-- cp*/
#define HTONL(x)		x=htonl(x)
#define HTONS(x)		x=htons(x)
#define NTOHL(x)		x=ntohl(x)
#define NTOHS(x)		x=ntohs(x)
#define PREFIX_MATCH_LEN	(sizeof(in_addr_t) + sizeof(uint8_t))
typedef struct {
	in_addr_t ip;
	uint8_t mask;           /* num of mask */
	uint8_t pad[3];
}__attribute__((packed)) prefix_match_t;

typedef struct {
	prefix_match_t match;
	uint16_t site;
	uint8_t pad[2];
}__attribute__((packed)) route_item_t;

typedef struct {
	route_item_t route;
	uint16_t idle_timeout;
	uint16_t hard_timeout;
}__attribute__((packed))rule_t;

/*IPGW_SERVICE
  dp <-> cp*/
// PHDR is a pointer of type service_t
#define FILL_SRV_HDR(SRV, TYPE, LEN) {		\
			SRV->ver = 1;		\
			SRV->type = TYPE;	\
			SRV->len = htons(LEN);	\
		}

enum {
	SRVC_NONE,
	SRVC_RES_REQ,
	SRVC_RES_REL,
	SRVC_ACK,
	SRVC_NOTIFY,
	SRVC_CTRL,
	SRVC_DATA
};
typedef struct {
	uint8_t ver;
	uint8_t type;
	uint16_t len;
	uint8_t pl[0];
}__attribute__((packed))service_t;
typedef struct {
	uint16_t xid;
	uint16_t site;
}__attribute__((packed))res_t;
enum {
	SRVC_RSLT_OK,
	SRVC_RSLT_ERR
};
typedef struct {
	uint8_t result;
	uint8_t pad[1];
	uint16_t xid;
}__attribute__((packed))ack_t;
enum {
	SRVC_NTFY_NONE,
	SRVC_NTFY_LOGOUT,
	SRVC_NTFY_ERR,
};
typedef struct {
	uint16_t site;
	uint8_t type;
	uint8_t pad[1];
}__attribute__((packed))notify_t;
enum {
	SRVC_DATA_NONE,
	SRVC_DATA_UNICAST,
	SRVC_DATA_MULTICAST,
	SRVC_DATA_BROADCAST,
};
typedef struct {
	uint8_t type;
	uint8_t pad[3];
	uint16_t len;
	uint16_t site;
	uint8_t data[0];
}__attribute__((packed))data_t;

typedef struct {
	ctrl_hdr_t hdr;
	union {
		pkt_in_t pkt_in;
		rule_t rule;
		service_t srvc_hdr;
		uint8_t fill[MAX_LENGTH];
	} pl;
}__attribute__((packed))ctrl_frm_t;

#endif /* __PKT_H__ */
