#ifndef _POSIX_PTHREAD_SEMANTICS
#define _POSIX_PTHREAD_SEMANTICS
#endif
#include "sthread.h"
#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void smutex_init(smutex_t *mutex)
{
  if(pthread_mutex_init(mutex, NULL)){
      perror("pthread_mutex_init failed");
      exit(-1);
  }    
}

void smutex_destroy(smutex_t *mutex)
{
  if(pthread_mutex_destroy(mutex)){
      perror("pthread_mutex_destroy failed");
      exit(-1);
  }    
}

void smutex_lock(smutex_t *mutex)
{
  if(pthread_mutex_lock(mutex)){
    perror("pthread_mutex_lock failed");
    exit(-1);
  }    
}

void smutex_unlock(smutex_t *mutex)
{
  if(pthread_mutex_unlock(mutex)){
    perror("pthread_mutex_unlock failed");
    exit(-1);
  }    
}



void scond_init(scond_t *cond)
{
  if(pthread_cond_init(cond, NULL)){
      perror("pthread_cond_init failed");
      exit(-1);
  }
}

void scond_destroy(scond_t *cond)
{
  if(pthread_cond_destroy(cond)){
      perror("pthread_cond_destroy failed");
      exit(-1);
  }
}

void scond_signal(scond_t *cond, smutex_t *mutex __attribute__((unused)))
{
  //
  // assert(mutex is held by this thread);
  //

  if(pthread_cond_signal(cond)){
    perror("pthread_cond_signal failed");
    exit(-1);
  }
}

void scond_broadcast(scond_t *cond, smutex_t *mutex __attribute__((unused)))
{
  //
  // assert(mutex is held by this thread);
  //
  if(pthread_cond_broadcast(cond)){
    perror("pthread_cond_broadcast failed");
    exit(-1);
  }
}

void scond_wait(scond_t *cond, smutex_t *mutex)
{
  //
  // assert(mutex is held by this thread);
  //

  if(pthread_cond_wait(cond, mutex)){
    perror("pthread_cond_wait failed");
    exit(-1);
  }
}



void sthread_create(sthread_t *thread,
		    void (*start_routine(void*)), 
		    void *argToStartRoutine)
{
  //
  // When a detached thread returns from
  // its entry function, the thread will be destroyed.  If we
  // don't detach it, then the memory associated with the thread
  // won't be cleaned up until somebody "joins" with the thread
  // by calling pthread_wait().
  //
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  if(pthread_create(thread, &attr, start_routine, argToStartRoutine)){
      perror("pthread_create failed");
      exit(-1);
  }


}

void sthread_exit(void)
{
  pthread_exit(NULL);
}

void sthread_join(sthread_t thrd)
{
  pthread_join(thrd, NULL);
}


/*
 * WARNING:
 * Do not use sleep for synchronizing threads that 
 * should be waiting for events (using condition variables)!
 * Sleep should only be used to wait for a specified amount
 * of time! (If you find yourself looping on a predicate
 * and calling sleep in the loop, you probably are using
 * it incorrectly! We will deduct points from your grade
 * if you do this!
 */
void sthread_sleep(unsigned int seconds, unsigned int nanoseconds)
{
  struct timespec rqt;
  assert(nanoseconds < 1000000000);
  rqt.tv_sec = seconds;
  rqt.tv_nsec = nanoseconds;
  if(nanosleep(&rqt, NULL) != 0){
    perror("sleep failed. Woke up early");
    exit(-1);
  }
}


/*
 * random() in stdlib.h is not MT-safe, so we need to lock
 * it.
 */
pthread_mutex_t  sulock = PTHREAD_MUTEX_INITIALIZER;

long sutil_random()
{
  long val;

  
  if(pthread_mutex_lock(&sulock)){
    perror("random lock");
    exit(-1);
  }
  val = random();
  if(pthread_mutex_unlock(&sulock)){
    perror("random unlock");
    exit(-1);
  }
  return val;
    
}
