// Name - Aman Tiwari , Roll No - 160094

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include<sys/wait.h> 
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc,char *argv[]){

	if(argc == 4 && argv[1][0] == '@'){ // when operator is @
		int fd[2];
		if(pipe(fd) == -1){ // create pipe
			perror("pipe");
			exit(1);
		}
		int pid = fork();
		if(pid == -1){
			perror("fork");
			exit(2);
		}
		if(pid == 0){
			// child process
			// close reading end of pipe for child and redirect STDOUT to writing end
			close(fd[0]);
			dup2(fd[1],1);
			char *arg[5];
			arg[0] = "grep";
			arg[1] = "-rF";
			arg[2] = argv[2];
			arg[3] = argv[3];
			arg[4] = NULL;
			execvp("grep",arg);
			perror("execvp child");
		} else {
			// parent process
			// close writing end of pipe for parent and redirect STDIN to reading end
			close(fd[1]);
			dup2(fd[0],0);
			char *arg[3];
			arg[0] = "wc";
			arg[1] = "-l";
			arg[2] = NULL;
			execvp("wc",arg);
			perror("execvp parent");
		}
	} else if(argv[1][0] == '$') { // when operator is $
		int fd[2];
		int fd2[2];
		if(pipe(fd) == -1){ // create first pipe
			perror("pipe");
			exit(1);
		}
		int pid1 = fork();
		if(pid1 == -1){
			perror("fork");
			exit(2);
		}
		if(pid1 == 0){
			if(pipe(fd2) == -1){ // create second pipe
				perror("pipe");
				exit(11);
			}
			int pid2 = fork();
			if(pid2 == -1){
				perror("fork");
				exit(22);
			}
			if(pid2 == 0){
				// 2nd child
				// this process executes grep command
				close(fd[1]); // close the first pipe for this child
				close(fd[0]);
				close(fd2[0]); // close the reading end of second pipe
				dup2(fd2[1],1); // redirect STDOUT to writing end of second pipe
				char *arg[5];
				arg[0] = "grep";
				arg[1] = "-rF";
				arg[2] = argv[2];
				arg[3] = argv[3];
				arg[4] = NULL;
				execvp("grep",arg);
				perror("execvp second child");
			} else {
				// 1st child
				// this process executes tee command
				close(fd[0]); // close reading end of first pipe
				close(fd2[1]); // close writing end of second pipe
				dup2(fd[1],1); // redirect STDOUT to writing end of first pipe 
				char buffer[100000];
				int fd1 = open(argv[4],O_WRONLY| O_CREAT | O_TRUNC, 0644); // open output.txt
				if(fd1 < 0){
					perror("Cannot open file");
					exit(1);
				}
				int j = 0;
				char temp;
				while(read(fd2[0],&temp,sizeof(char))){ // read from second pipe which has the output of grep
					buffer[j] = temp;
					j++;
				} 
				buffer[j] = '\0';
				write(fd1,buffer,strlen(buffer)); // write to output.txt file
				printf("%s",buffer); // since we have closed STDOUT the text goes to first pipe which we can read
			}
		} else {
			// parent
			close(fd[1]); 
			dup2(fd[0],0); // redirect STDIN to reading end of first pipe
			int cnt = argc - 5;
			char *arg[cnt+1];
			for(int i=0;i<cnt;i++){
				arg[i] = argv[5+i];
			}
			arg[cnt] = NULL;
			execvp(argv[5],arg);
			perror("execvp parent");
		}
	}
	return 0;
}