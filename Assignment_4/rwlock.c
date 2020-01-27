#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<common.h>

/* XXX NOTE XXX  
       Do not declare any static/global variables. Answers deviating from this 
       requirement will not be graded.
*/

void writeLock(rwlock_t *lock){
	long expected = 1LL << 48;
	long new = 0;
	while(__sync_bool_compare_and_swap(&lock->value,expected,new) == 0);
}

void writeUnlock(rwlock_t *lock){
	lock->value = (1LL << 48);
}

void readLock(rwlock_t *lock){
	while(atomic_add(&lock->value, -1) == -1);
}

void readUnlock(rwlock_t *lock){
	atomic_add(&lock->value, 1);
}

void init_rwlock(rwlock_t *lock)
{
   /*Your code for lock initialization*/
	lock->value = (1LL << 48);
}

void write_lock(rwlock_t *lock)
{
   /*Your code to acquire write lock*/
	writeLock(lock);	
}

void write_unlock(rwlock_t *lock)
{
   /*Your code to release the write lock*/
	writeUnlock(lock);
}


void read_lock(rwlock_t *lock)
{
   /*Your code to acquire read lock*/
	readLock(lock);
}

void read_unlock(rwlock_t *lock)
{
   /*Your code to release the read lock*/
	readUnlock(lock);
}
