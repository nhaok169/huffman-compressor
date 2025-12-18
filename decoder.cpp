
#include "decoder.h"

// 显示程序说明信息
void prompt() {
    printf("哈夫曼解码程序\n");
    printf("本程序仅支持解码出标准ASCII文本文件（0-127）\nUTF-8或其他编码的文件可能无法正确处理\n");
    printf("输入文件格式:\n[4字节] 魔数: \"HUFF\" (0x48554646)\n");
    printf("[4字节] 原始文件大小：M个字符(只支持4GB内,小端法，eg 0100 0000 为1个字符 )\n");
    printf("[1字节] 字符种类数 N\n");
    printf("[编码表] N个条目，每个：\n");
    printf("[1字节]字符+[1字节]编码长度(bits)+[X字节]编码数据((len+7)/8字节，高位对齐)\n");
    printf("[压缩数据] 哈夫曼编码的bits流\n");
}

// 哈夫曼解码函数
// 参数: inputf - 输入压缩文件名, outputf - 输出解压文件名
// 返回值: 0-成功, 1-失败
int decoder(char *inputf, char *outputf) {
    prompt(); // 显示程序说明
    
    char *inputfile = inputf, *outputfile = outputf, magic[5] = {'\0'};
    unsigned char src, tem[127] = {'\0'}, ch;
    // tem数组用于存储单个字符的编码位，最坏情况127位
    // temlen标记tem数组中已使用的长度
    unsigned int allchars, charnum, chlen, temlen = 0;
    int i, j;
    
    deptr root = createnode(); // 创建解码树的根节点
    deptr curptr; // 当前解码位置指针
    
    // 打开输入压缩文件
    FILE *input = fopen(inputfile, "rb");
    if (input == NULL) {
        printf("错误: 无法打开输入文件 '%s'\n", inputfile);
        return 1;
    }
    
    // 验证文件魔数
    if (fread(magic, 4, 1, input) != 1 || strcmp(magic, "HUFF")) {
        printf("错误: %s非HUFF压缩格式 \n", inputfile);
        return 1;
    }
    
    // 读取原始文件大小
    fread(&allchars, 4, 1, input);
    
    // 读取字符种类数
    charnum = getc(input);
    
    printf("\n需要解码共%d个%d种字符,编码表:\n", allchars, charnum);
    
    // 构建解码树
    for (i = 0; i < charnum; i++) {
        // 读取字符
        src = getc(input);
        if (src > 127) {
            printf("错误\n", src); // 字符超出ASCII范围
        }
        
        printf("%c(ASCII=0x%x): ", src, src);
        
        // 读取编码长度
        temlen = 0;
        memset(tem, 0, sizeof(tem)); // 清空临时数组
        chlen = getc(input); // 编码长度(位)
        
        // 读取编码数据
        for (j = 0; j < (chlen + 7) / 8; j++) {
            ch = getc(input); // 读取一个字节的编码数据
            // 将字节拆分为位
            split(ch, &tem[temlen], ((j + 1) * 8 > chlen) ? chlen - 8 * j : 8);
            temlen += ((j + 1) * 8 > chlen) ? chlen - 8 * j : 8; // 更新已存储的位数
        }
        
        // 根据编码构建解码树
        temlen = 0;
        curptr = root; // 从根节点开始
        for (temlen = 0; temlen < chlen; temlen++) {
            if (tem[temlen]) { // 编码位为1
                printf("1"); // 打印编码位
                if (!curptr->right) // 如果右孩子不存在
                    curptr->right = createnode(); // 创建右孩子节点
                curptr = curptr->right; // 移动到右孩子
            }
            else { // 编码位为0
                printf("0"); // 打印编码位
                if (!curptr->left) // 如果左孩子不存在
                    curptr->left = createnode(); // 创建左孩子节点
                curptr = curptr->left; // 移动到左孩子
            }
        }
        
        printf("\n"); // 换行
        curptr->ch = src; // 在叶子节点存储字符
    }
    
    // 创建输出文件
    FILE *output = fopen(outputfile, "w");
    if (output == NULL) {
        printf("错误: 无法创建输出文件 '%s'\n", outputfile);
        fclose(input);
        return 1;
    }
    
    // 解码数据
    chlen = 0;
    memset(tem, 0, sizeof(tem)); // 清空临时数组
    
    // 解码所有字符
    for (i = 0; i < allchars; i++) { // i轮解码
        // temlen标记读入位置，chlen标记已解码位置
        // 从根节点开始，沿着编码路径找到叶子节点
        for (curptr = root; curptr->left || curptr->right; chlen = (chlen + 1) % 16) {
            // 每8位读取一次数据
            if (chlen == 0 || chlen == 8) {
                ch = getc(input); // 读取一个字节的压缩数据
                if (chlen) // chlen == 8
                    split(ch, &tem[8], 8); // 存储到tem数组的后8位
                else // chlen == 0
                    split(ch, tem, 8); // 存储到tem数组的前8位
            }
            
            // 根据当前位选择路径
            if (tem[chlen])
                curptr = curptr->right; // 编码位为1，向右走
            else
                curptr = curptr->left; // 编码位为0，向左走
        }
        
        // 到达叶子节点，输出对应字符
        fprintf(output, "%c", curptr->ch);
    }
    
    printf("解码已完成");
    
    // 关闭文件和清理内存
    fclose(input);
    fclose(output);
    destroy(root); // 销毁解码树
    
    return 0;
}

// 销毁解码树(后序遍历)
// 参数: root - 解码树的根节点
void destroy(deptr root) {
    if (root->left)
        destroy(root->left); // 销毁左子树
    if (root->right)
        destroy(root->right); // 销毁右子树
    free(root); // 释放当前节点
}

// 创建新节点
// 返回值: 新创建的节点指针
deptr createnode() {
    deptr p = (deptr)malloc(sizeof(denode)); // 分配内存
    if (!p)
        return p; // 分配失败返回NULL
    p->ch = 0; // 初始化字符为0
    p->left = p->right = NULL; // 初始化左右孩子为NULL
    return p; // 返回新节点指针
}

// 将字节拆分为位数组
// 参数: ch - 要拆分的字节, tem - 存储位的数组, chlen - 要拆分的位数
void split(unsigned char ch, unsigned char *tem, int chlen) {
    if (chlen > 8) {
        printf("split error"); // 错误: 拆分位数超过8
        return;
    }
    
    // 将字节的每一位存储到tem数组中
    for (int i = 0; i < chlen; i++) {
        tem[i] = (ch >> (7 - i) & 1); // 从高位到低位提取
    }
}
