#include "common.h"
#include "encoder.h"
#include "decoder.h"
#include "finder.h"

void print_usage(const char *prog_name) {
    printf("哈夫曼压缩工具 v1.0\n");
    printf("用法:\n");
    printf("  %s -e <输入文件.txt> <输出文件.bin>   压缩文件\n", prog_name);
    printf("  %s -d <输入文件.bin> <输出文件.txt>   解压文件\n", prog_name);
    printf("  %s -f <输入文件.bin> <单词>   查找单词\n", prog_name);
    printf("\n示例: %s -e document.txt output.bin\n", prog_name);
    printf("\n示例: %s -f output.bin hello\n", prog_name);
}

int main(int argc, char *argv[]) {
    // 无参数显示帮助
    if(argc == 1) {
        print_usage(argv[0]);
        return 0;
    }
    // 检查特定模式
    if(argc == 4 && (strcmp(argv[1], "-e") == 0 ) ){
        // 编码模式
        return encoder(argv[2], argv[3]);
    }
    else if(argc == 4 && (strcmp(argv[1], "-d") == 0)) {
        // 解码模式
        return decoder(argv[2], argv[3]);
    }
    else if(argc == 4 && (strcmp(argv[1], "-f") == 0)) {
        // 查找模式
        return finder(argv[2], argv[3]);
    }
    else {
        printf("错误: 参数不正确\n\n");
        print_usage(argv[0]);
        return 1;
    }
    
    return 0;
}
