#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int 
main(int argc, char *argv[]) {
    int p1[2], p2[2];  // p1: 父->子, p2: 子->父
    char buf[64];  // 用于存储消息

    pipe(p1);
    pipe(p2);

    if (fork() == 0) {
        // child process
        close(p1[1]);  // 关闭不需要的写端
        close(p2[0]);  // 关闭不需要的读端
        read(p1[0], buf, sizeof(buf));        // 读取ping
        printf("%d: received ping\n", getpid()); // 打印消息
        write(p2[1], "pong", 4);             // 发送pong
        close(p1[0]);  // 关闭不需要的读端
        close(p2[1]);  // 关闭不需要的写端
        exit(0);
    } else {
        // parent process
        close(p1[0]);  // 关闭不需要的读端
        close(p2[1]);  // 关闭不需要的写端
        write(p1[1], "ping", 4);             // 发送ping
        read(p2[0], buf, sizeof(buf));       // 读取pong
        printf("%d: received pong\n", getpid()); // 打印消息
        close(p1[1]);  // 关闭不需要的写端
        close(p2[0]);  // 关闭不需要的读端
        wait(0);                             // 等待子进程
        exit(0);
    }
}