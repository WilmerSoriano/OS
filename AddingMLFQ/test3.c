/* Test 3 -- test demotion and promotion 
In this test, we set both running threshold and waiting threshold to a reasonable value
We bind process B staying in the round-robin queue.

So, we expect that:
(1) process A and B should first execute in the round robin queue for a while;
...ABABABABABAB...
(2) Then process A demotes to the second queue, priority-based queue.
As process B is in the queue0 with higher priority, B will be executing during this period;
...BBBBBBBBBBBB...
(3) Then process A accumulates waiting ticks and moves back to the round robin queue.
...ABABABABAB...

Overall, the expected results should be:
......ABABABABABAB...BBBBBBBBBB...ABABABABAB......
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

  ret = set_running_threshold(10);
  if (ret < 0)
  {
    printf(1, "cannot set running threshold\n");
    exit();
  }
  ret = set_waiting_threshold(20);
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
        //printf(1, "Program C[%d] %d\n", getpid(), i);
	loop();
      }
  }
  else
  {
      int i;
      close(fds1[0]);
      write(fds1[1],"Done", 5);
      printf(1, "\n start process B [%d]\n", getpid());

      // B process always in queue 0
      set_binding(getpid(), 0);

      for (i=0;i<100;i++)
      {
        //printf(1, "Program A[%d] %d\n", getpid(), i);
        loop();
      }
      wait();
  }
}

int
main(void)
{
  forktest();
  exit();
}
