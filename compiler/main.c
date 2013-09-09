#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

enum {
    LANG_C,
    LANG_CXX,
};

enum {
    CP_NML,
    CP_CE,
};

enum {
    CE_SIG,
    CE_NOSRC,
    CE_NOBIN,
    CE_NOLANG
};

char compile_command[1024];
const char *CMD_C[] = {"gcc", "-lm", "--static", "-Wall", "-fno-asm", "-O2", "-o", NULL};
const char *CMD_CXX[] = {"g++", "-lm", "--static", "-Wall", "-fno-asm", "-O2", "-o", NULL};
char source[1024], binary[1024];
int lang;
pid_t cpid;

void result(int rs, ...) {
    va_list tmp;
    int arg;
    va_start(tmp, rs);
    arg = va_arg(tmp, int);
    va_end(tmp);
    if(rs == CP_NML) {
        perror("Compiled\n");
    } else {
        if(arg == CE_SIG) {
            perror("Compile Error\n");
        } else if(arg == CE_NOSRC) {
            perror("Source Not Exist\n");
        } else if(arg == CE_NOBIN) {
            perror("Binary Not Exist\n");
        } else if(arg == CE_NOLANG) {
            perror("No Such LANG\n");
        } else {
            perror("Unexpected Error\n");
        }
    }
    printf("%d %d\n", rs, arg);
    exit(0);
}

void genCmd()
{
    int i;
    switch(lang) {
        case(LANG_C):
            for(i = 0; CMD_C[i] != NULL; i++)
                sprintf(compile_command, "%s %s", compile_command, CMD_C[i]);
            break;
        case(LANG_CXX):
            for(i = 0; CMD_CXX[i] != NULL; i++)
                sprintf(compile_command, "%s %s", compile_command, CMD_CXX[i]);
            break;
    }
    sprintf(compile_command, "%s %s %s", compile_command, binary, source);
}

void doCompile()
{
    execl("/bin/sh", "sh", "-c", compile_command, NULL);
}

void compile()
{
    cpid = fork();
    if(cpid < 0) {
        perror("Compile Fork Error");
        exit(1);
    } else if(cpid == 0) {
        doCompile();
    } else {
        int status = 0;
        waitpid(cpid, &status, 0);
        if(WIFEXITED(status) && WEXITSTATUS(status)) {
            result(CP_CE, CE_SIG);
        }
        if(access(binary, F_OK) == -1) {
            result(CP_CE, CE_NOBIN);
        }
        result(CP_NML);
    }
}

void srcExist()
{
    if(access(source, F_OK) == -1) {
        result(CP_CE, CE_NOSRC);
    }
}

void binExist()
{
    if(access(binary, F_OK) == 0) {
        remove(binary);
    }
}

int main(int argc, char *argv[])
{
    if(argc < 4) {
        printf("ERROR: Not enough arguments\n"\
                "Format: compiler [source] [binary] [lang]\n");
        exit(1);
    } else {
        strcpy(source, argv[1]);
        strcpy(binary, argv[2]);
        lang = atoi(argv[3]);
        if(lang > 1)
            result(CP_CE, CE_NOLANG);
    }
    srcExist();
    binExist();
    genCmd();
    compile();
    return 0;
}
