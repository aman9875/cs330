#include<pipe.h>
#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
/***********************************************************************
 * Use this function to allocate pipe info && Don't Modify below function
 ***********************************************************************/
struct pipe_info* alloc_pipe_info()
{
    struct pipe_info *pipe = (struct pipe_info*)os_page_alloc(OS_DS_REG);
    char* buffer = (char*) os_page_alloc(OS_DS_REG);
    pipe ->pipe_buff = buffer;
    return pipe;
}


void free_pipe_info(struct pipe_info *p_info)
{
    if(p_info)
    {
        os_page_free(OS_DS_REG ,p_info->pipe_buff);
        os_page_free(OS_DS_REG ,p_info);
    }
}
/*************************************************************************/
/*************************************************************************/


long pipe_close(struct file *filep)
{
    /**
    * TODO:: Implementation of Close for pipe 
    * Free the pipe_info and file object
    * Incase of Error return valid Error code 
    */
    return -1;
}



int pipe_read(struct file *filep, char *buff, u32 count)
{
    /**
    *  TODO:: Implementation of Pipe Read
    *  Read the contect from buff (pipe_info -> pipe_buff) and write to the buff(argument 2);
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */
    char *buff1 = filep->pipe->pipe_buff;
    if(count < PIPE_MAX_SIZE){
        int i;
        for(i=0;i<count && buff1[i] !='\0';i++){
            buff[i] = buff1[i];
        }
        buff[i] = '\0';
        return i;
    } else {
        return -1;
    }
}


int pipe_write(struct file *filep, char *buff, u32 count)
{
    /**
    *  TODO:: Implementation of Pipe Read
    *  Write the contect from   the buff(argument 2);  and write to buff(pipe_info -> pipe_buff)
    *  Validate size of buff, the mode of pipe (pipe_info->mode),etc
    *  Incase of Error return valid Error code 
    */
    char *buff1 = filep->pipe->pipe_buff;
    if(count < PIPE_MAX_SIZE){
        int i;
        for(i=0;i<count && buff[i]!='\0';i++){
            buff1[i] = buff[i];
        }
        buff1[i] = '\0';
        return i;
    } else {
        return -1;
    }
}

int create_pipe(struct exec_context *current, int *fd)
{
    /**
    *  TODO:: Implementation of Pipe Create
    *  Create file struct by invoking the alloc_file() function, 
    *  Create pipe_info struct by invoking the alloc_pipe_info() function
    *  fill the valid file descriptor in *fd param
    *  Incase of Error return valid Error code 
    */
    int fd1 = 0;
    while(current->files[fd1]){
        fd1++;
    }
    fd[0] = fd1++;;
    while(current->files[fd1]){
        fd1++;
    }
    fd[1] = fd1;
    struct file *filep1 = alloc_file();
    struct file *filep2 = alloc_file();
    current->files[fd[0]] = filep1;
    current->files[fd[1]] = filep2;
    struct pipe_info *pipe_info_p = alloc_pipe_info();
    filep1->pipe = pipe_info_p;
    filep2->pipe = pipe_info_p;
    filep1->fops->read = pipe_read;
    filep1->fops->close = generic_close;
    filep2->fops->write = pipe_write;
    filep2->fops->close = generic_close;
    char pipe_buff[PIPE_MAX_SIZE];
    pipe_info_p->pipe_buff = pipe_buff;
    pipe_info_p->read_pos = 0;
    pipe_info_p->write_pos = 0;
    pipe_info_p->buffer_offset = 0;
    pipe_info_p->is_ropen = 1;
    pipe_info_p->is_wopen = 1;
    return 0;
}

