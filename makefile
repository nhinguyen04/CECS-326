all: fish pellet SWIM_MILL
fish: fish.c
	gcc -o fish fish.c -pthread
pellet: pellet.c
	gcc -o pellet pellet.c -pthread
SWIM_MILL: SWIM_MILL.c
	gcc -o SWIM_MILL SWIM_MILL.c -pthread
