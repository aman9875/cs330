// Name - Aman Tiwari , Roll No - 160094

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

// for each sub-directory(which is not a file) we create a pipe and a new child process
// In the child process calculate the size of sub directory recursively and output 
// the name of sub-directory and its size to pipe
// In the parent process read from the pipe and store result in a char array, also parse
// the result to get the size of sub-directory and add it to size of root directory
// Finally print root directory name and its size and then print the name and size of all
// sub-directories.


// utility function to get integer from string
int parseString(char *str){
	int j;
	for(j=0;str[j]!=' ';j++);
	char temp[10];
	int k = 0;
	for(;str[j]!='\0';j++){
		temp[k] = str[j];
		k++;
	}
	temp[k] = '\0';
	int num = atoi(temp);
	return num;
}

// get size of file using stat
size_t getFilesize(const char* filename) {
    struct stat st;
    if(stat(filename, &st) != 0) {
        return 0;
    }
    return st.st_size;   
}

// Function to check whether file/directory denoted by path is a directory
int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

// to calculate size of directory recursively
int size_of_directory(char *basePath){
	DIR *dir =  opendir(basePath);
	struct dirent *dent;
	char path[1000];
	int sz = 0;
	if(dir!=NULL){
		while((dent = readdir(dir))!= NULL){
			if (strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0){
				strcpy(path,basePath);
				strcat(path,"/");
				strcat(path,dent->d_name);
				if(isDirectory(path))
				sz = sz + size_of_directory(path);
				else{
					sz = sz + getFilesize(path);
				}
			}
		}
	}

	return sz;
}

int main(int argc, char *argv[]){

	if(argc == 2){
		int sum = 0;
		int fd[2];
		char dirName[100];
		int k = 0;
		int j1 = strlen(argv[1])-1;
		while(argv[1][j1] == '/')j1--;
		int j2=j1;
		while(argv[1][j2]!='/' && j2>=0){
			j2--;
		}
		for(int i=j2+1;i<=j1;i++){
			dirName[k] = argv[1][i];
			k++;
		}
		dirName[k] = '\0';
		DIR *dir =  opendir(argv[1]);
		struct dirent *dent;
		char path[1000];
		char output[100000];
		int index = 0;
		if(dir!=NULL){
			while((dent = readdir(dir))!= NULL){
				if (strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0){
					strcpy(path,argv[1]);
					strcat(path,"/");
					strcat(path,dent->d_name);
					pipe(fd);
					if(isDirectory(path)){
						if(fork() == 0){
							close(fd[0]);
							char *str1 = dent->d_name;
							char sz[100];
							for(int j=0;j<strlen(str1);j++){
								sz[j] = str1[j];
							}
							sz[strlen(str1)] = ' ';
							int directorySize = size_of_directory(path);
							char str2[10];
							sprintf(str2,"%d",directorySize);
							int j;
							for(j=0;str2[j]!='\0';j++){
								sz[j+strlen(str1)+1] = str2[j];
							}
							sz[j+strlen(str1)+1] = '\0';
							write(fd[1],sz,strlen(sz));
							exit(0);
						} else {
							close(fd[1]);
							char sz1[100];
							int sz = read(fd[0],sz1,100);
							sz1[sz] = '\0';
							for(int j=0;j<sz;j++){
								output[index] = sz1[j];
								index++;
							}
							output[index] = '\n';
							index++;
							int num = parseString(sz1);
							sum = sum + num;
						}
					} else {
						sum = sum + getFilesize(path);
					}
				}
			}
			output[index] = '\0';
			printf("%s %d\n", dirName,sum);
			printf("%s",output);
		}
	}

	return 0;
}