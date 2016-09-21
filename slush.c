#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define max_buf_size 257
#define max_args 15

volatile sig_atomic_t skip = 0;

void sigHandler(int signum) {
	skip = 1;
	if (write(STDOUT_FILENO, "\n", 1) == -1) {
		perror("interrupt failed to write new line");
	}
}

int interpret(char* buf, int isFirst) {
	char* str = strtok(buf, "(");
	int readEnd;
	if (str) {
		readEnd = interpret(NULL, 0);
	} else {
		// bottom of recursion condiiton
		return -1;
	}

	int fd[2];
	if (!isFirst) {
		if(pipe(fd) == -1) {
			perror("pipe not created");
			exit(-1);
		}
	}

	int pid = fork();
	if (pid == -1) {
		perror("fork problem");
		exit(-1);
	} else if (pid != 0) {
		waitpid(pid, NULL, 0);
	} else {

		// replace STDIN with passed file descriptor if not bottom of recursion
		if (readEnd != -1) {
			if (dup2(readEnd, STDIN_FILENO) == -1) {
				perror("stdin not replaced");
			}
		}
		
		// replace STDOUT with new pipe if not top of recursion
		if (!isFirst) {
			if (dup2(fd[1], STDOUT_FILENO) == -1) {
				perror("stdout not replaced");
			}
			if (close(fd[0]) == -1) {
				perror("fd[0] not closed");
			}
		}

		
		int max_argv_size = max_args + 2; //one for argv[0], one for null terminator
		char* cmd;
		char* my_argv[max_argv_size];
		
		cmd = strtok(str, " ");
		char* curr = cmd;

		if (curr) {
			my_argv[0] = cmd;
		} else {
			my_argv[0] = '\0';
		}

		int i=1;
		while(curr) {
			my_argv[i] = strtok(NULL, " ");
			curr = my_argv[i];
			i++;
		}
		my_argv[i] = '\0';

		if (!cmd) {
			printf("Invalid null command");
			exit(-1);
		}

		if (execvp(cmd, my_argv) == -1) {
			char error[20];
			sprintf(error, "%s", cmd);
			perror(error);
			exit(-1);
		}
	}

	if (!isFirst) {
		if (close(fd[1]) == -1) {
			perror("parent failed to close fd[1]");
		}
	}

	if (readEnd != -1) {
		if (close(readEnd) == -1) {
			perror("parent failed to close readEnd");
		}
	}

	return fd[0];
}

void printPrompt()
{
        //char* cwd = getenv("PWD");
        char* cwd = get_current_dir_name();
	if (!cwd) {
		printf("could not get current dir name\n");
	}
        char* home = getenv("HOME");
	if (!home) {
		printf("could not get HOME envvar\n");
	}
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
        // {
        struct sigaction sa;
        sa.sa_handler = sigHandler;
        sa.sa_flags = 0;
        if (sigemptyset(&sa.sa_mask) == -1) {
		perror("sigemptyset failed\n");
	}
        if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction failed:\n");
	}
        // } source: http://beej.us/guide/bgipc/output/html/multipage/signals.html

        for(;;) {
                skip = 0;

                char* buf = (char*) malloc(sizeof(char)*max_buf_size);
		if (!buf) {
			printf("Malloc failed\n");
			exit(-1);
		}

                printPrompt();

                char* ret = fgets(buf, max_buf_size, stdin);
            
                if (skip) {
                    free(buf);
                    continue;
                }

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
				if (buf[strlen(buf) - 1] == '(') {
					printf("Invalid null command\n");
					interpretBuf = 0;
				} else if ((buf[0] == 'c') && (buf[1] == 'd'))
                                {
                                        char* path = strtok(buf, " ");
                                        path = strtok(NULL, " ");
                                        if (!path)
						path = getenv("HOME");
                                                if(path == NULL) {
							printf("could not get HOME");
							exit(-1);
						}
                                        if(chdir(path) == -1) {
						perror("could not chdir");
						exit(-1);
					}
                                        interpretBuf = 0;
                                }
                        }
		}
              
                if (interpretBuf) {
                        interpret(buf, 1);
		}


                free(buf);
        }
        return 0;
}
