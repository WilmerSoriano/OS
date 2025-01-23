/* Test 2 -- test queue1, the priority-based queue
In this test, we set waiting threshold to a large number
--- process in the priority queue cannot move back to the round robin queue

In the beginning, C is forced to stay in the round robin queue (using set_binding)
After a while, C is set to unbind from  queue0, and will move to queue1 (priority-based).
During this time, process A and B have already accumulate some waiting ticks.

So after C moves to queue1, we expect that process A and B will first execute for a while,
before process C starts executing (as C has the lowest waiting ticks[priority]).

The results after C unbinds from queue0 should be:
......ABABABABABAB.....ABCABCABC....

or something like
......AABBAABBAABB.....ABBCBBABBCBB......
(depends on how you deal with two processes with the same priority value).
*/

#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

void loop()
{
  int i;
  int j=0;
  for(i=0;i<1000000;i++)
    j=j+1;
}
void
forktest(void)
{
  int pid1;
  int ret;
  int fds1[2];

  ret = set_running_threshold(2);
  if (ret < 0)
  {
    printf(1, "cannot set running threshold\n");
    exit();
  }
  ret = set_waiting_threshold(1000000);
    if (ret < 0)
  {
    printf(1, "cannot set waiting threshold\n");
    exit();
  }

  ret = pipe(fds1);
  if ( ret < 0)
  {
    printf(1, "cannot create a pipe\n");
    exit();
  }

  pid1 = fork();
  if(pid1 < 0)
    return;
    
  if(pid1 == 0){
      int i;
      char buf[256];
      // block here
      close(fds1[1]);
      read(fds1[0], buf, 1);
      printf(1, "\n start process A [%d]\n", getpid());

      for (i=0;i<100;i++)
      {
        //printf(1, "Program C[%d] %d\n", getpid(), i); //=========inspect
	loop();
      }
  }
  else
  {
    int pid2;
    int fds2[2];
    ret = pipe(fds2);

    ret = pipe(fds2);
    if (ret < 0)
    {
        printf(1, "cannot create the second pipe\n");
    }

    pid2 = fork();
    if(pid2 < 0)
      return;
    
    if(pid2 == 0){
      int i;
      char buf[256];
      close(fds1[0]);
      close(fds2[1]);
      //block here
      read(fds2[0], buf, 1);
      write(fds1[1], "Done", 5);
      printf(1, "\n start process B [%d]\n", getpid());

      for (i=0;i<100;i++)
      {
	//printf(1, "Program B[%d] %d\n", getpid(), i); //==========inspect
        loop();
      }
    }
    else
    {
      int i;
      close(fds1[0]);
      close(fds1[1]);
      close(fds2[0]);
      write(fds2[1],"Done", 5);
      printf(1, "\n start process C [%d]\n", getpid());


      set_binding(getpid(), 0);
      for (i=0;i<25;i++)
      {
        //printf(1, "Program A[%d] %d\n", getpid(), i);//=========inspect
        loop();
      }
      set_binding(getpid(), 1);
      printf(1, "\n C prority changes\n");

      for (i=0;i<50;i++)
      {
        //printf(1, "Program A[%d] %d\n", getpid(), i);// ============inspect
        loop();
      }

      wait();
      wait();
    }
  }
}

int
main(void)
{
  forktest();
  //printf(1, "Finished!");
  exit();
}
