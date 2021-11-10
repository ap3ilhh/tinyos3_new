
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"
#include "util.h"
#include "tinyos.h"

void start_new_thread();

/** 
  @brief Create a new thread in the current process.
  */
Tid_t sys_CreateThread(Task task, int argl, void* args)
{ 
  PTCB* ptcb;
  ptcb = (PTCB*)xmalloc(sizeof(PTCB));
  initialize_PTCB(ptcb);

  //enhmerwsh pcb
  CURPROC->thread_count++;

  //enhmerwsh ptcb
  ptcb->tcb = spawn_thread(CURPROC,start_new_thread);

  ptcb->task = task;
  ptcb->argl = argl;
  ptcb->args = args;

  ptcb->exited = 0;
  ptcb->detached = 0;
  ptcb->exit_cv = COND_INIT; //???????????????

  ptcb->refcount = 0;

  //vazw sth process sth lista me ta ptcb to kainourio ptcb
  rlist_push_back(&CURPROC->ptcb_list,&ptcb->ptcb_list_node);

  //enhmrwsh tcb
  ptcb->tcb->owner_pcb = CURPROC;
  ptcb->tcb->ptcb = ptcb;


  wakeup(ptcb->tcb);

	return (Tid_t)ptcb;
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
	return (Tid_t) cur_thread();
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
	return -1;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
	return -1;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{

}

void start_new_thread()
{
  int exitval;

  Task call = cur_thread()->ptcb->task;
  int argl = cur_thread()->ptcb->argl;
  void* args = cur_thread()->ptcb->args;

  exitval = call(argl,args);
  ThreadExit(exitval);
}