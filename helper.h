#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include<math.h>

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

// struct for messages
typedef struct Msg {
	char topic[50];
	uint8_t tip_date;	
	char mes[1501]; 
} Msg;

/* information about udp client:
 udp port and ip address */
typedef struct Info {
	char info_str[16];
} Info;

// struct for topics
typedef struct {
	char name[50];
 	int SF;
 	int nr_mess;
 	Msg *messg;
 	Info *info;
} Topic;

// struct for clients
typedef struct {
	int connect; 
	char id[10];
	int nr_topics;
	Topic *topics;
} Client;

