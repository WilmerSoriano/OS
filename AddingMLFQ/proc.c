#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"


// CSE3320 select which scheduler to use by hard coding and recompiling: 0 for default RR, and 1 for MLFQ
int sched_policy = 1;
// CSE3320 CPU/process project
int sched_trace_enabled = 1;

// CSE3320 default running_threshold and waiting_threshold
int RUNNING_THRESHOLD = 2;
int WAITING_THRESHOLD = 4;

// CSE3320 create mlfq which contains two process arrays: rr_queue and pri_queue
// num_0: the number of processes in rr_queue
// num_1: the number of processes in pri_queue
struct {
  struct proc *rr_queue[NPROC];
  struct proc *pri_queue[NPROC];
  int num_0;
  int num_1;
} mlfq;

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;


static struct proc *initproc;

int nextpid = 1;


extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);



void
pinit(void)
{
  initlock(&ptable.lock, "ptable");

  // CSE3320 initialize num_0 and num_1 in mlfq
  mlfq.num_0 = 0;
  mlfq.num_1 = 0;
}

// CSE3320 set if process [pid] binding in rr_queue
// 0: yes
// 1: no
int set_binding(int pid, int pinned)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  {
    if(p->pid == pid)
    {
      p->pinned = pinned;
      return 0;
    }
  }

  return -1;
}

// CSE3320 add a process into rr_queue
void add_to_rr_queue(struct proc *p)
{
  mlfq.rr_queue[mlfq.num_0] = p;
  mlfq.num_0++;
}

// CSE3320 add a process into pri_queue
void add_to_pri_queue(struct proc *p)
{
  mlfq.pri_queue[mlfq.num_1] = p;
  mlfq.num_1++;
}

// CSE3320 remove a process from rr_queue
void rm_from_rr_queue(struct proc *p)
{
  for(int i = 0; i < mlfq.num_0; i++)
  {
    if(mlfq.rr_queue[i]->pid == p->pid)
    {
      for(int j = i; j < mlfq.num_0; j++)
      {
        mlfq.rr_queue[j] = mlfq.rr_queue[j+1];
      }
      mlfq.rr_queue[mlfq.num_0] = 0;
      mlfq.num_0--;

      p->running_tick = 0;
      p->waiting_tick = 0;

      break;
    }
  }
}

// CSE3320 remove a process from pri_queue
void rm_from_pri_queue(struct proc *p)
{
  for(int i = 0; i < mlfq.num_1; i++)
  {
    if(mlfq.pri_queue[i]->pid == p->pid)
    {
      for(int j = i; j < mlfq.num_1; j++)
      {
        mlfq.pri_queue[j] = mlfq.pri_queue[j+1];
      }
      mlfq.pri_queue[mlfq.num_1] = 0;
      mlfq.num_1--;

      p->running_tick = 0;
      p->waiting_tick = 0;

      break;
    }
  }
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;
  release(&ptable.lock);
  return 0;

found:
  // CSE3320 add the process into rr_queue when initializing
  if(sched_policy == 1)
  {
    add_to_rr_queue(p);
  }

  p->state = EMBRYO;
  p->pid = nextpid++;
  // CSE3320 initialize the process new states
  p->pinned = 1;
  p->running_tick = 0;
  p->waiting_tick = 0;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  p = allocproc();

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));
 
  pid = np->pid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        // CSE3320 When a process finishes running, remove it from rr_queue or pri_queue by its parent
        rm_from_pri_queue(p);
        rm_from_rr_queue(p);

        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

// CSE3320: xv6's default RR scheduler
int sched1(void)
{
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
  }
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE)
	return 1;
  }
  return 0;
  
}

// CSE3320
void update_running_tick_check_demotion(struct proc *p)
{
  p->running_tick++;

  if((p->running_tick >= RUNNING_THRESHOLD) && (p->pid != 1) && (p->pid != 2) && (p->pinned != 0))
  {
    rm_from_rr_queue(p);
    add_to_pri_queue(p);
  }
}

// CSE3320
void update_waiting_tick_check_promotion(struct proc *p)
{
  struct proc *q;

  for(int i = 0; i < mlfq.num_1; i++)
  {
    q = mlfq.pri_queue[i];

    if (q->pid == p->pid)
    {
      continue;
    }

    q->waiting_tick++;

    if (q->waiting_tick >= WAITING_THRESHOLD || q->pinned == 0)
    {
      rm_from_pri_queue(q);
      add_to_rr_queue(q);
    }
  }
}

// CSE3320
int find_max_waiting_tick_index()
{
	struct proc *process;
	int checkTick = 0;
	int maxIndex = -1;
	int i = -1;

	if(mlfq.num_1 == 1) // If there is no process, another than the sh shell, then skip, and return 0
	{
		return 0;
	}
	for(i = 0; i < mlfq.num_1; i++)
	{
		process = mlfq.pri_queue[i];
	
		if((process->pid != 1) && (process->pid != 2) && (process->pinned != 0))
		{
			//printf("\nstate: %d, name: %s, waiting tick:%d, id %d, pinned:%d and index:%d\n", process->state, process->name, process->waiting_tick, process->pid, process->pinned, i);
			if(process->waiting_tick > checkTick)
			{
				checkTick = process->waiting_tick;
				maxIndex = i; 
			//cprintf("it worked tick %d, index: %d and number of pri_queue:%d", checkTick, maxIndex, mlfq.num_1);
			//return maxIndex;
			}	

		}
	}
	return maxIndex;
}

// CSE3320: Multi-level queue feedack scheduling
int sched2(void)
{
  struct proc *p;
  int rr_found = 0;

  // Queue 0 - RR
  for(int i = 0; i < mlfq.num_0; i++)
  {
    p = mlfq.rr_queue[i];

    if(p->state != RUNNABLE)
    {
      continue;
    }

    rr_found = 1;

    proc = p;
    switchuvm(p);
    p->state = RUNNING;
    swtch(&cpu->scheduler, proc->context);
    switchkvm();

    update_running_tick_check_demotion(p);
    update_waiting_tick_check_promotion(p);

    proc = 0;
  }

  // Queue 1 - priority-based
  if(rr_found == 0)
  {
    int max_waiting_index = find_max_waiting_tick_index();

    if(max_waiting_index >= 0)
    {
      struct proc *q;
      q = mlfq.pri_queue[max_waiting_index];

      proc = q;
      switchuvm(q);
      q->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      update_waiting_tick_check_promotion(q);

      proc = 0;
    }
  }

  // fix the 100% cpu usage bug
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE)
        return 1;
  }
  return 0;
}


// CSE3320: Modify existing scheduler to support two schedulers
void
scheduler(void)
{
  //struct proc *p;
  int ran = 0; // CSE3320: to solve the 100%-CPU-utilization-when-idling problem

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    if (sched_policy)
    	ran = sched2();    //MLFQ
    else
	    ran = sched1();    //default RR
    release(&ptable.lock);

    if (ran == 0){
        halt();
    }
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;

  //CSE3320 tracing function
  if ( sched_trace_enabled && 
	proc && 
	proc->pid != 1
	&& (proc->pid != 2))
   	cprintf("[%d]", proc->pid);

  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot 
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
