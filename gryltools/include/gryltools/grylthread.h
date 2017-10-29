#ifndef GRYLTHREAD_H_INCLUDED
#define GRYLTHREAD_H_INCLUDED

/************************************************************  
 *         GrylloThreads C-Multithreading Framework         *
 *                ~ ~ ~ By GrylloTron ~ ~ ~                 *
 *                       Version 0.3                        *
 *        -  -  -  -  -  -  -  -  -  -  -  -  -  -  -       *      
 *                                                          *
 *  Features:                                               *
 *  - Cross-Platform concurrent execution framework         *
 *    - Currently supports POSIX and Win32                  *
 *  - Currently supported Native concurrency entities:      *
 *    - Thread                                              *
 *    - Mutex                                               * 
 *    - Condition Variable                                  *
 *                                                          *
 *  TODOS:
 *  - Process management API (Partly implemented).
 *  - Maybe split SharedMutex and SimpleMutex
 *  
 *  BUGS:
 *  - Currently no spotted, but must check more on POSIX.
 *                                                          *
 ***********************************************************/ 

#define GTHREAD_VERSION "v0.3"

/*! The typedef'd primitives
 *  Implementation is defined in their respective source files.
 */ 
typedef void *GrThread;
typedef void *GrMutex;
typedef void *GrSharedMutex;
typedef void *GrCondVar;
typedef void *GrProcess;

/*! Specific attribute flags
 */
// If set, mutex will be shared among processes.
#define GTHREAD_MUTEX_SHARED  1

/*! Threading functions
 *  Supports creation, checking if running, joining, etc.
 */ 
GrThread gthread_Thread_create(void (*proc)(void*), void* param);
void gthread_Thread_join(GrThread hnd, char destroy);
void gthread_Thread_detach(GrThread hnd);
void gthread_Thread_destroy(GrThread hnd);
void gthread_Thread_terminate(GrThread hnd);

char gthread_Thread_isRunning(GrThread hnd);
char gthread_Thread_isJoinable(GrThread hnd);

long gthread_Thread_getID(GrThread hnd);
char gthread_Thread_equal(GrThread t1, GrThread t2);
void gthread_Thread_sleep(unsigned int millisecs);
void gthread_Thread_exit();

/*! Process Functions.
 *  Allow process creation, joining, exitting, and Pid-operations.
 */ 
GrProcess gthread_Process_create(const char* pathToFile, const char* commandLine);
GrProcess gthread_Process_fork(void (*proc)(void*), void* param);
void gthread_Process_join(GrProcess hnd);
char gthread_Process_isRunning(GrProcess hnd);
long gthread_Process_getID(GrProcess hnd);

/*! Mutex functions.
 *  Allow all basic mutex operations.
 */ 
GrMutex gthread_Mutex_init(int attribs);
void gthread_Mutex_destroy(GrMutex* mtx);

char gthread_Mutex_lock(GrMutex mtx);
char gthread_Mutex_tryLock(GrMutex mtx);
char gthread_Mutex_unlock(GrMutex mtx);

/*! CondVar functions
 */ 
GrCondVar gthread_CondVar_init();
void gthread_CondVar_destroy(GrCondVar* cond);

char gthread_CondVar_wait(GrCondVar cond, GrMutex mtp);
char gthread_CondVar_wait_time(GrCondVar cond, GrMutex mtp, long millisec);
void gthread_CondVar_notify(GrCondVar cond);
void gthread_CondVar_notifyAll(GrCondVar cond);


#endif //GRYLTHREAD_H_INCLUDED
