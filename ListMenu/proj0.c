/*
   Wilmer Soriano
   ID:1001885481
*/

/*Project 0 Making a simulation to the echo command but with different string text:
  CSE3320 proj0 printing in user space:*/

#include "types.h" // Refrence code from echo command
#include "stat.h"
#include "user.h"


int main(int argc, char *argv[])
{
   int i;

   printf(1,"CE3320 proj0 printing in user space: "); //Just added comment 
   
   for(i = 1; i < argc; i++){
	   printf(1,"%s%s", argv[i], i+1 < argc ? " " : "\n");
   }

   exit();
}
