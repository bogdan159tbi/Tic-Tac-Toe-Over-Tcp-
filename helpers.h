#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 *     int fd = open(file_name, O_RDONLY);
 *     DIE(fd == -1, "open failed");
 */

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)
#define BOARD_LEN 3

void show_matrix(char *matrix){
	int index = 0;
	for(int i = 0 ; i < BOARD_LEN; i++){
		for(int j = 0 ; j < BOARD_LEN; j++){
			if(matrix[index] == '*')
				printf("* ");
			else if(matrix[index] == 1)
				printf("X ");
			else
				printf("0 ");
			index++;
		}
		printf("\n");
	}
}
#define BUFLEN		256	// dimensiunea maxima a calupului de date
#define MAX_CLIENTS	5	// numarul maxim de clienti in asteptare
#endif
