#ifndef UTILS_H
#define UTILS_H
#include <stdio.h>

void printhelp();

#endif //UTILS_H



#ifdef DEBUG
	#include <stdio.h>
	#define LOG(x) fprintf(stderr,x);
#else
	#define LOG(x) ;
#endif
