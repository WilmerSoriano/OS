/*
  * Project 3: shared memory 
  */

#include "types.h"
#include "user.h"

void test_case1()
{
   //We fork two processes attaching to the same shared page
   //one first write a string to it
   //the other can obtain the exact same string later
   //Once done the shared page is deleted
   //Thoughout the whole process, only one physical frame is used
   //Once the shared page deleted, the phyiscal frame number should be the same
   //as the beginning
   //
   printf(1, "\n**********Test 1***************\n");
   print_free_frame_cnt();
   
   //fork the first process
   int pid = fork();
   if (pid == 0)
   {
       char *vaddr = shmget(0);
       printf(1, "PID:%d: return addr:0x%x for shared page:%d\n", getpid(), vaddr, 0);
       strcpy(vaddr, "Hello world\n");
       printf(1, "PID:%d: string stored in addr:0x%x is %s\n", getpid(), vaddr, vaddr);
       exit();
   }
   wait();
   print_free_frame_cnt();

   //fork the second process
   pid = fork();
   if (pid == 0)
   {
       char *vaddr = shmget(0);
       printf(1, "PID:%d: return addr:0x%x for shared page:%d\n", getpid(), vaddr, 0);
       printf(1, "PID:%d: string stored in addr:0x%x is %s\n", getpid(), vaddr, vaddr);
       exit();
   }
   wait();
   print_free_frame_cnt();

   //for the third one which delete the shared page 0
   pid = fork();
   if (pid == 0)
   {
       printf(1, "PID:%d: delete shared page:%d\n", getpid(), 0);
       shmdel(0);
       exit();
   }
   wait();
   print_free_frame_cnt();

   return;
}

void test_case2()
{
   //We fork the first process, which attaches to shared page 0, writes a string to it, and 
   //quickly deleted the shared page 0.
   //
   //We then fork the second process, which attaches to the same shared page 0. 
   //However, since the shared page has been deleted by the first process, the second process attaches
   //to a brand new shared page 0. It can not fetch the string written by the first process
   printf(1, "\n**********Test 2***************\n");
   print_free_frame_cnt();

   // fork the first process
   int pid = fork();
   if (pid == 0)
   {
       char *vaddr = shmget(0);
       printf(1, "PID:%d: return addr:0x%x for shared page:%d\n", getpid(), vaddr, 0);
       strcpy(vaddr, "Hello world\n");
       printf(1, "PID:%d: string stored in addr:0x%x is %s\n", getpid(), vaddr, vaddr);
       // we delete the shared page
       shmdel(0);
       exit();
   }
   wait();
   print_free_frame_cnt();

   // fork the second process
   pid = fork();
   if (pid == 0)
   {
       char *vaddr = shmget(0);
       printf(1, "PID:%d: return addr:0x%x for shared page:%d\n", getpid(), vaddr, 0);
       printf(1, "PID:%d: string stored in addr:0x%x is %s\n", getpid(), vaddr, vaddr);
       exit();
   }
   wait();
   print_free_frame_cnt();

   // for the third process to deleted the shared page
   pid = fork();
   if (pid == 0)
   {
       printf(1, "PID:%d: delete shared page:%d\n", getpid(), 0);
       shmdel(0);
       exit();
   }
   wait();
   print_free_frame_cnt();

   return;
}

void test_case3()
{
   //We fork two processes which both attach to the same shared page 0
   //Before these two processes exit, the third process tries to delete the shared page
   //As the shared page are referenced by exisitng processes, it should not be released
   //
   //Finally, once those two processes exit, we delete the shared page again. This time, 
   //it should succeed.
   printf(1, "\n**********Test 3***************\n");
   print_free_frame_cnt();

   // fork the first process
   int pid = fork();
   if (pid == 0)
   {
       char *vaddr = shmget(0);
       printf(1, "PID:%d: return addr:0x%x for shared page:%d\n", getpid(), vaddr, 0);
       strcpy(vaddr, "Hello world\n");
       printf(1, "PID:%d: string stored in addr:0x%x is %s\n", getpid(), vaddr, vaddr);
       sleep(10);
       exit();
   }

   // fork the second process
   pid = fork();
   if (pid == 0)
   {
       char *vaddr = shmget(0);
       printf(1, "PID:%d: return addr:0x%x for shared page:%d\n", getpid(), vaddr, 0);
       printf(1, "PID:%d: string stored in addr:0x%x is %s\n", getpid(), vaddr, vaddr);
       sleep(10);
       exit();
   }

   // fork the third process trying to delete the shared page
   pid = fork();
   if (pid == 0)
   {
       printf(1, "PID:%d: delete shared page:%d\n", getpid(), 0);
       shmdel(0);
       exit();
   }
   wait();
   wait();
   wait();

   // fork the third process to delete the shared page again
   pid = fork();
   if (pid == 0)
   {
       printf(1, "PID:%d: delete shared page:%d again\n", getpid(), 0);
       shmdel(0);
       exit();
   }
   wait();
   print_free_frame_cnt();

   return;
}

void test_case4()
{
   // Some corner test cases
   // for example, we only support at most 10 shared pages
   // A shared page should be deleted twice
   printf(1, "\n**********Test 4***************\n");
   print_free_frame_cnt();

   //fork the first process
   int pid = fork();
   if (pid == 0)
   {
       char *vaddr;
       int i;
       for(i=0;i<=10;i++)
       {
           vaddr = shmget(i);
       	   printf(1, "PID:%d: return addr:0x%x for shared page:%d\n", getpid(), vaddr, i);
       }
       exit();
   }
   wait();
   print_free_frame_cnt();

   //fork the second process
   pid = fork();
   if (pid == 0)
   {
       int i;
       for(i=0;i<=10;i++)
       {
           printf(1, "PID:%d: delete shared page:%d\n", getpid(), i);
	   shmdel(i);
       }
       // double deletion
       shmdel(0);
       exit();
   }
   wait();
   print_free_frame_cnt();

   // for the third process
   pid = fork();
   if (pid == 0)
   {
       char *vaddr;
       vaddr = shmget(0);
       printf(1, "PID:%d: return addr:0x%x for shared page:%d\n", getpid(), vaddr, 0);
       vaddr[0] = 'a';
       printf(1, "PID:%d: char stored at addr:0x%x is %c\n", getpid(), vaddr, vaddr[0]);
       shmdel(0);
       vaddr[0] = 'b';
       printf(1, "PID:%d: char stored at addr:0x%x is %c\n", getpid(), vaddr, vaddr[0]);
       exit();
   }
   wait();
   print_free_frame_cnt();

   return;
}

int
main(int argc, char *argv[])
{

   // test 1
   test_case1();

   // test 2
   test_case2();

   // test 3
   test_case3();

   // test 4
   test_case4();
   exit();   
}

