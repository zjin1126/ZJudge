#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

char ans_file[1024], out_file[1024];

enum {
    RS_AC,
    RS_WA,
    RS_NOFILE,
    RS_NOANS,
};

void result(int res) {
    if(res == RS_AC) {
        printf("%d\n", RS_AC);
        exit(0);
    } else if (res == RS_WA) {
        printf("%d\n", RS_WA);
        exit(0);
    } else if (res == RS_NOFILE) {
        printf("%d\n", RS_NOFILE);
        exit(0);
    } else if (res == RS_NOANS) {
        printf("%d\n", RS_NOANS);
        exit(0);
    }
}

void anscmp() {
    FILE *fpans;
    FILE *fpout;

    fpans = fopen(ans_file, "r");
    fpout = fopen(out_file, "r");

    if(fpout == NULL) {
        result(RS_NOFILE);
    }

    char temp_ans, temp_out;
    while((temp_ans = fgetc(fpans))) {
        temp_out = fgetc(fpout);
        /*printf("%c %c\n", temp_ans, temp_out);*/
        
        if(temp_out != temp_ans) {
            result(RS_WA);
        } else if(temp_out == EOF && temp_ans == EOF) {
            result(RS_AC);
        }
    }
}

void filestatus() {
    if(access(ans_file, F_OK)) {
        perror("Ans non exist!\n");
        result(RS_NOANS); 
    }
    if(access(out_file, F_OK)) {
        perror("Output non exist!\n");
        result(RS_NOFILE);
    }
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        printf("ERROR: Not enough arguments\n"\
          "Format: checker [answer] [output]\n");
        exit(1);
    }
    sprintf(ans_file, "%s", argv[1]);
    sprintf(out_file, "%s", argv[2]);
    filestatus();
    anscmp();

    return 0;
}
