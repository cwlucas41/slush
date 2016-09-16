#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#define max_buf_size 256
#define max_args 15

volatile sig_atomic_t skip = 0;

void sigHandler(int signum) {
	skip = 1;
	write(STDOUT_FILENO, "\n", 1);
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
		//	printf("%s ", my_argv[j]);
		//}
		//printf("]\n");

		if (execvp(cmd, my_argv) == -1) {
			char error[20];
			sprintf(error, "%s", cmd);
			perror(error);
			exit(-1);
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
		

int main(int argc, char** argv) {
	//signal(2, sigHandler);

	// {
	struct sigaction sa;
	sa.sa_handler = sigHandler;
	sa.sa_flags = 0;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, NULL);
	// } source: http://beej.us/guide/bgipc/output/html/multipage/signals.html

	for(;;) {
		skip = 0;

		char* buf = (char*) malloc(sizeof(char)*max_buf_size);

		printf("slush > ");
		char* ret = fgets(buf, max_buf_size, stdin);

		if (skip) {
			continue;
		}

		if (!skip) {
			if (!ret) {
				printf("\n");
				exit(-1);
			} 

			buf = strtok (buf,"\n");
			interpret(buf, 1);
		} else {
			printf("skipped ");
		}

		free(buf);
	}
	return 0;
}
