#include "finder.h"
#define MAXR 100000  // 定义最大窗口大小

// 显示程序说明信息
void prompt3() {
    printf("编码查找程序\n");
    printf("本程序仅支持对HUFF压缩的文件进行查找\n");
    printf("输入文件格式:\n[4字节] 魔数: \"HUFF\" (0x48554646)\n");
    printf("[4字节] 原始文件大小：M个字符(只支持4GB内,小端法，eg 0100 0000 为1个字符 )\n");
    printf("[1字节] 字符种类数 N\n");
    printf("[编码表] N个条目，每个：\n");
    printf("[1字节]字符+[1字节]编码长度(bits)+[X字节]编码数据((len+7)/8字节，高位对齐)\n");
    printf("[压缩数据] 哈夫曼编码的bits流\n");
}

// 在哈夫曼压缩文件中查找字符串
// 参数: inputf - 输入压缩文件名, seekword - 要查找的字符串
// 返回值: 0-成功, 1-失败
int finder(char *inputf, char *seekword) {
    prompt3();  // 显示程序说明
    
    char *inputfile = inputf, magic[5] = {'\0'};
    unsigned char src, tem[127] = {'\0'}, *word = NULL, windows[MAXR * 8] = {'\0'};
    int allchars, charnum, chlen, temlen = 0, findtimes = 0, ch;
    // 注意: ch一定要用int，因为getc()返回int类型(0..255或EOF(-1))
    // 使用unsigned char可能导致11111111被误判为EOF
    
    int *goodjump = NULL;  // good跳转表，用于Sunday算法
    int i, j, k, searchlen = 0, badjump[2] = {-1, -1};
    int counter = 0, find1 = 0, find0 = 0;  // counter为已经解码的字符个数
    
    codeptr hashlist[128] = {NULL};  // 哈希表，存储字符编码信息
    deptr root = createnode(), curptr;  // 解码树
    
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
    
    // 读取原始文件大小和字符种类数
    fread(&allchars, 4, 1, input);
    charnum = getc(input);
    printf("\n需要查找共%d个%d种字符,编码表:\n", allchars, charnum);
    
    // 构建搜索列表和解码树
    for (i = 0; i < charnum; i++) {
        // 读取字符
        src = getc(input);
        chlen = getc(input);  // 读取编码长度
        
        if (src > 127) printf("错误\n");  // 字符超出ASCII范围
        
        printf("%c (ASCII=0x%x,编码长度%d): ", src, src, chlen);
        
        // 为当前字符分配编码存储空间
        hashlist[src] = (codeptr)malloc(sizeof(node));
        hashlist[src]->len = chlen;
        hashlist[src]->code = (unsigned char*)malloc(sizeof(unsigned char) * chlen);
        memset(hashlist[src]->code, 0, sizeof(unsigned char) * chlen);
        
        // 读取编码数据
        memset(tem, 0, sizeof(tem));
        temlen = 0;
        for (j = 0; j < (chlen + 7) / 8; j++) {
            ch = getc(input);
            // 将字节拆分为位
            split(ch, &tem[temlen], ((j + 1) * 8 > chlen) ? chlen - 8 * j : 8);
            temlen += ((j + 1) * 8 > chlen) ? chlen - 8 * j : 8;
        }
        
        // 构建解码树并保存编码
        temlen = 0;
        curptr = root;  // 从根节点开始
        for (temlen = 0; temlen < chlen; temlen++) {
            if (tem[temlen]) {  // 编码位为1
                printf("1");
                if (!curptr->right)
                    curptr->right = createnode();  // 创建右孩子节点
                curptr = curptr->right;
                (hashlist[src]->code)[temlen] = 1;  // 保存编码位
            }
            else {  // 编码位为0
                printf("0");
                if (!curptr->left)
                    curptr->left = createnode();  // 创建左孩子节点
                curptr = curptr->left;
                (hashlist[src]->code)[temlen] = 0;  // 保存编码位
            }
        }
        printf("\n");
        curptr->ch = src;  // 在叶子节点存储字符
    }
    
    // 构建要查找的字符串的位模式
    for (i = 0; seekword[i]; i++) {
        if (seekword[i] > 127) {  // 检查字符是否在ASCII范围内
            printf("error! can't search for %c(ASCII=0x%x) ", seekword[i], seekword[i]);
            return 0;
        }
        if (!hashlist[seekword[i]]) {  // 字符没有编码，查找失败
            printf("查找失败");
            goto cleanup;
        }
        searchlen += hashlist[seekword[i]]->len;  // 累加编码长度
    }
    
    // 分配内存存储整个查找字符串的位模式
    word = (unsigned char*)malloc(sizeof(unsigned char) * searchlen);
    i = 0;
    for (j = 0; seekword[j]; j++) {
        for (k = 0; k < hashlist[seekword[j]]->len; k++) {
            word[i++] = hashlist[seekword[j]]->code[k];  // 拼接编码位
        }
    }
    
    // 构建坏字符跳转表(badjump) - Sunday算法的坏字符规则
    for (j = searchlen - 1; j >= 0; j--) {
        if (word[j] && !find1) {  // 查找最后一个1的位置
            find1 = 1;
            badjump[1] = j;
        }
        if (!word[j] && !find0) {  // 查找最后一个0的位置
            find0 = 1;
            badjump[0] = j;
        }
        if (find1 && find0) break;  // 两个位置都找到，退出
    }
    
    // 计算好跳转表(goodjump) - Sunday算法的好后缀规则
    goodjump = (int*)malloc(sizeof(int) * searchlen);
    if (!goodjump) goto cleanup;
    memset(goodjump, 0, sizeof(int) * searchlen);
    
    // goodjump[i]表示从后往前匹配了i个字符后不匹配，应该向前跳几个位置
    for (i = 1; i < searchlen; i++) {
        chlen = 1;
        int findflag = 1;
        // 查找与后缀匹配的前缀
        for (k = searchlen - 2; k >= i - 1; k--, chlen++) {
            findflag = 1;
            for (j = k; j > k - i; j--) {
                if (word[j] != word[searchlen - 1 - k + j]) {
                    findflag = 0;  // 不匹配
                    break;
                }
            }
            if (findflag) {  // 找到匹配
                goodjump[i] = chlen;
                break;
            }
        }
        if (!findflag && i)  // 没有找到匹配
            goodjump[i] = max(chlen, goodjump[i - 1]);
    }
    
    // 开始查找：预加载第一窗口数据
    curptr = root;
    find0 = 0;
    for (j = 0; j <= (MAXR - 1) * 8; j += 8) {
        if ((ch = getc(input)) == EOF) {  // 文件结束
            find0 = 1;
            break;
        }
        split(ch, &windows[j], 8);  // 将字节拆分为位并存入窗口
    }
    
    // 第一轮查找(针对小于窗口的文件)
    chlen = 0;  // 解码位置
    for (i = 0; i <= j - searchlen; ) {
        // Sunday算法匹配
        for (k = searchlen - 1; k >= 0; k--) {
            if (word[k] != windows[k + i]) {  // 不匹配
                // 根据坏字符和好后缀规则计算跳转距离
                i += max(k - badjump[windows[k + i]], goodjump[searchlen - k - 1]);
                break;
            }
        }
        
        // 更新解码位置
        for (; chlen < min(i, j); chlen++) {
            if (windows[chlen])
                curptr = curptr->right;
            else
                curptr = curptr->left;
            if (!curptr->left && !curptr->right) {  // 到达叶子节点
                counter++;  // 解码字符计数
                curptr = root;  // 重置解码指针
            }
        }
        
        if (k == -1) {  // 完全匹配
            printf("第%d次出现在第%d个字符\n", ++findtimes, counter + 1);
            i++;  // 继续下一个位置
        }
    }
    
    if (find0) goto end;  // 文件已处理完毕
    
    // 多轮查找(处理大于窗口的文件)
    find1 = 1;  // find1=0时退出循环
    
    // 将未处理的数据移动到窗口开头
    for (k = 0; i < j; k++, i++)
        windows[k] = windows[i];
    j = k;  // 更新窗口大小
    chlen = 0;
    i = 0;
    
    while (find1) {
        // 填充窗口剩余部分
        for (; j <= (MAXR - 1) * 8; j += 8) {
            if ((ch = getc(input)) == EOF) {
                find1 = 0;  // 文件结束
                break;
            }
            split(ch, &windows[j], 8);
        }
        
        // 在当前窗口中进行查找
        for (; i <= j - searchlen; ) {
            for (k = searchlen - 1; k >= 0; k--) {
                if (word[k] != windows[k + i]) {  // 不匹配
                    i += max(k - badjump[windows[k + i]], goodjump[searchlen - k - 1]);
                    break;
                }
            }
            
            find0 = j > i;  // 标记是否有剩余数据
            
            // 更新解码位置
            for (; chlen < min(j, i); chlen++) {
                if (windows[chlen])
                    curptr = curptr->right;
                else
                    curptr = curptr->left;
                if (!curptr->left && !curptr->right) {  // 到达叶子节点
                    counter++;
                    curptr = root;
                }
            }
            
            if (k == -1) {  // 完全匹配
                printf("第%d次出现在第%d个字符\n", ++findtimes, counter + 1);
                i++;
            }
        }
        
        // 将未处理的数据移动到窗口开头，准备下一轮
        for (k = 0; i < j; i++, k++)
            windows[k] = windows[i];
        chlen = find0 ? 0 : k;  // 更新解码位置
        j = k;  // 更新窗口大小
        i = 0;
    }

end:
    printf("查找结束，共找到%d个", findtimes);

cleanup:
    // 清理资源
    if (input) fclose(input);
    if (root) destroy(root);
    free(word);
    free(goodjump);
    for (i = 0; i < 128; i++) {
        if (hashlist[i]) {
            free(hashlist[i]->code);
            free(hashlist[i]);
        }
    }
    
    return 0;
}
