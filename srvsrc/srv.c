#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "common.h"
#include "crc32c.h"

uint32_t g_tot_size = 0;
uint32_t g_tot_num = 0;
uint32_t g_tot_err_num = 0;
uint32_t g_first_pkt_rcvd = 0;
uint32_t g_sec = 0;

void int_hndlr()
{
	float ratio;

	if (g_tot_num && g_sec)
		ratio = ((float)g_tot_err_num) / g_tot_num * 100;
	else {
		printf("\nerr out!\n");
		exit(-1);
	}
	printf("\nSummary:\n");
	printf("  tot size = %uB, tot # = %up\n", g_tot_size, g_tot_num);
	printf("  tot time = %u secs, avg. speed = %uKbps\n", g_sec, g_tot_size/g_sec*8/1000);
	printf("  tot err = %up, err ratio= %5.2f%%\n", g_tot_err_num, ratio);

	exit(0);
}

void init_sighup_mask()
{
	signal (SIGINT, (void *) int_hndlr);
	signal (SIGTERM, (void *) int_hndlr);
	signal (SIGQUIT, (void *) int_hndlr);
}

static void *recv_thread(void *arg)
{
	static char buf[2000];
	static tch_t *hdr = (tch_t *)buf;
	int sockfd;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t addr_size;
	int len;
	uint32_t seq_exp = 0;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	memset(&serv_addr, '\0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

	printf("waiting for first pkt...\n");
try_again:
	addr_size = sizeof(cli_addr);
	len = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&cli_addr, &addr_size);
	if (len <= 0) {
		perror("recvfrom");
		exit(-1);
	}
	g_tot_size = len + 8 + 20 + 14;	// record from the first pkt
	++g_tot_num;

	printf("\tfrom %s:%u\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
	if (ntohs(hdr->dlen) != len - sizeof(tch_t)) {
		printf("\tdata size mismatch: get %u, exp %u, seq=%u\n", ntohs(hdr->dlen), (uint32_t)(len - sizeof(tch_t)), ntohl(hdr->seq));
		++g_tot_err_num;
		++seq_exp;
		goto try_again;
	}
	if (crc32c((uint8_t *)(buf + sizeof(uint32_t)), len - sizeof(uint32_t)) != hdr->chksum) {
		printf("\tcrc err\n");
		++g_tot_err_num;
		++seq_exp;
		goto try_again;
	}
	g_tot_num = ntohl(hdr->seq)+1;
	if (ntohl(hdr->seq) != seq_exp) {
		int k = ntohl(hdr->seq) - seq_exp;
		printf("\tseq err: get %u, exp %u\n", ntohl(hdr->seq), seq_exp);

		if (k > 0)
			g_tot_err_num += k;
		else
			++g_tot_err_num;
		seq_exp = ntohl(hdr->seq);
	}
	printf("\tstart seq: %u\n", seq_exp);
	++seq_exp;
	printf("...done\n");
	g_first_pkt_rcvd = 1;

	while (1) {
		len = recv(sockfd, buf, sizeof(buf), 0);
		if (len <= 0) {
			perror("recv");
			continue;
		}
		g_tot_size += len + 8 + 20 + 14;
		++g_tot_num;
		if (ntohs(hdr->dlen) != len - sizeof(tch_t)) {
			printf("pkt size mismatch: get %u, exp %u, seq=%u\n", ntohs(hdr->dlen), (uint32_t)(len - sizeof(tch_t)), ntohl(hdr->seq));
			++g_tot_err_num;
			++seq_exp;
			continue;
		}
		if (crc32c((uint8_t *)(buf + sizeof(uint32_t)), len - sizeof(uint32_t)) != hdr->chksum) {
			printf("crc err\n");
			++g_tot_err_num;
			++seq_exp;
			continue;
		}
		if (ntohl(hdr->seq) != seq_exp) {
			int k = ntohl(hdr->seq) - seq_exp;

			printf("seq err: get %u, exp %u\n", ntohl(hdr->seq), seq_exp);
			if (k > 0) {
				g_tot_err_num += k;
				g_tot_num += k;
			}
			else
				++g_tot_err_num;
			seq_exp = ntohl(hdr->seq);
		}
		++seq_exp;
	}

	return NULL;
}

int main(int argc, char **argv)
{
	pthread_t rthread;

	init_sighup_mask();

	pthread_create(&rthread, NULL, recv_thread, NULL);

	while (1) {
		static uint32_t old_size = 0;
		static uint32_t old_num = 0;
		static uint32_t old_err_num = 0;

		while (!g_first_pkt_rcvd)
			usleep(1000);

		sleep(1);
		if (g_tot_num - old_num) {
			float ratio;
			ratio = ((float)(g_tot_err_num - old_err_num)) / (g_tot_num - old_num) * 100;
			printf("%d: last sec(# = %up, rate = %uKbps, err = %up, ratio = %5.2f%%)\n", ++g_sec, g_tot_num - old_num, (g_tot_size - old_size)*8/1000, g_tot_err_num - old_err_num, ratio);
		}
		else
			printf("%d: last sec(# = %up, rate = %uKbps, err = %up, ratio = N/A)\n", ++g_sec, g_tot_num - old_num, (g_tot_size - old_size)*8/1000, g_tot_err_num - old_err_num);
		old_size = g_tot_size;
		old_num = g_tot_num;
		old_err_num = g_tot_err_num;
	}

	return 0;
}
