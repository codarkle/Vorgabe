#include "../lib/utils.h"

void printhelp(){
	printf("Usage:\n"
	"-l, --load <filename>\n\tLoads an existing filesystem\n"
	"-c, --create <filename> <size>\n\tCreates a new filesystem with given filename and size (amount of INodes/Blocks)\n"
	"-h, --help\n\tPrint this help\n");
}
