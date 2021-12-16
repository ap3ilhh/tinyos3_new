
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"
#include "util.h"
#include "tinyos.h"
#include "kernel_streams.h"

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
	return (Tid_t) cur_thread()->ptcb;
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

  /*an to thread zhta apo ton euato tou na tou kanei join*/
  else if (cur_thread()->ptcb ==(PTCB*)tid){
    return -1;
  }

  /*an to thread dn einai joinable*/
  else if(((PTCB*)tid)->detached == 1 ){
    return -1;
  }

  //mporei na ginei threadjoin 
  ((PTCB*)tid)->refcount ++;

  while(((PTCB*)tid)->exited==0 && ((PTCB*)tid)->detached==0)
  {
    kernel_wait(&((PTCB*)tid)->exit_cv, SCHED_USER);
  }
  
  //an egine detach epestrepse
  if(((PTCB*)tid)->detached == 1)
  {
    return -1;
  }

  /*to tid exei teleiwsei*/

  ((PTCB*)tid)->refcount --;  /*tote meiwnw to refcount*/

  if(exitval != NULL)
  {
    *exitval = ((PTCB*)tid)->exitval;
  }

 /*free to ptcb*/
  if(((PTCB*)tid)->refcount == 0){
    rlist_remove(&((PTCB*)tid)->ptcb_list_node);
    free(((PTCB*)tid));
  }

  return 0;
	
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{ 

  PTCB* ptcb = (PTCB*)tid;

    /*elegxos an to tid einai ths curproc*/
  if (rlist_find(&CURPROC->ptcb_list, (PTCB*)tid, NULL)==NULL){
    return -1;
  }

  /*an to thread einai detached*/
  if (ptcb->detached == 1 ){
    return 0;
  } 

  /*to thread einai exited*/
  if (ptcb->exited == 1){
    return -1;
  }
  /*thread detached*/
  ptcb->detached = 1;

  /*ksupnaei ola ta threads pou to perimenan*/
  kernel_broadcast(&(ptcb->exit_cv));

	return 0;
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
  PCB *curproc = CURPROC;
  TCB* curthread = cur_thread();
  PTCB* curptcb = curthread->ptcb;

  curptcb->exitval = exitval;
  curptcb->exited = 1;
  curptcb->tcb = NULL;


  /*meiwsh tou counter pou metraei ta threads sto pcb*/
  curproc->thread_count--;

  /*ksupanei ola ta threads pou to perimenan*/
  kernel_broadcast(&(curptcb->exit_cv));

  /* an to thread einai teleutaio*/
  if(curproc->thread_count == 0)
  {
    if(get_pid(curproc)!=1)
    {  

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

    }

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
    
    /*free ola ta ptcb*/
    /*rlnode* ptcb_l_node;
    while(!is_rlist_empty(&CURPROC->ptcb_list)){
      ptcb_l_node = rlist_pop_front(&CURPROC->ptcb_list);
      free(ptcb_l_node->ptcb);
    }*/
  }




  /* Bye-bye cruel world */
  kernel_sleep(EXITED, SCHED_USER);

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