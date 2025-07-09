#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

// 提取文件名（去除路径）
char*
basename(char *path)
{
    char *p;
    for(p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    return p + 1;
}

void
find(char *path, char *target)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
    case T_FILE:
        // 检查文件名是否匹配
        if(strcmp(basename(path), target) == 0){
            printf("%s\n", path);
        }
        break;

    case T_DIR:
        // 构建路径缓冲区
        if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        
        // 读取目录项
        while(read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum == 0)
                continue;
            
            // 跳过 "." 和 ".."
            if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            
            // 构建完整路径
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            
            // 递归查找
            find(buf, target);
        }
        break;
    }
    close(fd);
}

int
main(int argc, char *argv[])
{
    if(argc != 3){
        fprintf(2, "Usage: find <directory> <filename>\n");
        exit(1);
    }

    find(argv[1], argv[2]);
    exit(0);
}