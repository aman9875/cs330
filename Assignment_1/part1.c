// Name - Aman Tiwari , Roll No - 160094

#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

// Function to check whether the file/directory denoted by path is regular file
int is_regular_file(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

// Function to check whether file/directory denoted by path is a directory
int isDirectory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

void listFiles(char *path, char *string);

int main(int argc,char *argv[]){
	if(argc == 3){
		// if the path denotes a regular file then we can directly search for string in it
		if(is_regular_file(argv[2])){
			int fd = open(argv[2],O_RDONLY);
			if(fd < 0){
				perror("Cannot open file");
				exit(1);
			}
			char temp,str[10000];
			int j = 0;
			/* read each character and store it in str, until we get a newline character then we can search for 
			string in str and print str if we find it */
			while(read(fd,&temp,sizeof(char))){
				if(temp != '\n'){
					str[j] = temp;
					j++;
				} else {
					str[j] = '\0';
					if(strstr(str,argv[1])!=NULL){
						printf("%s\n",str);
					}
					j = 0;
				}
			}
		} else {
			// if the path denotes a directory then call recursive function listFiles
			listFiles(argv[2],argv[1]);
		}
	}

	return 0;
}

void listFiles(char *basePath, char *string){
	int len = strlen(basePath);
	if(basePath[len-1] == '/'){
		basePath[len-1] = '\0';
	}
	DIR *dir =  opendir(basePath);
	struct dirent *dent;
	char path[1000];
	if(dir!=NULL){
		// iterate through all the subdirectories
		while((dent = readdir(dir))!= NULL){
			 if (strcmp(dent->d_name, ".") != 0 && strcmp(dent->d_name, "..") != 0){
				strcpy(path,basePath);
				strcat(path,"/");
				strcat(path,dent->d_name);
				// if subdirectory is a directory then make recursive call
				if(isDirectory(path))
					listFiles(path,string);
				// otherwise search for string in the file
				else if(is_regular_file(path)){
					int fd = open(path,O_RDONLY);
					if(fd  < 0) {
						perror("Cannot open file");
						exit(1);
					}
					char temp,str[10000];
					int j = 0;
					while(read(fd,&temp,sizeof(char))){
						if(temp != '\n'){
							str[j] = temp;
							j++;
						} else {
							str[j] = '\0';
							if(strstr(str,string)!=NULL){
								printf("%s:%s\n",path,str);
							}
							j = 0;
						}
					}
				}
			}
		}
	}
}
