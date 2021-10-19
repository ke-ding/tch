#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "common.h"
#include "crc32c.h"
#include "random.h"

#define BASE_MASK		0xffff
#define BASE_NUM		(BASE_MASK + 1)
#define MAX_DATA_LEN		(1500 - 14 - 20 - 8)

#define MS_EACH_SLOT		2
#define ADJACENT_SLOTS		20

int sockfd;
uint32_t rl_max_size; 
uint32_t g_tot_size = 0;
uint32_t g_tot_num = 0;
uint32_t g_sec = 0;

void int_hndlr()
{       
	printf("\nSummary:\n");
	printf("  tot size = %uB, tot # = %up\n", g_tot_size, g_tot_num);
	printf("  tot time = %u secs, avg. speed = %uKbps\n", g_sec, g_tot_size/g_sec*8/1000);

	exit (-1);
}

void init_sighup_mask()
{       
	signal (SIGINT, (void *) int_hndlr);
	signal (SIGTERM, (void *) int_hndlr);
	signal (SIGQUIT, (void *) int_hndlr);
}       

static void *send_thread(void *arg)
{
	static char patten_buf[BASE_NUM+MAX_DATA_LEN];
	uint32_t rl_tot_sent = 0;
	uint32_t this_time_tot = 0;
	uint32_t rt_tot_in_slot[ADJACENT_SLOTS];
	uint8_t rt_slot_index = 0;    
	int t_s=0;
	int t_ms=0;
	struct timeval _tv;
	uint32_t seq = 0;
	char do_recheck = 1;

	bzero(rt_tot_in_slot, sizeof(rt_tot_in_slot));
	random_bytes(patten_buf, sizeof(patten_buf));

	gettimeofday(&_tv, NULL);
	t_s = _tv.tv_sec;
	t_ms = _tv.tv_usec / 1000;

	while (1) {
		int ms;

		gettimeofday(&_tv, NULL);
		ms = _tv.tv_usec / 1000;

		do_recheck = 1;
		if ((_tv.tv_sec != t_s) || (ms - t_ms >= MS_EACH_SLOT)) {
			int intvl = (_tv.tv_sec - t_s)*1000 + ms - t_ms;
			if (intvl >= MS_EACH_SLOT) {
				uint8_t oldest = (rt_slot_index + 1) % ADJACENT_SLOTS;        // next/oldest

				rl_tot_sent -= rt_tot_in_slot[oldest];
				rt_tot_in_slot[oldest] = 0;
				rt_slot_index = oldest;       // point to the next one
				t_s = _tv.tv_sec;
				t_ms = ms;

				do_recheck = 0;
			}
		}
		if (do_recheck) {
			usleep(200);
			continue;
		}

		this_time_tot = 0;
		while ((rl_tot_sent < rl_max_size) && (this_time_tot < (rl_max_size / ADJACENT_SLOTS))) {
			static char buf[sizeof(tch_t) + MAX_DATA_LEN];
			static tch_t *hdr = (tch_t *)buf;
			int start = random_uint32() & BASE_MASK;
			int len = random_uint32() % MAX_DATA_LEN;
			int r;

			if (len + 8 + 20 + 14 > rl_max_size - rl_tot_sent)
				len = rl_max_size - rl_tot_sent - 8 - 20 - 14;
			if (len < 0)
				break;
			hdr->seq = htonl(seq ++);
			hdr->dlen = htons(len);
			memcpy(DATA_POS(buf), patten_buf + start, len);
			hdr->chksum = crc32c((uint8_t *)(buf + sizeof(uint32_t)), len + sizeof(tch_t) - sizeof(uint32_t));
			len += sizeof(tch_t);

			r = send(sockfd, buf, len, 0);
			if (r != len) {
				perror("send");
				printf("(MILO)[%s %d] write err, ret=%d, exp=%d!\n", __FILE__, __LINE__, r, len);
			}
			len += 8 + 20 + 14;	// udp hdr + ip hdr + mac hdr
			rt_tot_in_slot[rt_slot_index] += len;
			rl_tot_sent += len;
			this_time_tot += len;
			g_tot_size += len;
			++g_tot_num;
		}

		usleep(1000);
	}

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t sthread;
	int ret;
	struct sockaddr_in serverAddr;

	if(argc != 3) {
		printf("Usage: %s <serv ip> <rate limit in Kbps>\n", argv[0]);
		exit(0);
	}

	init_sighup_mask();

	sockfd = socket(PF_INET, SOCK_DGRAM, 0);
	bzero(&serverAddr, sizeof(serverAddr));

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(SERV_PORT);
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);

	ret = connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (ret) {
		perror("connect");
		exit(-1);
	}

	sscanf(argv[2], "%u", &rl_max_size);
	rl_max_size /= 8;
	// now, rl_max_size is the # of bytes should be sent in every 1 ms

	rl_max_size *= MS_EACH_SLOT * ADJACENT_SLOTS;	// MS_EACH_SLOT ms / slot
	printf("rl_max_size=%u\n", rl_max_size);

	pthread_create(&sthread, NULL, send_thread, NULL);
	printf("\nstart send random size random length pkts to %s:%u...\n\n", argv[1], SERV_PORT);

	while (1) {
		static uint32_t old_size = 0;
		static uint32_t old_num = 0;
		static uint32_t sec = 0;

		sleep(1);
		printf("%d: tot(size = %uB, # = %up), last sec(# = %up, rate = %uKbps)\n", ++sec, g_tot_size, g_tot_num, g_tot_num - old_num, (g_tot_size - old_size)*8/1000);
		old_size = g_tot_size;
		old_num = g_tot_num;
		++g_sec;
	}

	return 0;
}
