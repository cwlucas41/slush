#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define max_buf_size 256
#define max_args 15

int skip = 0;

void sigHandler(int signum) {
        skip = 1;
        printf("\n");
}

int interpret(char* buf, int isFirst) {
        char* str = strtok(buf, "(");
        int readEnd;
        if (str) {
                readEnd = interpret(NULL, 0);
        } else {
                return -1;
        }

        int fd[2];
        if (!isFirst) {
                pipe(fd);
        }

        int pid = fork();
        if (pid) {
                waitpid(pid, NULL, 0);
        } else {

                // replace STDIN with passed file descriptor if not bottom of recursion
                if (readEnd != -1) {
                        dup2(readEnd, STDIN_FILENO);
                }

                // replace STDOUT with new pipe if not top of recursion
                if (!isFirst) {
                        dup2(fd[1], STDOUT_FILENO);
                        close(fd[0]);
                }


                int max_argv_size = max_args + 2; //one for argv[0], one for null terminator
                char* cmd;
                char* my_argv[max_argv_size];

                cmd = strtok(str, " ");
                char* curr = cmd;
                my_argv[0] = cmd;

                int i=1;
                while(curr) {
                        my_argv[i] = strtok(NULL, " ");
                        curr = my_argv[i];
                        i++;
                }
                my_argv[i] = '\0';

                //int j=0;
                //printf("[ ");
                //for(j; j<i; j++) {
                //      printf("%s ", my_argv[j]);
                //}
                //printf("]\n");

                int execRet = execvp(cmd, my_argv);
                if (execRet == -1) {
                        char error[20];
                        sprintf(error, "%s", cmd);
                        perror(error);
                }
        }

        if (!isFirst) {
                close(fd[1]);
        }

        if (readEnd != -1) {
                close(readEnd);
        }

        return fd[0];
}

void printPrompt()
{
        //char* cwd = getenv("PWD");
        char* cwd = get_current_dir_name();
        char* home = getenv("HOME");
        int cwdLen = (int)strlen(cwd);
        int truncLen = cwdLen - (int)strlen(home) - 1;
        if (truncLen < 0)
        {
                printf("slush| > ");
        }
        else
        {
                char truncatedPath[truncLen];
                int i = 1;
                for (i; i < truncLen + 1; i++)
                {
                        truncatedPath[truncLen - i] = cwd[cwdLen - i];
                }
                printf("slush|%s > ", truncatedPath);
        }

}

int main(int argc, char** argv) {
        //signal(2, sigHandler);

        // {
        struct sigaction sa;
        sa.sa_handler = sigHandler;
        sa.sa_flags = 0;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGINT, &sa, NULL);
        // } source: http://docs.aws.amazon.com/iot/latest/developerguide/iot-gs.html

        for(;;) {
                skip = 0;

                char* buf = (char*) malloc(sizeof(char)*max_buf_size);

                printPrompt();

                char* ret = fgets(buf, max_buf_size, stdin);

                if (!skip) {
                        if (!ret) {
                                printf("\n");
                                exit(-1);
                        } 

                        buf = strtok (buf,"\n");
                        int interpretBuf = 1;
                        if (buf)
                        {
                                if ((int)strlen(buf) > 1)
                                {
                                        if ((buf[0] == 'c') && (buf[1] == 'd'))
                                        {
                                                char* path = strtok(buf, " ");
                                                path = strtok(NULL, " ");
                                                if (!path)
                                                        path = getenv("HOME");
                                                chdir(path);
                                                interpretBuf = 0;
                                        }
                                }
                        }
                        if (interpretBuf)
                                interpret(buf, 1);
                }

                free(buf);
        }
        return 0;
}
