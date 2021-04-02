#include "helper.h"
/* Keti Duracoska 322CB */

void usage(char *name) {
	fprintf(stderr, "Usage: %s server_port\n", name);
}

// find the bigger integer
int max(int x, int y) {
	return x > y ? x : y;
}

int is_sub(Client *clients, char *id, char *name, int nr_clients) {
	int i, j; 
	for (i = 0; i < nr_clients; i++) {
		// search for the needed client
		if (strcmp(clients[i].id, id) == 0) {
			for (j = 0; j < clients[i].nr_topics; j++) {
				// search all the topic to which client is sub.
				if (strcmp(clients[i].topics[j].name, name) == 0) {
					// the client is already subs to that topic
					return 1;
				}
			}
		}
	}
	return 0;
}

void subscribe(Client *clients, char* id, char *name, char *SF,
 int nr_clients) {
	int i; 
	for (i = 0; i < nr_clients; i++) {
		// find the client
		if (strcmp(clients[i].id, id) == 0) {
			// set the new information
			strcpy(clients[i].topics[clients[i].nr_topics].name, name);
			clients[i].topics[clients[i].nr_topics].SF = atoi(SF);
			clients[i].topics[clients[i].nr_topics].nr_mess = 0;
			clients[i].topics[clients[i].nr_topics].messg 
			= malloc(sizeof(Msg) * 20000);
			clients[i].topics[clients[i].nr_topics].info
			=  malloc(sizeof(Info) * 20000);
			clients[i].nr_topics++;
			return;
		}
	}
} 

int unsubscribe(Client *clients, char *id, char *name, int nr_clients) {
	int i, j, k; 
	for (i = 0; i < nr_clients; i++) {
		// search for the needed client
		if (strcmp(clients[i].id, id) == 0) {
			for (j = 0; j < clients[i].nr_topics; j++) {
				if (strcmp(clients[i].topics[j].name, name) == 0) {
					// find the topic
					free(clients[i].topics[j].messg);
					free(clients[i].topics[j].info);
					for (k = j; k < clients[i].nr_topics; k++) {
						// delete the topic from the array
						clients[i].topics[k] = clients[i].topics[k + 1];
					}
					clients[i].nr_topics--;
					return 1;
				}
			}
		}
	}
	return 0;
}

Client delete_client(Client *clients, char *id, int nr_clients) {
	int i, j;
	Client aux;
	memset(&aux, 0 , sizeof(Client));
	for (i = 0; i < nr_clients; i++) {
		// search for the  client
		if (strcmp(clients[i].id, id) == 0) {
			aux = clients[i];
			for (j = i; j < nr_clients; j++) {
				// delete the client
				clients[j] = clients[j + 1];
			}
			return aux;
		}
	}
	return aux;
}

void delete_list_clients(Client *clients, int nr) {
	int i, j;
	for (i = 0; i < nr; i++) {
		for (j = 0; j < clients[i].nr_topics; j++) {
			// delete all messages
			free(clients[i].topics[j].messg);
			free(clients[i].topics[j].info);
		}
		// delete all topics
		free(clients[i].topics);
	}
	// delete all clients
	free(clients);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		usage(argv[0]);
		return -1;
	}

	int sock_tcp, sock_udp, fdmax, connect, port;
	int ret, i, j, k;
	int nr_clients = 0, exists = 0, delete = 0, nr_deleted = 0;
	char *command = malloc(100), *id = malloc(10);
	if(command == NULL) {
		return -1;
	}
	if(id == NULL) {
		free(command);
		return -1;
	}
	struct sockaddr_in serv_addr, cli_addr;
	fd_set read_fds, tmp_fds, tcp_fds;
	socklen_t cli_len;

	// arrays of current and deleted clients
	Client *clients = malloc(sizeof(Client) * 3000);
	if (clients == NULL) {
		free(command);
		free(id);
		return -1;
	}

	Client *deleted = malloc(sizeof(Client) * 3000);
	if (deleted == NULL) {
		free(command);
		free(id);
		free(clients);
		return -1;
	}

	Msg *mesg = malloc(sizeof(Msg));
	if (mesg == NULL) {
		free(command);
		free(id);
		free(clients);
		free(deleted);
		return -1;
	}
	// tcp socket
	sock_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sock_tcp < 0, "tcp socket");
	// clean the memory of struct
	bzero(&serv_addr, sizeof(struct sockaddr_in));
	// set the struct elements
	port = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sock_tcp, (struct sockaddr *)&serv_addr,
	sizeof(struct sockaddr));
	DIE(ret < 0, "tpc bind");

	ret = listen(sock_tcp, 1000);
	DIE(ret < 0, "tpc listen");
	// udp socket
	sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sock_udp < 0, "udp socket");
	ret = bind(sock_udp, (struct sockaddr *)&serv_addr,
	sizeof(struct sockaddr));
	DIE(ret < 0, "udp bind");
	// clean the fd_sets
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);
	FD_ZERO(&tcp_fds);
	/* set the sockets file descripotrs and stdin 
	for reading */
	FD_SET(sock_tcp, &read_fds);
	FD_SET(sock_udp, &read_fds);
	FD_SET(0, &read_fds);
	FD_SET(sock_tcp, &tcp_fds);

	fdmax = 1 + max(sock_tcp, sock_udp);

	while (1) {
		memset(command, 0, 100);
		tmp_fds = read_fds;
		// select file descriptor for reading
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");
		for (i = 0; i <= fdmax; i++) {
			memset(mesg, -1, sizeof(Msg));
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sock_tcp) {
					// new tcp client connected
					exists = 0;
					delete = 0;
					cli_len = sizeof(cli_addr);
					connect = accept(sock_tcp, (struct sockaddr *)&cli_addr, 
					&cli_len);
					DIE(connect < 0, "accept");
					// set new file descriptor 
					FD_SET(connect, &read_fds);
					FD_SET(connect, &tcp_fds);

					if (connect > fdmax) { 
						fdmax = connect;
					}
					// disable Neagle algorithm
					int option = 1;
					ret = setsockopt(connect, IPPROTO_TCP, TCP_NODELAY, 
					(char *)&option, sizeof(option));
					DIE(ret < 0, "TCP_NODELAY");

					// get the id of the new client
					recv(connect, id, 10, 0);
					// check if client alredy exits
					for (j = 0; j < nr_clients; j++) {
						if (strcmp(clients[j].id, id) == 0) {
							exists = 1;
							// do not connect the clietn
							fprintf(stderr, "Client with id: %s already exists"
							". Can't be connected.\n", id);
							send(connect, "exit", 5, 0);
							FD_CLR(connect, &tmp_fds);
							FD_CLR(connect, &tcp_fds);
							break;
						}
					}
					// check if it was disconnected
					if (exists == 0) {
						for (j = 0; j < nr_deleted; j++) {
							if (strcmp(deleted[j].id, id) == 0) {
								delete++;
							}
						}
						if (delete == 0) {
							// it is a new client set the information
							strcpy(clients[nr_clients].id, id);
							clients[nr_clients].connect = connect; 
							// new client is not subs to any topic
							clients[nr_clients].nr_topics = 0;
							clients[nr_clients].topics 
							= malloc(sizeof(Topic) * 2000);
							if(clients[nr_clients].topics == NULL) {
								shutdown(sock_udp, 0);
								shutdown(sock_tcp, 0);
								free(command);
								free(id);
								free(mesg);
								delete_list_clients(deleted, nr_deleted); 
								delete_list_clients(clients, nr_clients); 
								return -1;
							}
							nr_clients++;
							fprintf(stdout,"New client %s connected from %s:%d"
							".\n", id, inet_ntoa(cli_addr.sin_addr), 
							ntohs(cli_addr.sin_port));

						} else {
							// actually the old one reconerted.
							fprintf(stdout,"New client %s connected from %s:%d"
							".\n", id, inet_ntoa(cli_addr.sin_addr),
							ntohs(cli_addr.sin_port));
							/* remove it from list of deleted and add it
							 to the list of current clients */
							clients[nr_clients] = delete_client(deleted, 
							id, nr_deleted);
							clients[nr_clients].connect = connect;
							nr_deleted--;
							/* send the messages from the topics with SF = 1
							 which were sent while the clinet was offline */
							for (j = 0; j < clients[nr_clients].nr_topics; j++) {
								for (k = 0; k < clients[nr_clients].topics[j].nr_mess; k++) {
									ret = send(clients[nr_clients].connect,
									&clients[nr_clients].topics[j].messg[k],
									sizeof(Msg), 0);
									if (ret < 0) {
										fprintf(stderr, "Send error.\n");
										return -1;
									}
									// send ip addres and port
									ret = send(clients[nr_clients].connect,
									clients[nr_clients].topics[j].info[k].info_str,
									sizeof(Info), 0);
									if (ret < 0) {
										fprintf(stderr, "Send error.\n");
										return -1;
									}
								}
								
								// all the messages were send
								clients[nr_clients].topics[j].nr_mess = 0;
							}
							nr_clients++;
						}
					}	
				}
				if (i == 0) {
					fgets(command, 5, stdin);
					if (strcmp(command, "exit") == 0) {
						for (j = 0; j < nr_clients; j++) {
							send(clients[j].connect, "exit", 5, 0);
						}
						shutdown(sock_udp, 0);
						shutdown(sock_tcp, 0);
						free(command);
						free(id);
						free(mesg);
						delete_list_clients(deleted, nr_deleted); 
						delete_list_clients(clients, nr_clients); 
						return 0;
					}
					else {
						if (strcmp(command, "\n") == 0) {
							continue;
						}
						fprintf(stderr, "Invalid command\n");
						continue;
					}
				}
				if (FD_ISSET(i, &tcp_fds) && (recv(i, command, 100, 0)) > 0) {
					// message recveived from tcp client 
					char *tokens[30];
					memset(tokens, 0, 29);
					int index = 0;
					char* token = strtok(command, " ");
					while (token != NULL) {
						tokens[index] = token;
						index++;
						token = strtok(NULL, " ");
					}
					if (strcmp(tokens[0], "subscribe") == 0) {
						if (tokens[1] == NULL || tokens[2] == NULL 
							|| tokens[3] == NULL) {
							send(i, "Invalid command", 18, 0);
							continue;
						}
						if (strcmp(tokens[2], "1") != 0
						 && strcmp(tokens[2], "0") != 0) {
						 	send(i, "SF must be 0 or 1", 18, 0);
							continue;
						}
						ret = is_sub(clients, tokens[3], tokens[1], 
							nr_clients);
						if (ret == 1) {
							send(i, "Already subscribed.", 20, 0);
							continue;
						}
						// subscribe the client to the topic
						subscribe(clients, tokens[3], tokens[1], tokens[2], 
						nr_clients);
						fprintf(stdout, "Client with ID: %s subscribed"
							" to the topic: %s \n", tokens[3], tokens[1]);

					} else if (strcmp(tokens[0], "unsubscribe") == 0) {
						if (tokens[1] == NULL || tokens[2] == NULL) {
							send(i, "Invalid command", 18, 0);
							continue;
						}
						ret = unsubscribe(clients, tokens[2], tokens[1], 
							nr_clients);
						if (ret == 1) {
							fprintf(stdout, "Client with ID: %s unsubscribed" 
							" the topic: %s \n", tokens[2], tokens[1]);
						} else {
							fprintf(stdout, "Client with ID: %s was not"
							" subscribed to the topic: %s \n",
							 tokens[2], tokens[1]);
						}
					} else if (strcmp(tokens[0], "exit") == 0) {
						fprintf(stdout, "Client %s disconnected.\n"
						, tokens[1]);
						// delete the client from the list of clients
						deleted[nr_deleted] = delete_client(clients, tokens[1],
						nr_clients);
						// clear the set for reading
						FD_CLR(deleted[nr_deleted].connect, &tmp_fds);
						FD_CLR(deleted[nr_deleted].connect, &tcp_fds);
						nr_deleted++;
						nr_clients--;
					}
					else {
						send(i, "Invalid command", 18, 0);
					}
				}
				if (i == sock_udp && recvfrom(i, mesg, sizeof(Msg), 0,
				(struct sockaddr *)&cli_addr, &cli_len)) {
					char *info = malloc(sizeof(char) * 50);
					if (info == NULL) {
						return -1;
					}
					sprintf(info, "%d" ,ntohs(cli_addr.sin_port));
					strcat(info, " ");
					strcat(info, inet_ntoa(cli_addr.sin_addr));
					// message received from udp client
					for (j = 0; j < nr_clients; j++) {
						for (k = 0; k < clients[j].nr_topics; k++) {
							if (strcmp(clients[j].
								topics[k].name, mesg->topic) == 0) {
								/* find all the subscribed 
								clients and send the message to them */
								send(clients[j].connect, mesg, sizeof(Msg), 0);
								send(clients[j].connect, info, strlen(info), 0);
							}
						}
					}
					Info *info_str = malloc(sizeof(Info));
					if (info_str == NULL) {
						return -1;
					}
					strcpy(info_str->info_str, info);
					/* save the messages for the clients with SF = 1
					on that topic */
					for (j = 0; j < nr_deleted; j++) {
						for (k = 0; k < deleted[j].nr_topics; k++) {
							if (strcmp(deleted[j].topics[k].name, mesg->topic) == 0) {
								if (deleted[j].topics[k].SF == 1) {
									deleted[j].topics[k].messg[deleted[j].
									topics[k].nr_mess] = *mesg;
									deleted[j].topics[k].info[deleted[j].
									topics[k].nr_mess] = *info_str;
									deleted[j].topics[k].nr_mess++;
								}
							}
						}
					}
					free(info);
					free(info_str);
				}
			}
		}
	}
return 0;
}