#include<types.h>
#include<context.h>
#include<file.h>
#include<lib.h>
#include<serial.h>
#include<entry.h>
#include<memory.h>
#include<fs.h>
#include<kbd.h>
#include<pipe.h>


/************************************************************************************/
/***************************Do Not Modify below Functions****************************/
/************************************************************************************/
void free_file_object(struct file *filep)
{
    if(filep)
    {
       os_page_free(OS_DS_REG ,filep);
       stats->file_objects--;
    }
}

struct file *alloc_file()
{
  
  struct file *file = (struct file *) os_page_alloc(OS_DS_REG); 
  file->fops = (struct fileops *) (file + sizeof(struct file)); 
  bzero((char *)file->fops, sizeof(struct fileops));
  stats->file_objects++;
  return file; 
}

static int do_read_kbd(struct file* filep, char * buff, u32 count)
{
  kbd_read(buff);
  return 1;
}

static int do_write_console(struct file* filep, char * buff, u32 count)
{
  struct exec_context *current = get_current_ctx();
  return do_write(current, (u64)buff, (u64)count);
}

struct file *create_standard_IO(int type)
{
  struct file *filep = alloc_file();
  filep->type = type;
  if(type == STDIN)
     filep->mode = O_READ;
  else
      filep->mode = O_WRITE;
  if(type == STDIN){
        filep->fops->read = do_read_kbd;
  }else{
        filep->fops->write = do_write_console;
  }
  filep->fops->close = generic_close;
  filep->ref_count = 1;
  return filep;
}

int open_standard_IO(struct exec_context *ctx, int type)
{
   int fd = type;
   struct file *filep = ctx->files[type];
   if(!filep){
        filep = create_standard_IO(type);
   }else{
         filep->ref_count++;
         fd = 3;
         while(ctx->files[fd])
             fd++; 
   }
   ctx->files[fd] = filep;
   return fd;
}
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/
/**********************************************************************************/



void do_file_fork(struct exec_context *child)
{
  /*TODO the child fds are a copy of the parent. Adjust the refcount*/
  int i = 0;
  while(i < MAX_OPEN_FILES){
    struct file *filep = child->files[i];
    if(filep){
      filep->ref_count++;
    }
  }
}

void do_file_exit(struct exec_context *ctx)
{
   /*TODO the process is exiting. Adjust the ref_count
     of files*/
    int i = 0;
    while(i < MAX_OPEN_FILES){
      struct file *filep = ctx->files[i];
      if(filep){
        generic_close(filep);
        ctx->files[i] = NULL;
      }
    }
}

long generic_close(struct file *filep)
{
  /** TODO Implementation of close (pipe, file) based on the type 
   * Adjust the ref_count, free file object
   * Incase of Error return valid Error code 
   */
    filep->ref_count--;
    if(filep->ref_count == 0){
      free_file_object(filep);
    }
    return 0;
}

static int do_read_regular(struct file *filep, char * buff, u32 count)
{
   /** TODO Implementation of File Read, 
    *  You should be reading the content from File using file system read function call and fill the buf
    *  Validate the permission, file existence, Max length etc
    *  Incase of Error return valid Error code 
    * */
    if(filep){
      struct inode *inode1 = filep->inode;
      if((inode1->mode)&(1<<0)){
        int num = flat_read(inode1, buff, count, &(filep->offp));
        filep->offp += num; 
        return num;
      } else {
        return -EACCES;
      }
    } else {
      return -1;
    }
}


static int do_write_regular(struct file *filep, char * buff, u32 count)
{
    /** TODO Implementation of File write, 
    *   You should be writing the content from buff to File by using File system write function
    *   Validate the permission, file existence, Max length etc
    *   Incase of Error return valid Error code 
    * */
    if(filep){
      struct inode *inode1 = filep->inode;
      if((inode1->mode)&(1<<1)){
        int num = flat_write(inode1, buff, count, &(filep->offp));
        filep->offp += num;
        return num;
      } else {
        return -EACCES;
      } 
    } else {
      return -1;
    }
}

static long do_lseek_regular(struct file *filep, long offset, int whence)
{
    /** TODO Implementation of lseek 
    *   Set, Adjust the ofset based on the whence
    *   Incase of Error return valid Error code 
    * */
    if(filep){
      long int size = filep->inode->file_size;
      long int offp;
      if(whence == SEEK_SET){
        offp = offset;
      } else if(whence == SEEK_CUR){
        offp = filep->offp + offset;
      } else if(whence == SEEK_END){
        offp = size + offset;
      } else {
        return -1;
      }
      if(offp > size){
        return -1;
      } else {
        filep->offp = offp;
        return offp;
      }
    } else {
      return -1;
    }
}

extern int do_regular_file_open(struct exec_context *ctx, char* filename, u64 flags, u64 mode)
{ 
  /**  TODO Implementation of file open, 
    *  You should be creating file(use the alloc_file function to creat file), 
    *  To create or Get inode use File system function calls, 
    *  Handle mode and flags 
    *  Validate file existence, Max File count is 32, Max Size is 4KB, etc
    *  Incase of Error return valid Error code 
    * */
    struct inode *inode1 = lookup_inode(filename);
    if(inode1 == NULL){
      if((1<<3)&flags)
        inode1 = create_inode(filename, mode);
      else return -1;
    }
    struct file *filep = alloc_file();
    filep->type = REGULAR;
    filep->inode = inode1;
    filep->mode = mode;
    filep->offp = 0;
    filep->pipe = NULL;
    filep->fops->read = do_read_regular;
    filep->fops->write = do_write_regular;
    filep->fops->close = generic_close;
    filep->fops->lseek = do_lseek_regular;
    filep->ref_count++;
    int fd = 3;
    while(ctx->files[fd]){
      fd++;
    }
    ctx->files[fd] = filep;
    int ret_fd = fd; 
    return ret_fd;
}

int fd_dup(struct exec_context *current, int oldfd)
{
   /** TODO Implementation of dup 
    *  Read the man page of dup and implement accordingly 
    *  return the file descriptor,
    *  Incase of Error return valid Error code 
    * */
    if(oldfd < 0){
      return -1;
    } else {
      struct file *filep = current->files[oldfd];
      int newfd = 0;
      while(current->files[newfd]){
        newfd++;
      }
      current->files[newfd] = filep;
      return newfd;
    }
}


int fd_dup2(struct exec_context *current, int oldfd, int newfd)
{
  /** TODO Implementation of the dup2 
    *  Read the man page of dup2 and implement accordingly 
    *  return the file descriptor,
    *  Incase of Error return valid Error code 
    * */
    if(oldfd < 0 || newfd < 0){
      return -1;
    } else if(oldfd == newfd){
      return newfd;
    } else {
      struct file *filep = current->files[newfd];
      generic_close(filep);
      current->files[newfd] = NULL;
      struct file *filep1 = current->files[oldfd];
      current->files[newfd] = filep1;
      return newfd;
    }
}
