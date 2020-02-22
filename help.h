#define BUFLEN		1800	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	10	// numarul maxim de clienti in asteptare

typedef struct {

	char topic[50];
	uint8_t type;
	char content[1500];

} udpMsj;

typedef struct {

	unsigned short port;
	char ip_addr[25];
	udpMsj msj;

} udpInfo;

typedef struct {

	char topicName[50];
	udpInfo* msj;  // stochez mesajele in cazul in care clientul deconectat
	int nrMsj;
	int SF;

} topic;

typedef struct {

	int connected; // 0 - off, 1 - on
	int socketId;
	int nrSubs;
	char clientId[10];
	topic* subscriptions;  // lista de abonamente

} Client;