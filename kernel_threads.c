
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
  //ptcb->tcb->owner_pcb = CURPROC;
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
  /*elegxos an to tid einai ths curproc*/
  if (rlist_find(&CURPROC->ptcb_list, (PTCB*)tid, NULL)==NULL){
    return -1;
  }

  else if (cur_thread()->ptcb ==(PTCB*)tid){
    return -1;
  }

  else if(((PTCB*)tid)->detached == 1){
    return -1;
  }

  kernel_wait(&(((PTCB*)tid)->exit_cv),SCHED_USER);

  if(exitval != NULL)
  {
    *exitval = ((PTCB*)tid)->exitval;
  }


  return 0;
	
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
  /* an to thread einai teleutaio*/
  PCB *curproc = CURPROC;
  if(cur_thread()->ptcb->refcount == 0){
    /* Reparent any children of the exiting process to the 
   initial task */
  PCB* initpcb = get_pcb(1);
  while(!is_rlist_empty(& curproc->children_list)) {
    rlnode* child = rlist_pop_front(& curproc->children_list);
    child->pcb->parent = initpcb;
    rlist_push_front(& initpcb->children_list, child);
  }

  /* Add exited children to the initial task's exited list 
     and signal the initial task */
  if(!is_rlist_empty(& curproc->exited_list)) {
    rlist_append(& initpcb->exited_list, &curproc->exited_list);
    kernel_broadcast(& initpcb->child_exit);
  }

  /* Put me into my parent's exited list */
  rlist_push_front(& curproc->parent->exited_list, &curproc->exited_node);
  kernel_broadcast(& curproc->parent->child_exit);



  assert(is_rlist_empty(& curproc->children_list));
  assert(is_rlist_empty(& curproc->exited_list));


/* 
  Do all the other cleanup we want here, close files etc. 
 */

  /* Release the args data */
  if(curproc->args) {
    free(curproc->args);
    curproc->args = NULL;
  }

  /* Clean up FIDT */
  for(int i=0;i<MAX_FILEID;i++) {
    if(curproc->FIDT[i] != NULL) {
      FCB_decref(curproc->FIDT[i]);
      curproc->FIDT[i] = NULL;
    }
  }

  /* Disconnect my main_thread */
  curproc->main_thread = NULL;

  /* Now, mark the process as exited. */
  curproc->pstate = ZOMBIE;

  /* Bye-bye cruel world */
  kernel_sleep(EXITED, SCHED_USER);
  }
  

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