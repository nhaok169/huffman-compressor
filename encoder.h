#ifndef ENCODER_H
#define ENCODER_H

#include "common.h"

typedef struct{
	unsigned long weight; 
	int lc,rc,par;
}htnode,*htree;

typedef struct {
	unsigned char src;//原字符是什么 
    unsigned char bits[16];  // 128位,128个字符最多127位编码 
    unsigned char len;       //实际码长度(按位计) 
} huffcode;

int encoder(char *inputf, char *outputf);

unsigned char tobyte(unsigned char* src, int len);//len<=8

void prompt2();

#endif
