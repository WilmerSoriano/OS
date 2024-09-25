/*Implementing a Fork process*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
   int f1 = fork(); // Making a fork
   /
   if(f1 < 0) // catch/exception
   {
      fprinf(stderr, "fork has failed!");
      exist(-1);
   }
   else if()
   {

   }
   else
   {

   }

   return 0;

}