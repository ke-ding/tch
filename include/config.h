#include <inttypes.h>
#define USE_LINKER_SECTIONS 1
#define VERSION "0.00.1"
//#define HAVE_NETLINK 1
#define IP_FMT "%"PRIu32".%"PRIu32".%"PRIu32".%"PRIu32
#define IP_ARGS(ip)                             \
    ntohl(ip) >> 24,                            \
    (ntohl(ip) >> 16) & 0xff,                   \
    (ntohl(ip) >> 8) & 0xff,                    \
    ntohl(ip) & 0xff
//#define coverage_init(x)
//#define coverage_clear(x)
//#define coverage_log(x)
#define OFP10_VERSION 0x1
#define OFP_SSL_PORT  6633
#define HAVE_MLOCKALL

#define TUN_DEV_NAME			"sat_tun"
#define TUN_BUF_SIZE			2048
#define HEADROOM			64
#define SERVICE_DEFAULT_MYPORT		6666
#define SERVICE_DEFAULT_PORT		6667
#define SRVC_BUF_SIZE			2048

#define SRVC_TIMEOUT			(5 * 1000)	// in ms
#define FLOW_MAX_BUFFERED_NUM		500
#define FLOW_HASH_MAP_TIMER_INTERVAL	(1 * 1000)	// in ms
