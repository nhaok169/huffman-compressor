#include "encoder.h"

// 哈夫曼编码压缩函数
// 参数: inputf - 输入文件名, outputf - 输出文件名
// 返回值: 0-成功, 1-失败
int encoder(char *inputf, char *outputf) {
    prompt2(); // 显示程序说明
    
    char *inputfile = inputf, *outputfile = outputf, ch;
    unsigned char tem[128] = {'\0'}, buffer = '\0'; // tem用于临时存储编码,buffer用于比特流缓冲
    htnode codelist[256]; // 哈夫曼树节点数组,索引1-128为叶子节点,129-255为分支节点
    huffcode *hashlist[128] = {NULL}; // 哈希表,快速通过字符ASCII码查找编码
    memset(codelist, 0, sizeof(codelist)); // 初始化节点数组
    
    int firsmall, secsmall, father, child, charnum = 0, counter = 0, i, j;
    unsigned int allchars = 0; // 统计总字符数

    // 打开输入文件
    FILE *input = fopen(inputfile, "r");
    if (input == NULL) {
        printf("错误: 无法打开输入文件 '%s'\n", inputfile);
        return 1;
    }
    
    // 统计字符频次
    while ((ch = getc(input)) != EOF) {
        allchars++; // 总字符数加1
        codelist[ch + 1].weight++; // 权值加1,ch+1是因为索引0作为哨兵
    }
    
    codelist[0].weight = LONG_MAX; // 设置哨兵节点的权值为最大值
    
    // 统计实际需要编码的字符种类数
    for (i = 1; i < 129; i++) {
        if (codelist[i].weight) {
            father = i; // 记录最后一个有频次的字符位置
            charnum++; // 字符种类数加1
        }
    }
    
    // 检查文件是否为空
    if (!charnum) {
        printf("输入文件为空，stop.\n");
        return 1;
    }
    
    // 创建编码表数组
    huffcode bincode[charnum];
    memset(bincode, 0, sizeof(bincode)); // 初始化编码表
    
    // 特殊情况: 只有一个字符
    if (charnum == 1) {
        if (father - 1 < 0 || father - 1 > 127) {
            printf("错误：文件只含字符%c(ascii= %d)且超出0-127范围导致丢失！\n", ch, ch);
            return 1;
        }
        bincode[0].bits[0] = 128; // 编码为10000000
        bincode[0].len = 1; // 编码长度1位
        bincode[0].src = father - 1; // 字符ASCII码
        hashlist[father - 1] = &bincode[0]; // 添加到哈希表
        goto printcodelist; // 跳转到打印编码表
    }
    
    // 构建哈夫曼树(从129开始作为分支节点)
    for (i = 129; i < 129 + charnum - 1; i++) {
        firsmall = secsmall = 0; // 初始化最小和次小节点索引为哨兵
        // 查找权值最小的两个节点
        for (j = 1; j < i; j++) {
            if (codelist[j].weight && !codelist[j].par) { // 有频次且无父节点
                if (codelist[j].weight < codelist[secsmall].weight) {
                    if (codelist[j].weight < codelist[firsmall].weight)
                        firsmall = j; // 更新最小节点
                    else
                        secsmall = j; // 更新次小节点
                }
            }
        }
        // 构建新节点
        codelist[firsmall].par = i; // 设置最小节点的父节点
        codelist[secsmall].par = i; // 设置次小节点的父节点
        codelist[i].lc = firsmall; // 设置左孩子
        codelist[i].rc = secsmall; // 设置右孩子
        codelist[i].weight = codelist[firsmall].weight + codelist[secsmall].weight; // 权值为子节点权值和
    }
    
    // 从叶子节点回溯到根节点生成编码
    for (i = 1; i < 129; i++) {
        if (!codelist[i].weight) continue; // 跳过无频次的字符
        
        memset(tem, 0, sizeof(tem)); // 清空临时数组
        father = codelist[i].par; // 获取父节点
        child = i; // 当前子节点
        firsmall = 0, secsmall = 0; // firsmall记录编码长度,secsmall记录编码位
        
        // 回溯生成编码(反向)
        do {
            if (codelist[father].rc == child) // 如果是右孩子
                tem[firsmall] = (unsigned char)1; // 编码为1
            firsmall++; // 编码长度加1(左孩子默认为0)
            father = codelist[father].par; // 向上回溯
            child = codelist[child].par; // 向上回溯
        } while (father); // 直到根节点(根节点的父节点为0)
        
        // 保存编码信息
        bincode[counter].len = firsmall; // 编码长度
        bincode[counter].src = i - 1; // 字符ASCII码
        hashlist[i - 1] = &bincode[counter]; // 添加到哈希表
        
        // 将编码位转换为字节存储(每8位一字节)
        for (secsmall = 0; firsmall >= 0; firsmall -= 8) {
            unsigned char tembyte;
            if (!((firsmall - 1) / 8)) // 最后一组不足8位
                tembyte = tobyte(tem, firsmall); // 转换剩余的位
            else // 完整的一组8位
                tembyte = tobyte(&tem[firsmall - 8], 8); // 转换8位
            bincode[counter].bits[secsmall++] = tembyte; // 存储字节
        }
        
        counter++; // 下一个编码表条目
    }
    
// 打印编码表
printcodelist:
    printf("总共需要编码 %d 个字符\n", charnum);
    for (i = 0; i < charnum; i++) {
        printf("字符 '%c'(ASCII 0x%x): 编码长度=%d位: ", bincode[i].src, bincode[i].src, bincode[i].len);
        int bytes = (bincode[i].len + 7) / 8, bit = 0; // 计算字节数
        // 打印二进制编码
        for (int b = 0; b < bytes; b++)
            for (int p = 7; p >= 8 - ((b == bytes - 1) ? (bincode[i].len - b * 8) : 8); p--)
                printf("%d", (bincode[i].bits[b] >> p) & 1);
        printf(" (0x");
        // 打印十六进制编码
        for (j = 0; j < (bincode[i].len + 7) / 8; j++)
            printf("%x", bincode[i].bits[j]);
        printf(")\n");
    }
    
    // 创建输出文件
    FILE *output = fopen(outputfile, "wb");
    if (output == NULL) {
        printf("错误: 无法创建输出文件 '%s'\n", outputfile);
        fclose(input);
        return 1;
    }
    
    // 写入文件头
    fputc(int('H'), output); // 魔数'H'
    fputc(int('U'), output); // 魔数'U'
    fputc(int('F'), output); // 魔数'F'
    fputc(int('F'), output); // 魔数'F' -> "HUFF"
    
    fwrite(&allchars, 4, 1, output); // 写入原始文件大小(4字节)
    fputc(charnum, output); // 写入字符种类数(1字节)
    
    // 写入编码表
    for (i = 0; i < 128; i++) {
        if (hashlist[i]) {
            fputc(int(hashlist[i]->src), output); // 字符ASCII码
            fputc(int(hashlist[i]->len), output); // 编码长度
            // 写入编码数据
            for (j = 0; j < (hashlist[i]->len + 7) / 8; j++)
                fputc(hashlist[i]->bits[j], output);
        }
    }
    
    // 重新读取输入文件并进行编码压缩
    buffer = '\0';
    counter = 0;
    rewind(input); // 回到文件开头
    
    // 逐字符编码
    while ((ch = getc(input)) != EOF) {
        if (ch < 0 || ch > 127) { // 检查字符是否在0-127范围内
            printf("错误：字符%c(ascii= %d)超出0-127范围导致丢失！\n", ch, ch);
            continue; // 跳过无效字符
        }
        
        // 处理当前字符的编码
        for (i = 0; i < (hashlist[ch]->len + 7) / 8; i++) { // 遍历编码字节
            if (hashlist[ch]->len - i * 8 + counter < 8) { // 剩余位不足一个字节
                buffer += hashlist[ch]->bits[i] >> counter; // 添加到缓冲区
                counter = hashlist[ch]->len - i * 8 + counter; // 更新计数器
                break; // 一定是最后不足8位,退出循环
            }
            
            buffer += hashlist[ch]->bits[i] >> counter; // 添加当前字节到缓冲区
            fputc(int(buffer), output); // 输出完整字节
            buffer = '\0'; // 清空缓冲区
            buffer += hashlist[ch]->bits[i] << (8 - counter); // 处理剩余位
            
            // 更新计数器
            if ((hashlist[ch]->len + counter - 8 * i) >= 8) {
                if (hashlist[ch]->len + counter - 8 * (i + 1) < counter)
                    counter = (hashlist[ch]->len + counter - 8 * i) % 8;
                // 否则counter保持不变
            }
        }
    }
    
    // 处理缓冲区中剩余的位
    if (counter)
        fputc(int(buffer), output);
    
    // 计算压缩率
    fseek(input, 0, SEEK_END);
    long original_size = ftell(input); // 获取原始文件大小
    fseek(output, 0, SEEK_END);
    long compressed_size = ftell(output); // 获取压缩文件大小
    
    printf("\n====== 压缩结果 ======\n");
    printf("原始文件大小: %ld 字节\n", original_size);
    printf("压缩文件大小: %ld 字节\n", compressed_size);
    printf("压缩率: %.2f%%(小文件由于要存编码表，可能导致压缩率<0)\n", (1 - (double)compressed_size / original_size) * 100);
    
    // 关闭文件
    fclose(input);
    fclose(output);
    
    return 0;
}

// 将位数组转换为字节
// 参数: src - 位数组指针, len - 位数组长度
// 返回值: 转换后的字节
unsigned char tobyte(unsigned char* src, int len) {
    unsigned char buffer = '\0'; // 初始化缓冲区
    
    // 将每位放入字节的对应位置
    for (int i = 0; i < len; i++) {
        buffer += src[i] << (8 - len + i); // 高位对齐
    }
    
    return buffer; // 返回转换后的字节
}

// 显示程序说明信息
void prompt2() {
    printf("哈夫曼编码压缩程序\n");
    printf("本程序仅支持标准ASCII文本文件（0-127）\nUTF-8或其他编码的文件可能无法正确处理\n");
    printf("输出文件格式:\n[4字节] 魔数: \"HUFF\" (0x48554646)\n");
    printf("[4字节] 原始文件大小：M个字符(只支持4GB内,小端法，eg 0100 0000 为1个字符 )\n");
    printf("[1字节] 字符种类数 N\n");
    printf("[编码表] N个条目，每个：\n");
    printf("[1字节]字符+[1字节]编码长度(bits)+[X字节]编码数据((len+7)/8字节，高位对齐)\n");
    printf("[压缩数据] 哈夫曼编码的bits流\n");
}

