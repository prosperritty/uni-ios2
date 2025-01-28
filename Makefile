all: proj2

proj2: proj2.c
	gcc proj2.c -o proj2 -std=gnu99 -Wall -Wextra -Werror -pedantic -pthread -lrt
	
