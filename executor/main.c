#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/user.h>
#include <sys/reg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include "denycall.h"

enum {
    RS_NML, /* 0 */
    RS_TLE, /* 1 */
    RS_MLE, /* 2 */
    RS_OLE, /* 3 */
    RS_RE, /* 4 */
    RS_RF, /* 5 */
};

int additional_mem = 2; //more 2 MB memory for cpid
char input[1024], output[1024];
struct timeval tvstart, tvnow;
int time_limit, mem_limit, file_limit;
int use_time;
int use_mem;
int status;
pid_t cpid;

int result(int rs) {
    if(rs == RS_NML) {
        printf("%d %d %d\n", RS_NML, use_time, use_mem); 
        exit(0);
    } else if(rs == RS_TLE) {
        printf("%d %d %d\n", RS_TLE, use_time, use_mem);
        exit(0);
    } else if(rs == RS_MLE) {
        printf("%d %d %d\n", RS_MLE, use_time, use_mem);
        exit(0);
    } else if(rs == RS_OLE) {
        printf("%d %d %d\n", RS_OLE, use_time, use_mem);
        exit(0);
    } else if(rs == RS_RE) {
        printf("%d %d %d\n", RS_RE, use_time, use_mem);
        exit(0);
    } else if(rs == RS_RF) {
        printf("%d %d %d\n", RS_RF, use_time, use_mem);
        exit(0);
    }
    return 0;
}

int musage(int pid) {
    int mem;
    char proc_mem[32];
    sprintf(proc_mem, "/proc/%d/statm", cpid);
    FILE *fptr;
    fptr = fopen(proc_mem, "r");
    fscanf(fptr, "%d", &mem);
    mem *= 4;
    fclose(fptr);
    return mem;
}

void limit() {
    struct rlimit limit;
    limit.rlim_cur = limit.rlim_max = time_limit;
    limit.rlim_max += 2; //set time hard limit(SIGKILL), add more two secends for cpid response
    setrlimit(RLIMIT_CPU, &limit);
    limit.rlim_cur = limit.rlim_max = mem_limit;
    setrlimit(RLIMIT_AS, &limit);
    limit.rlim_cur = limit.rlim_max = file_limit;
    setrlimit(RLIMIT_FSIZE, &limit);
}

void getfile(int addr, char *filepath) {
    char *fptr = filepath;
    union peeker {
        long val;
        char chars[sizeof(long)];
    } data;
    int i, j, k = 1;
    for(i = 0; k; i++) {
        data.val = ptrace(PTRACE_PEEKDATA, cpid, addr+i*8, NULL);
        memcpy(fptr, data.chars, sizeof(long));
        for(j = 0; j < sizeof(long); j++) {
            if(fptr[j] == 0) {
                k = 0;
                break;
            }
        }
        fptr += sizeof(long);
    }
}

int binExist(char *filename) {
    if(access(filename, F_OK)) {
        printf("Binary non exist!\n");
        exit(1);
    } else {
        return 0;
    }
}

void run(char *path) {
    cpid = fork();
    long orig_rax;
    struct rusage used;
    if(cpid == 0) {
        //run programe
        limit();
        freopen(input, "r", stdin);
        freopen(output, "w+", stdout);
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(path, path, NULL);
    }
    while(1) {
        wait4(cpid, &status, 0, &used);
        if(WIFEXITED(status)) {
            if(!WEXITSTATUS(status)) {
                result(RS_NML);
            } else {
                result(RS_RE);
            }
            break;
        } else if(WIFSIGNALED(status)) {
            printf("%d\n", WTERMSIG(status));
            result(RS_RE);
        }
        orig_rax = ptrace(PTRACE_PEEKUSER, cpid, 8 * ORIG_RAX, NULL);
        //danger syscall
        if(denycall[orig_rax]) {
            ptrace(PTRACE_KILL, cpid, NULL, NULL);
            kill(cpid, SIGKILL);
            result(RS_RF);
            return;
        }

        if(orig_rax == SYS_open) {
            char *filepath = (char *)malloc(1024);
            struct user_regs_struct regs;
            ptrace(PTRACE_GETREGS, cpid, NULL, &regs);
            getfile(regs.rdi, filepath);
            /*printf("open file: %s\n", filepath);*/
            if(strcmp(filepath, input) || strcmp(filepath, output)) {
                ptrace(PTRACE_KILL, cpid, NULL, NULL);
                kill(cpid, SIGKILL);
                free(filepath);
                result(RS_RF);
            }
            free(filepath);
        }

        //time usage
        use_time = (used.ru_utime.tv_sec + used.ru_stime.tv_sec) * 1000 + (used.ru_utime.tv_usec + used.ru_stime.tv_usec) / 1000;
        if(use_time > time_limit) {
            ptrace(PTRACE_KILL, cpid, NULL, NULL);
            kill(cpid, SIGKILL);
            result(RS_TLE);
        }

        //memory usage
        use_mem = musage(cpid);
        if(use_mem > mem_limit) {
            ptrace(PTRACE_KILL, cpid, NULL, NULL);
            kill(cpid, SIGKILL);
            result(RS_MLE);
        }
        ptrace(PTRACE_SYSCALL, cpid, NULL, NULL);
    }
}

int main(int argc, char *argv[]) {
    if(argc < 7) {
        printf("ERROR: Not enough arguments\n"\
          "Format: executor [binary] [input] [output] [time] [memory] [output_size]\n");
        return EXIT_FAILURE;
    } else {
        binExist(argv[1]); 
        strcpy(input, argv[2]);
        strcpy(output, argv[3]);
        time_limit = atoi(argv[4]);
        mem_limit = (atoi(argv[5]) + additional_mem) * 1024 * 1024; //MB
        file_limit = atoi(argv[6]) * 1024 * 1024; //MB
        run(argv[1]);
    }
    return 0;
}
