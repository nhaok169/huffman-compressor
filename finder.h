#ifndef FINDER_H
#define FINDER_H

#include "decoder.h"

typedef struct {
	unsigned char len, *code; 
}node,*codeptr;


int finder( char *inputf, char *seekword);

#endif
