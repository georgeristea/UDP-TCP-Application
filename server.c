#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <stdatomic.h>
#include <math.h>
#include "help.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}
void printTopic(Client c) {

	int i;
	for (i = 0; i < c.nrSubs; i++) {
		printf("\t%s [sf = %d] (nrMsj: %d)\n", c.subscriptions[i].topicName, 
			c.subscriptions[i].SF, c.subscriptions[i].nrMsj);
	}
	printf("\n");
}
void printClients(Client* clients, int nrClients) {


	int i;
	printf("------------------------------\n");
	for (i = 0; i < nrClients; i++) {
		Client cl = clients[i];
		printf("<%s> (socket: %d) (subs: %d) ---> :\n",cl.clientId, cl.socketId,
			 cl.nrSubs);
		printTopic(cl);
	}
	printf("------------------------------\n");
}

int searchClientByID(Client* clients, int nrClients, char id[]) {

	int i;
	for (i = 0; i < nrClients; i++) {
		if (strcmp(clients[i].clientId, id) == 0)
			return i;
	}
	return -1;
}
void searchAndSubscribe(Client* clients, int nrClients, int socketId, char token[], int sf) {

	int i, j;
	for (i = 0; i < nrClients; i++) {
		if (clients[i].socketId == socketId) {
			for (j = 0; j < clients[i].nrSubs; j++) {
				topic s = clients[i].subscriptions[j];
				if (strcmp(s.topicName, token) == 0) {
					clients[i].subscriptions[j].SF = sf;
					return ;
				}
			}
			strcpy(clients[i].subscriptions[j].topicName, token);
			clients[i].subscriptions[j].SF = sf;
			clients[i].nrSubs++;
			break;
		}
	}
}

int main(int argc, char *argv[])
{
	int maxClients = 50, maxSubscription = 25;
	int maxInfoStored = 25;
	int noClient = 0;
	int sock_tcp, newsock_tcp, portno;
	int sock_udp;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc != 2) {
		usage(argv[0]);
	}

	// Initial aloc memorie pentru a retine informatii despre 50 de clienti

	Client* clients = (Client*) malloc(maxClients * sizeof(Client));
	if (clients == NULL) {
		printf("[ERROR]Cannot allocate enough memory!\n");
		return -1;
	}

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_tcp < 0) {
		perror("TCP socket error!\n");
		return -1;
	}

	sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_udp < 0) {
		perror("UDP socket error!\n");
		return -1;
	}

	portno = atoi(argv[1]);
	if (portno == 0) {
		perror("port error!\n");
		return -1;
	}

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sock_tcp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	if (ret < 0) {
		perror("TCP socket error! [BINDING]\n");
		return -1;
	}

	ret = bind(sock_udp, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	if (ret < 0) {
		perror("UDP socket error! [BINDING]\n");
		return -1;
	}

	ret = listen(sock_tcp, MAX_CLIENTS);
	if (ret < 0) {
		perror("listen ERROR! \n");
		return -1;
	}

	FD_SET(sock_tcp, &read_fds);
	FD_SET(sock_udp, &read_fds);
	// trebuie sa pun si descriptorul pentru interfata cu utilizatorul
	// inputul de la tastatura ("quit")
	FD_SET(0, &read_fds);

	char buf[BUFLEN];
	char* token;
	int j, k, ok = 1;
	fdmax = sock_tcp;

	while (1) {

		if (ok == 0)
			break;

		tmp_fds = read_fds; 
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		if (ret < 0) 
			printf("select ERROR !\n");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sock_tcp) {
					// a venit o cerere de conexiune pe socketul cu listen
					clilen = sizeof(cli_addr);
					// accept conexiunea in server
					newsock_tcp = accept(sock_tcp, (struct sockaddr *)&cli_addr, &clilen);

					if (newsock_tcp < 0) {
						perror("cannot accept connection!\n");
						return -1;
					} 
					else {

						memset(buffer, 0, BUFLEN);
						n = recv(newsock_tcp, buffer, sizeof(buffer), 0);

						if (n < 0) {
							printf("error receiving <CLIENT ID>\n");
						}
						else {
							strcpy(buf, buffer);
							// se adauga noul socket intors de accept() la multimea
							// descriptorilor de citire
							FD_SET(newsock_tcp, &read_fds);
							int index = searchClientByID(clients, noClient, buffer);


							if (noClient == maxClients) {
								// trebuie sa realoc memorie pentru clienti
								Client* p = realloc(clients, 2 * maxClients * sizeof(Client));
								if (p != NULL) {
									clients = p;
									maxClients = 2 * maxClients;
								}
								else {
									printf("[ERROR] Cannot alocate memory for realloc\n");
								}
							}
							if (index == -1) {
								clients[noClient].connected = 1;
								clients[noClient].socketId = newsock_tcp;
								clients[noClient].nrSubs = 0;
								strcpy(clients[noClient].clientId, buffer);
								topic* subs = (topic*) malloc(maxSubscription * sizeof(topic));
								for (j = 0; j < maxSubscription; j++) {
									subs[j].msj = (udpInfo*) malloc (maxInfoStored * sizeof(udpInfo));
									subs[j].nrMsj = 0;
									if (subs[j].msj == NULL) {
										printf("Memory error [UdpInfo]!\n");
										free(subs);
									} 
								}
								if (subs == NULL) {
									printf("Memory error !\n");
								} 
								else {
									clients[noClient++].subscriptions = subs;
								}
							} 
							else {
								clients[index].socketId = newsock_tcp;
								clients[index].connected = 1;
								// trebuie sa trimit toate mesajele stocate cat timp 
								// a fost deconectat
								Client c = clients[index];
								topic s;
								int k;
								for (j = 0; j < c.nrSubs; j++) {
									s = c.subscriptions[j];
									for (k = 0; k < s.nrMsj; k++) {
										memcpy(buffer, &(s.msj[k]), sizeof(s.msj[k]));
										send(c.socketId, buffer, sizeof(buffer), 0);
									}
									clients[index].subscriptions[j].nrMsj = 0;
								}

							}

							if (newsock_tcp > fdmax) { 
								fdmax = newsock_tcp;
							}
							/*  Dezactivarea algoritmului Neagle */
							int flag = 1;
							int result = setsockopt(newsock_tcp, IPPROTO_TCP, TCP_NODELAY, (char*)&flag,
														sizeof(int));
							if (result < 0) {
								printf("NODELAY ERROR !!!\n");
							}
							printf("New client (%s) connected from %d:%s.\n", buf, 
								ntohs(cli_addr.sin_port), inet_ntoa(cli_addr.sin_addr));
						}

					}
				} 
				else if (i == sock_udp) {

					// am primit mesaj de la un client UDP
					memset(buffer, 0, BUFLEN);
					n = recvfrom(i, buffer, BUFLEN, 0,(struct sockaddr *) &cli_addr, &clilen);

					if (n < 0) {
						printf("[UDP] recv error");
					} 
					else if (n == 0) {
						// clientul UDP inchide brusc conexiunea
						printf("[UDP] connection lost");
						close(i);
						FD_CLR(i, &read_fds);
					}
					else {
						udpInfo tosend;
						udpMsj mesaj;
						uint32_t x;
						uint16_t y;
						uint8_t z;
						memset(&mesaj, 0, sizeof(udpMsj));
						memcpy(&mesaj, buffer, sizeof(udpMsj));

						if (mesaj.type == 0) {
							// daca e INT
							memcpy(&x, &mesaj.content[1], sizeof(uint32_t));
							x = ntohl(x);
							if (mesaj.content[0] == 1) {
								x = 0 - x;
							}
							sprintf(mesaj.content, "%d", x);
						}
						else if (mesaj.type == 1) {
							// daca e SHORT_REAL
							memcpy(&y, &mesaj.content[0], sizeof(uint16_t));
							y = ntohs(y);
							float yy = y / 100.0f;
							sprintf(mesaj.content, "%.2f", yy);

						}
						else if (mesaj.type == 2) {
							int sign = mesaj.content[0];
							memcpy(&x, &mesaj.content[1], sizeof(uint32_t));
							memcpy(&z, &mesaj.content[5], sizeof(uint8_t));
							memset(mesaj.content, 0, sizeof(mesaj.content));
							x = ntohl(x);
							if (sign == 1)
								mesaj.content[0] = '-';
							char val[15];
							sprintf(val, "%d", x);
							if (strlen(val) > z) {
								// am numar supraunitar
								if (z == 0) {
									strcat(mesaj.content, val);
								}
								else {
									strncat(mesaj.content, val, (strlen(val) - z));
									strcat(mesaj.content, ".");
									strcat(mesaj.content, &val[z]);
								}
							}
							else {
								int dif = z - strlen(val);
								mesaj.content[sign] = '0';
								mesaj.content[sign + 1] = '.';
								for (int i = 0; i < dif; i++) {
									mesaj.content[sign + 2 + i] = '0';
								}
								strcat(mesaj.content, val);
							}	
						}

						tosend.port = cli_addr.sin_port;
						strcpy(tosend.ip_addr, inet_ntoa(cli_addr.sin_addr));
						tosend.msj = mesaj;

						memcpy(buffer, &tosend, sizeof(tosend));

						int k, l;
						for (k = 0; k < noClient; k++) {
							Client c = clients[k];
							for (l = 0; l < c.nrSubs; l++) {
								topic s = c.subscriptions[l];
								if (strcmp(s.topicName, mesaj.topic) == 0) {
									// daca e abonat ii trimit mesajul
									if (c.connected == 1) {
										// daca clientul e conectat
										send(c.socketId, buffer, sizeof(buffer), 0);
									}
									else {
										if (s.SF == 1) {
											// daca nu e conectat trebuie sa stochez mesajul 
											// si sa il trimit cand se conecteaza
											int nrMsj = clients[k].subscriptions[l].nrMsj;
											if (nrMsj == maxInfoStored) {
												// trebuie sa aloc mai mult memorie pentru 
												// stocarea mesajelor 
												udpInfo* p = realloc(clients[k].subscriptions[l].msj, 
													2 * maxInfoStored * sizeof(udpInfo));
												if (p == NULL){
													printf("[ERROR] cannot alloc memory for udpInfo\n");
												}
												else {
													clients[k].subscriptions[l].msj = p;
													maxInfoStored = 2 * maxInfoStored;
												}
											}
											clients[k].subscriptions[l].msj[nrMsj] = tosend;
											clients[k].subscriptions[l].nrMsj++;
											// trebuie sa verific daca se depaseste nr. de mesaje 
											// si sa realoc memorie suficienta
										}
									}
									break;
								}
							}
						}
			
					}
				}
				else if (i == 0) {
					// am primit comanda "quit" de la tastatura
					// trimit tuturor clientilor activi faptul ca vreau sa 
					// termin conexiunea si ii fortez si pe ei sa opreasca 
					// conexiunea

					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);

					if (strncmp(buffer, "exit", 4) == 0) {
						for (j = 0; j < noClient; j++) {
							if (clients[j].connected == 1)
								send(clients[j].socketId, buffer, sizeof(buffer), 0);
						}
						ok = 0;
						break;
					} else {
						printf("[WARNING]User input unknown!\n");
					}
				}
				else {
					// s-au primit date pe unul din socketii de client
					// asa ca serverul trebuie sa le receptioneze
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					if (n < 0) {
						printf("recv error!\n");
					}
					if (n == 0) {
						// daca clientul a inchis brusc conexiunea
						char id[10];
						for (j = 0; j < noClient; j++) {
							if (clients[j].socketId == i) {
								// am gasit clientul care a inchis conexiunea
								// ii setez variabila disconnected
								clients[j].connected = 0;
								strcpy(id, clients[j].clientId);
								break;
							}
						}
						printf("Client (%s) disconnected.\n", id);
						close(i);
						FD_CLR(i, &read_fds);
					} else {

						token = strtok(buffer, " ");
						if (strcmp(token, "subscribe") == 0) {
							token = strtok(NULL, " ");
							int sf = atoi(strtok(NULL, " "));
							searchAndSubscribe(clients, noClient, i, token, sf);
						}
						else if (strcmp(token, "unsubscribe") == 0) {

							token = strtok(NULL, " \n");
							for (j = 0; j < noClient; j++) {
								if (clients[j].socketId == i) {
									for (k = 0; k < clients[j].nrSubs; k++) {
										topic s = clients[j].subscriptions[k];
										if (strcmp(s.topicName, token) == 0) {
											int l;
											for (l = k; l < clients[j].nrSubs - 1; l++) {
												clients[j].subscriptions[l] = clients[j].subscriptions[l + 1];
											}
											clients[j].nrSubs--;
											break;
										}
									}
									break;
								}
							}

						}
						else {
							printf("[SERVER]Wrong message received from client!\n");
						}
					}
				}
				//printClients(clients, noClient);
			}
		}
	}
	/* Eliberez memoria alocata */
	for (i = 0; i < maxClients; i++) {
		for (j = 0; j < clients[i].nrSubs; j++) {
			// vectorul de mesaje pentru fiecare abonament
			free(clients[i].subscriptions[j].msj);
		}
		// vectorul de abonamente
		free(clients[i].subscriptions);
	}
	// vectorul de clienti 
	free(clients);


	close(sock_tcp);
	close(sock_udp);

	return 0;
}
