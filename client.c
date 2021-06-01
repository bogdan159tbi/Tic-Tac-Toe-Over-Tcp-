#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "helpers.h"
#define LOCAL_HOST "127.0.0.1"
void usage(char *file)
{
	fprintf(stderr, "Usage: %s  server_port\n", file);
	exit(0);
}

int main(int argc, char *argv[])
{
	int sockfd, n, ret;
	struct sockaddr_in serv_addr;
	char buffer[BUFLEN];
	
	char *matrix = calloc(9, sizeof(char));
	DIE(!matrix, "matrix null\n");
	
	size_t len = sizeof(matrix) + 2;
	if (argc < 2) {
		usage(argv[0]);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	//ret = inet_aton(argv[1], &serv_addr.sin_addr);
	//DIE(ret == 0, "inet_aton");
	serv_addr.sin_addr.s_addr = inet_addr(LOCAL_HOST);
	ret = connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr));
	DIE(ret < 0, "connect");
	fd_set read_fds;
	fd_set tmp_fds;
	int fdmax;

	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	FD_SET(STDIN_FILENO, &read_fds);
	fdmax = (fdmax < STDIN_FILENO) ? STDIN_FILENO : fdmax;
	int moved = 0;
	while (1) {
		tmp_fds = read_fds;
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0 , "select");
		for(int i = 0; i <= fdmax ; i++){
			if (FD_ISSET(i, &tmp_fds)){
				if (i == sockfd){
					//a primit mesaj de la server 
					memset(buffer, 0 , BUFLEN);
					ret = recv(i, buffer, BUFLEN, 0);
					DIE(ret < 0,"recv from server\n");
					if (ret == 0){
						//s a inchis socket de la server
						printf("server closed\n");
						FD_CLR(sockfd, &read_fds);
						return -1;
					}
					printf("received from server: %s\n", buffer);

				} else if (i == STDIN_FILENO){
					//a primit de la tastatura
					// se citeste de la tastatura
					// se trimite mesaj la server
					
					memset(buffer, 0, BUFLEN);
					fgets(buffer, BUFLEN - 1, stdin);
					if (strncmp(buffer, "exit", 4) == 0) {
						break;
					}
					n = send(sockfd, buffer, strlen(buffer), 0);
					DIE(n < 0, "send");
				}
			}
		}
  		
	}

	close(sockfd);

	return 0;
}
