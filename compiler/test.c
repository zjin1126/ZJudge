#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

int main() {
    struct rlimit limit;
    pid_t child;
    child = fork();
    if(child == 0) {
        limit.rlim_cur = limit.rlim_max = 0;
        setrlimit(RLIMIT_FSIZE, &limit);
        execl("/root/test", "test", NULL); 
    } else {
        int status = 0;
        waitpid(child, &status, 0);
        if(WIFEXITED(status))
            printf("exited normally\n");
        else
            printf("%d\n", WTERMSIG(status));
            printf("exited expected\n");
        printf("end\n");
    }
    return 0;
}
