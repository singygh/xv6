#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

// 全局变量：存储-exec的命令及参数（NULL终止）
char **exec_argv = 0;
int has_exec = 0;

// 提取路径中的basename（确保以\0终止）
char* get_basename(char *path) {
    char *p;
    for(p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;
    if (strlen(p) >= DIRSIZ) {
        p[DIRSIZ - 1] = '\0';
    }
    return p;
}

// 执行-exec指定的命令，将file作为参数
void exec_command(char *file) {
    if (!has_exec || !exec_argv) return;

    // 计算命令参数数量（含file和NULL终止符）
    int argc = 0;
    while (exec_argv[argc]) argc++;
    char **argv = malloc((argc + 2) * sizeof(char*)); // 原命令 + file + NULL
    if (!argv) return;

    // 复制原命令参数
    for (int i = 0; i < argc; i++) {
        argv[i] = exec_argv[i];
    }
    // 添加文件路径作为最后一个参数
    argv[argc] = file;
    argv[argc + 1] = 0;

    // 创建子进程执行命令
    int pid = fork();
    if (pid == 0) {
        exec(argv[0], argv); // 执行命令（如grep hello ./a/b）
        fprintf(2, "find: exec %s failed\n", argv[0]);
        exit(1);
    } else if (pid > 0) {
        wait(0); // 等待子进程完成
    } else {
        fprintf(2, "find: fork failed\n");
    }
    free(argv);
}

void find(char *path, char* name) {
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    int n;
    
    if((fd = open(path, O_RDONLY)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if(fstat(fd, &st) < 0){
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch (st.type) {
    case T_FILE: 
        if(strcmp(get_basename(path), name) == 0) {
            // 如果有-exec，执行命令；否则打印路径
            if (has_exec) {
                exec_command(path);
            } else {
                printf("%s\n", path);
            }
        }
        break;

    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ >= sizeof(buf)) {
            fprintf(2, "find: path too long: %s\n", path);
            close(fd);
            return;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        if (p > buf && *(p - 1) != '/') {
            *p++ = '/';
        }
        
        while((n = read(fd, &de, sizeof(de))) == sizeof(de)) {
            if(de.inum == 0) continue;
            
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = '\0';
            
            char *child_name = get_basename(buf);
            if (strcmp(child_name, ".") == 0 || strcmp(child_name, "..") == 0) {
                continue;
            }
            
            find(buf, name);
        }
        if(n < 0){
            fprintf(2, "find: read error in %s\n", path);
        }
        break;

    case T_DEVICE:
        break;
    }

    close(fd);
}

int main(int argc, char* argv[]) {
    // 解析参数：支持两种格式
    // 1. find <path> <name>
    // 2. find <path> <name> -exec <cmd>...
    if (argc < 3) {
        fprintf(2, "usage: find <path> <name> [-exec <cmd>...]\n");
        exit(1);
    }

    char *path = argv[1];
    char *name = argv[2];

    // 检查是否有-exec选项
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-exec") == 0) {
            has_exec = 1;
            exec_argv = &argv[i + 1]; // 命令参数从-exec后开始
            // 确保命令参数不为空
            if (i + 1 >= argc) {
                fprintf(2, "find: -exec requires command\n");
                exit(1);
            }
            break;
        }
    }

    find(path, name);
    exit(0);
}