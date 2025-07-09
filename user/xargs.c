#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/param.h"

int
main(int argc, char *argv[])
{
    char buf[512];
    char *args[MAXARG];
    int i;
    char c;
    int pos = 0;

    // 检查参数
    if(argc < 2){
        fprintf(2, "Usage: xargs command [args...]\n");
        exit(1);
    }

    // 复制命令和初始参数
    for(i = 1; i < argc; i++){
        args[i-1] = argv[i];
    }

    // 读取标准输入的每一行
    while(1){
        pos = 0;
        // 读取一行
        while(read(0, &c, 1) == 1){
            if(c == '\n'){
                break;
            }
            buf[pos++] = c;
        }
        
        // 如果没有读取到任何字符，退出
        if(pos == 0){
            break;
        }
        
        buf[pos] = 0; // 字符串结束符
        
        // 将读取到的行作为参数添加
        args[argc-1] = buf;
        args[argc] = 0; // 参数数组结束符
        
        // 创建子进程执行命令
        if(fork() == 0){
            exec(args[0], args);
            fprintf(2, "xargs: exec %s failed\n", args[0]);
            exit(1);
        } else {
            wait(0); // 等待子进程完成
        }
    }
    
    exit(0);
}