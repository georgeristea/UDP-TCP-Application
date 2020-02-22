#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include "help.h"


void usage(char *file)
{
	fprintf(stderr, "Usage: %s client_id server_address server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];

	if (argc != 4) {
		usage(argv[0]);
	}

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);	

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) 
		printf("socket error!\n");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	if (ret == 0) 
		printf("inet_aton error!\n");

	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	if (ret < 0) 
		printf("Connect error!\n");

	strcpy(buffer, argv[1]);
	n = send(sockfd, buffer, strlen(buffer), 0);
	if (n < 0) {
		printf("send error\n");
	}

	FD_SET(0, &read_fds);
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;

	/*  Dezactivarea algoritmului Neagle */
	int flag = 1;
	int result = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag,
				sizeof(int));
	if (result < 0) {
		printf("NODELAY ERROR !!!\n");
	}

	char* token;
	int ok = 0;
	char buf[BUFLEN];
	while (1) {

		tmp_fds = read_fds;

		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		if (ret < 0) {
			printf("select error in subscrieber!\n");
		}

		if(FD_ISSET(0, &tmp_fds)){
	  		// se citeste de la tastatura
			memset(buffer, 0, BUFLEN);
			fgets(buffer, BUFLEN - 1, stdin);
			// facem o copie a buferului pentru send
			strcpy(buf, buffer);
			
			if (strncmp(buffer, "exit", 4) == 0) {
				break;
			}
			token = strtok(buffer, " ");
			char* token2;
			if (strcmp(token, "subscribe") == 0) {
				token2 = strtok(NULL, " ");
				char* sf = strtok(NULL, " \n");
				if (sf == NULL || (atoi(sf) != 1 && atoi(sf) != 0)) {
					printf("Wrong input...ignore\n");
					ok = 1;
				}
			}
			else if (strcmp(token, "unsubscribe") == 0) {
				token2 = strtok(NULL, " \n");
				if (strtok(NULL, " \n") != NULL) {
					printf("Wrong input...ignore\n");
					ok = 1;
				}
			}
			if (ok != 1) {
				// se trimite mesaj la server
				n = send(sockfd, buf, strlen(buf), 0);
				if (n < 0) {
					printf("send error in subscrieber!\n");
				}
				if (strcmp(token, "subscribe") == 0) {
					printf("subscribed %s\n", token2);
				}
				else if (strcmp(token, "unsubscribe") == 0) {
					printf("unsubscribed %s\n", token2);
				}
				ok = 0;
			}
			ok = 0;
		}
		else if(FD_ISSET(sockfd, &tmp_fds)) {

			udpInfo tosend;
			memset(buffer, 0, BUFLEN);
			n = recv(sockfd, buffer, sizeof(buffer), 0);

			if (strncmp(buffer, "exit", 4) == 0)
				break;
			else {
				memcpy(&tosend, buffer, sizeof(udpInfo));
				printf("%s:%d - ", tosend.ip_addr, tosend.port);
				printf("%s - ", tosend.msj.topic);
				uint8_t type = tosend.msj.type;

				if (type == 0) {
					printf("INT - ");
				}
				else if (type == 1) {
					printf("SHORT_REAL - ");
				}
				else if (type == 2) {
					printf("FLOAT - ");
				}
				else {
					printf("STRING - ");
				}
				printf("%s\n", tosend.msj.content);

			}
		}
	}
	close(sockfd);

	return 0;
}
