#include "helper.h"

int print_message(Msg *mesg, int sock) {
	if (strcmp(mesg->topic, "exit") == 0) 
		return -1;
	if (strcmp(mesg->topic, "Invalid command") == 0) {
		fprintf(stderr, "%s\n", mesg->topic);
		return 0;
	}
	if (strcmp(mesg->topic, "SF must be 0 or 1") == 0) {
		fprintf(stderr, "%s\n", mesg->topic);
		return 0;
	}
	if (strcmp(mesg->topic, "Already subscribed.") == 0) {
		fprintf(stderr, "Already subscriber.\nIf you "
		"want to change the SF unsubscribe and than "
		"subscribe again with different SF.\n");
		return 0;
	}
	char *info = malloc(sizeof(char) * 50);
	if(info == NULL) {
		return -1;
	}
	char *tokens[2];
	// recv the ip addres and port of udp client
	if (recv(sock, info, sizeof(Info), 0) > 0) {
		int index = 0;
		char* token = strtok(info, " ");
		while (token != NULL) {
			tokens[index] = token;
			index++;
			token = strtok(NULL, " ");
		}
	}
	if (mesg->tip_date == 0) {
		// INT
		uint8_t semn;
		uint32_t int_num;
		memcpy(&semn, mesg->mes, sizeof(uint8_t));
		memcpy(&int_num, mesg->mes + sizeof(uint8_t),
		sizeof(uint32_t));
		if (semn == 0) {
			// POSITIVE
			fprintf(stdout, "%s:%s - %s - INT - %d\n",
			tokens[1], tokens[0], mesg->topic, ntohl(int_num));
		} else if (semn == 1) {
			// NEGATIVE
			fprintf(stdout, "%s:%s - %s - INT - -%d\n",
			tokens[1], tokens[0], mesg->topic,
			ntohl(int_num));
		}
	} if (mesg->tip_date == 1) {
		// SHORT_REAL
		uint16_t real_num;
		memcpy(&real_num, mesg->mes, sizeof(uint16_t));
		double res = (double)(ntohs(real_num)) / 100;
		fprintf(stdout, "%s:%s - %s - SHORT_REAL - %.2f\n",
		tokens[1], tokens[0], mesg->topic, res);
	} if (mesg->tip_date == 2) {
		// FLOAT
		uint8_t semn_float;
		uint32_t float_num;
		uint8_t dec;
		memcpy(&semn_float, mesg->mes, sizeof(uint8_t));
		memcpy(&float_num, mesg->mes + sizeof(uint8_t),
		sizeof(uint32_t));
		memcpy(&dec, mesg->mes + sizeof(uint8_t) 
		+ sizeof(uint32_t), sizeof(uint8_t));
		double power = pow(10, dec);
		power = 1 / power;
		if (semn_float == 0) {
			// POSITIVE
			fprintf(stdout, "%s:%s - %s - FLOAT - %.15g\n", 
			tokens[1], tokens[0], mesg->topic, 
			ntohl(float_num) * power);
		} else if (semn_float == 1) {
				// NEGATIVE
				fprintf(stdout, "%s:%s - %s - FLOAT - -%.15g\n"
				, tokens[1], tokens[0], mesg->topic,
				ntohl(float_num) * power);
		}
	} if (mesg->tip_date == 3) {
		// STRING
		mesg->mes[1500] = '\0';
		fprintf(stdout, "%s:%s - %s - STRING - %s\n",
		tokens[1], tokens[0], mesg->topic, mesg->mes);
	} 
	free(info);
	return 0;
}

void usage(char *name) {
	fprintf(stderr, "Usage: %s subscriber_id server_address server_port.\n"
	, name);
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		usage(argv[0]);
		return -1;
	}
	int sock, ret, fdmax, i;
	char *command = malloc(200);
	if(command == NULL) {
		return -1;
	}
	struct sockaddr_in serv_addr;
	fd_set read_fds;
	fd_set tmp_fds;
	Msg *mesg = malloc(sizeof(Msg));
	if (mesg == NULL) {
		free(command);
		return -1;
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock < 0, "socket");
	// set the struct elements
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[3]));
	ret = inet_aton(argv[2], &serv_addr.sin_addr);
	DIE(ret == 0, "inet_aton");
	ret = connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
	// clear the fd sets 
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	/* set the file descriptor from socket 
	and stdin in read_fds*/
	FD_SET(sock, &read_fds);
	FD_SET(0, &read_fds);
	fdmax = sock;
	// send the clietn id to the server
	send(sock, argv[1], 10, 0);

	while (1) {
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		for (i = 0; i <= fdmax; i++) {
			memset(mesg, -1, sizeof(Msg));	
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == 0 && fgets(command, 200, stdin) > 0) {
					// command from stdin
					if (strcmp(command, "\n") == 0)
						continue;
					command[strcspn(command, "\n")] = '\0';
					strcat(command, " ");
					strcat(command, argv[1]);
					// send the command to the server
					send(sock, command, strlen(command), 0);
					// if command is exit disconnect the client
					if (strcmp(strtok(command, " "), "exit") == 0) {
						shutdown(sock, 0);
						free(command);
						free(mesg);
						return 0;
					}
					memset(command, 0, 200);
				}
				else if (recv(sock, mesg, sizeof(Msg), 0) > 0 ) {
					// message received from the client 
					ret = print_message(mesg, sock);
					if (ret == -1) {
						fprintf(stdout, "Exiting\n");
						shutdown(sock, 0);
						free(command);
						free(mesg);
						return 0;
					}	
				}
			}
		}
	}
	return 0;
}
