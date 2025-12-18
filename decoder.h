#ifndef DECODER_H
#define DECODER_H

#include"common.h"

typedef struct decodenode {
    unsigned char ch;           // 字符（仅叶子节点有效）
    struct decodenode *left, *right;   // left=0,right=1
} denode,*deptr;



deptr createnode(); 

void destroy(deptr root);

int decoder( char *inputf, char *outputf);

#endif
