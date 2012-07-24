all:
	gcc -std=c99 -Wall -Wextra -pedantic counter.c -o counter -lrt -lpthread

clean:
	rm counter 
