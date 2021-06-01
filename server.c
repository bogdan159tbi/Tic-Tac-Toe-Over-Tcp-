#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "helpers.h"

void usage(char *file)
{
	fprintf(stderr, "Usage: %s server_port\n", file);
	exit(0);
}
int get_sockfd(char *buffer){
	char *tok = strtok(buffer," ");
	return atoi(tok);
}
struct Game{
	int player1_sockfd;
	int player2_sockfd;
	char *matrix;
	int turn_sockfd; // sockfd de la player1 sau player2 
	int live; // 0 sau 1
};

struct Game *init_game(){
	struct Game *game;
	game = calloc(1, sizeof(struct Game));
	DIE(game == NULL, "game not allocated\n");

	game->matrix = calloc(BOARD_LEN * BOARD_LEN, sizeof(char));
	DIE(game->matrix == NULL, "matrix is null");

	for(int i = 0 ;i < BOARD_LEN * BOARD_LEN; i++){
		game->matrix[i] = '*';
	}
	game->player2_sockfd = -1;
	game->player1_sockfd = -1;
	game->live = 0;

	return game;
}
int client_left(int **queue, int *nr_clients, int sockfd){
	int index = -1;
	for(int i = 0; i < *nr_clients; i++){
		if ((*queue)[i] == sockfd){
			index = i;
		}
	}
	if(index < 0){
		//nu exista sockfd dat ca parametru
		return -1;
	}
	for(int i = index ; i < *nr_clients -1;i++){
		(*queue)[i] = (*queue)[i+1];
	}
	*nr_clients -= 1;
	return 0;
}
void init_board(char **board){
	for(int i = 0 ;i < BOARD_LEN * BOARD_LEN; i++){
		(*board)[i] = '*';
	}
}
int get_moves(char *buf, int *x ,int *y){
	char *tok;
	tok = strtok(buf," ");
	if(tok[0] < 48 || tok[0] > 51)
		return 0;
	*x = atoi(tok);
	
	tok = strtok(NULL," ");
	if(tok[0] < 48 || tok[0] > 51)
		return 0;

	*y = atoi(tok);

	return 1;
}
void modify_matrix(char **matrix, int x, int y, int value){
	char board[4][4];
	int index = 0;

	for(int i = 0; i < BOARD_LEN; i++){
		for(int j = 0 ; j < BOARD_LEN; j++){
			board[i][j] = (*matrix)[index];
			if(i == x && j == y){
				board[i][j] = value;
			}
			index++;
		}
	}
	index = 0;
	for(int i = 0; i < BOARD_LEN; i++){
		for(int j = 0 ; j < BOARD_LEN; j++){
			(*matrix)[index++] = board[i][j];
		}
	}
}
// A function that returns true if any of the row
// is crossed with the same player's move
int rowCrossed(char board[BOARD_LEN][BOARD_LEN])
{
    for (int i=0; i<BOARD_LEN; i++)
    {
        if (board[i][0] == board[i][1] &&
            board[i][1] == board[i][2] && 
            board[i][0] != '*'){
			if(board[i][0] == 1)
            	return 2;
			else return 1;
			}	
    }
    return 0;
}
  
// A function that returns true if any of the column
// is crossed with the same player's move
int columnCrossed(char board[BOARD_LEN][BOARD_LEN])
{
    for (int i=0; i<BOARD_LEN; i++)
    {
        if (board[0][i] == board[1][i] &&
            board[1][i] == board[2][i] && 
            board[0][i] != '*'){
			if(board[0][i] == 1)
            	return 2;
			else return 1;
		}
    }
    return 0;
}
  
// A function that returns true if any of the diagonal
// is crossed with the same player's move
int diagonalCrossed(char board[BOARD_LEN][BOARD_LEN])
{
    if (board[0][0] == board[1][1] &&
        board[1][1] == board[2][2] && 
        board[0][0] != '*'){
		if(board[1][1] == 1)
        	return 2;
		else return 1;
	}
          
    if (board[0][2] == board[1][1] &&
        board[1][1] == board[2][0] &&
         board[0][2] != '*'){
        if(board[1][1] == 1)
			return 2;
		else return 1;
	}
    return 0;
}
int check_status(char *matrix){
	char board[3][3];
	int index = 0;

	for(int i = 0; i < BOARD_LEN; i++){
		for(int j = 0 ; j < BOARD_LEN; j++){
			board[i][j] = matrix[index];			
			index++;
		}
	}
	if(diagonalCrossed(board) || columnCrossed(board) || rowCrossed(board))
		return 1;

	return 0;
}
int is_valid_move(int x, int y, char *matrix){
	char board[3][3];
	int index = 0;

	for(int i = 0; i < BOARD_LEN; i++){
		for(int j = 0 ; j < BOARD_LEN; j++){
			board[i][j] = matrix[index];			
			if(i == x && j == y){
				if(board[i][j] != '*')
					return 0;
			}
			index++;
		}
	}

	return 1;
}
int game_equal(char *matrix){
	char board[BOARD_LEN][BOARD_LEN];
	int index = 0;
	for(int i = 0; i < BOARD_LEN; i++){
		for(int j = 0 ; j < BOARD_LEN; j++){
			board[i][j] = matrix[index];			
			if(board[i][j] == '*')
				return 0;
			index++;
		}
	}
	return 1;
}
void game_ended(struct Game **game, int **player_queue_sockfd, int *players_online){
	(*game)->live = 0;

	client_left(player_queue_sockfd, players_online, (*game)->player1_sockfd);
	client_left(player_queue_sockfd, players_online, (*game)->player2_sockfd);
	close((*game)->player2_sockfd);
	close((*game)->player1_sockfd);
	
	printf("Game between %d and %d ended.Waiting for new oponents !\n",(*game)->player1_sockfd, (*game)->player2_sockfd);
}
void send_equal(int sockfd){
	char *msg = strdup("You're equal this time!");
	DIE(!msg, "msg eqaul failed to allocate mem\n");
	int ret = send(sockfd, msg, strlen(msg), 0);
	DIE(ret < 0, "sending failed\n");
}
void send_winner(int sockfd){
	char *msg = strdup("Congrats..You won!");
	DIE(!msg, "msg winner failed to allocate mem\n");
	int ret = send(sockfd, msg, strlen(msg), 0);
	DIE(ret < 0, "sending failed\n");
}
void send_looser(int sockfd){
	char *msg = strdup("Sorry..You lost!");
	DIE(!msg, "msg looser failed to allocate mem\n");
	int ret = send(sockfd, msg, strlen(msg), 0);
	DIE(ret < 0, "sending failed\n");
}
void announce_turn(int sockfd){
	char *buffer = strdup("Please select move.It's your turn!");
	int ret = send(sockfd , buffer, strlen(buffer), 0);
	DIE(ret < 0, "send announce failed\n");
}
/*
	1, daca s a terminat meciul
	0 altfel
*/
int make_move(struct Game **game, int x, int y,fd_set *read_fds, int sockfd){
	if((*game)->turn_sockfd == (*game)->player1_sockfd)
		modify_matrix(&(*game)->matrix, x, y, 1);
	else
		modify_matrix(&(*game)->matrix, x, y, 0);
	show_matrix((*game)->matrix);
	int won = check_status((*game)->matrix);
	if(won){
		send_winner((*game)->player1_sockfd);
		send_looser((*game)->player2_sockfd);
		//end game
		FD_CLR((*game)->player1_sockfd, read_fds);
		FD_CLR((*game)->player2_sockfd, read_fds);
		return 1;
	}
	(*game)->turn_sockfd = sockfd;
	announce_turn(sockfd);
	return 0;
}
int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	char buffer[BUFLEN];
	struct sockaddr_in serv_addr, cli_addr;
	int n, i, ret;
	socklen_t clilen;

	fd_set read_fds;	// multimea de citire folosita in select()
	fd_set tmp_fds;		// multime folosita temporar
	int fdmax;			// valoare maxima fd din multimea read_fds

	if (argc < 2) {
		usage(argv[0]);
	}

	// se goleste multimea de descriptori de citire (read_fds) si multimea temporara (tmp_fds)
	FD_ZERO(&read_fds);
	FD_ZERO(&tmp_fds);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	portno = atoi(argv[1]);
	DIE(portno == 0, "atoi");

	memset((char *) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	ret = bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(struct sockaddr));
	DIE(ret < 0, "bind");
	int enable = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) == -1) {
		perror("setsocketopt");
		exit(1);
	}
	ret = listen(sockfd, MAX_CLIENTS);
	DIE(ret < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in multimea read_fds
	FD_SET(sockfd, &read_fds);
	fdmax = sockfd;
	char *list_clients = calloc(BUFLEN, 1);
	strcpy(list_clients,"Friends online: ");
	
	//game implementation
	int *player_queue_sockfd = calloc(MAX_CLIENTS, sizeof(int));
	DIE(!player_queue_sockfd, "players queue null\n");
	int players_online = 0;

	struct Game *game = init_game();
	show_matrix(game->matrix);

	while (1) {
		tmp_fds = read_fds; 
		
		ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
		DIE(ret < 0, "select");

		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &tmp_fds)) {
				if (i == sockfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					clilen = sizeof(cli_addr);
					newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
					DIE(newsockfd < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor de citire
					FD_SET(newsockfd, &read_fds);
					if (newsockfd > fdmax) { 
						fdmax = newsockfd;
					}
					printf("Noua conexiune de la %s, port %d, socket client %d\n",
							inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), newsockfd);

					player_queue_sockfd[players_online++] = newsockfd;
					if (players_online % 2 == 0 && game->live == 0){
						//start game
						game->player1_sockfd = player_queue_sockfd[0];
						game->player2_sockfd = player_queue_sockfd[1];
						game->turn_sockfd = game->player1_sockfd;
						printf("Game started between %d and %d\n", game->player1_sockfd, game->player2_sockfd);
						announce_turn(game->player1_sockfd);
						game->live = 1;
						
					} else if(game->live == 1){
						char *wait_msg = strdup("Game is live.Please wait\n");
						DIE(wait_msg == NULL, "wait msg failed\n");
						ret = send(newsockfd, wait_msg, strlen(wait_msg), 0);
						DIE(ret < 0, "wait msg failed\n");
					}
					
				} else if (i != sockfd){
					
					ret = game_equal(game->matrix);
					if(ret == 1){
						send_equal(game->player1_sockfd);
						send_equal(game->player2_sockfd);
						game_ended(&game, &player_queue_sockfd, &players_online);
					}

					// s au primit indicii pentru o mutare 
					memset(buffer, 0, BUFLEN);
					n = recv(i, buffer, sizeof(buffer), 0);
					DIE(n < 0, "recv");

					if (n == 0) {
						// conexiunea s-a inchis
						printf("Player %d left...\n", i);
						// se scoate din multimea de citire socketul inchis 
						if(i == game->player1_sockfd || i == game->player2_sockfd){
							//game ended but who won ?
							if (i == game->player1_sockfd){
								printf("Game ended...Player %d won\n", game->player2_sockfd);
							} else if (i == game->player2_sockfd){
								printf("Game ended...Player %d won\n", game->player1_sockfd);
							}
							game->live = 0;
							game->player1_sockfd = -1;
							game->player2_sockfd = -1;
							init_board(&game->matrix);
						}
						client_left(&player_queue_sockfd, &players_online, i);

						close(i);
						FD_CLR(i, &read_fds);
					} else {
						
						buffer[strlen(buffer)-1] = '\0';
						//buffer has the indexes from one player
						
						//send board to enemy after being completed	
						int x,y;
						ret = get_moves(buffer, &x, &y);
						if(ret){
							printf("S-au primit de la clientul de pe socketul %d indexii: %d %d\n", i, x, y);
							if(!is_valid_move(x,y, game->matrix)){
								char *msg = strdup("Move is not valid!Try other move!");
								DIE(!msg, "bla\n");
								ret = send(i, msg, strlen(msg), 0);
								DIE(ret < 0, "valid move failed\n");
								if (i == game->player2_sockfd)
									game->turn_sockfd = game->player2_sockfd;
								else 
									game->turn_sockfd= game->player1_sockfd;
							} else {
								if(game->live){
									if (i == game->player1_sockfd ){
										if(game->turn_sockfd == i){
											ret = make_move(&game, x, y, &read_fds, game->player2_sockfd);
											if(ret)
												game_ended(&game, &player_queue_sockfd, &players_online);

										} else {
											memset(buffer, 0, sizeof(buffer));
											strcpy(buffer, "It s not your turn!Please wait...");
											ret = send(i, buffer, strlen(buffer), 0);
											DIE(ret < 0,"announce failed\n");

										}
									} else if (i == game->player2_sockfd ){
										if(game->turn_sockfd == i){
											ret = make_move(&game, x, y, &read_fds, game->player1_sockfd);
											if(ret)
												game_ended(&game, &player_queue_sockfd, &players_online);

										} else {
											memset(buffer, 0, sizeof(buffer));
											strcpy(buffer, "It s not your turn!Please wait...");
											ret = send(i, buffer, strlen(buffer), 0);
											DIE(ret < 0,"announce failed\n");
										}
									}
								}
							}
						} else {
							char *msg = strdup("Index not correct!Try again!");
							DIE(!msg, "bla\n");
							ret = send(i, msg, strlen(msg), 0);
							DIE(ret < 0, "index move failed\n");
							if (i == game->player2_sockfd)
								game->turn_sockfd = game->player2_sockfd;
							else 
								game->turn_sockfd= game->player1_sockfd;
						}
					}
				} 
			}
		}
	}

	close(sockfd);
	free(player_queue_sockfd);

	return 0;
}
