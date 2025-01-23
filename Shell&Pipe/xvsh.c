#include "types.h"
#include "fcntl.h"
#include "user.h"

#define SH_PROMPT "xvsh> "
#define NULL (void *)0


char *strtok(char *s, const char *delim);

int process_one_cmd(char *);

#define MAXLINE 256

int main(int argc, char *argv[])
{
    char buf[MAXLINE];
    int i;
    int n;
    printf(1, SH_PROMPT);  /* print prompt (printf requires %% to print %) */

    while ( (n = read(0, buf, MAXLINE)) != 0) 
    {
        if (n == 1)                           /* no input at all, we should skip */
        {
            printf(1, SH_PROMPT);
            continue;			//The space option has been added
        }
        buf[i = (strlen(buf) - 1)] = 0;       /* replace newline with null */
	
        process_one_cmd(buf);
        
        printf(1, SH_PROMPT);
      
        memset(buf, 0, sizeof(buf));
    }
    
    exit();
}

int exit_check(char **tok, int num_tok) // Note: strcmp not availble 
{
	char str[4] = "exit";  
	int a = 0, b = 0;

	for(a = 0; a < 4; a++)
		if(str[a] == tok[0][a])// Comapres only the first command to the word 'exit'
			++b;

	if(b == 4)// If all 4 words match up then it must be exit!
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

int process_normal(char **tok, int bg) // Implemeting fork from sh. file 
{
	int f1 = fork(); // create a fork
	int val = 0;  //checks the if a command is real or not

	if (f1 < 0) // checks and catches exeception
	{
		printf(1, "fork failed\n");
		exit();
	}
	else if (f1 == 0) // the process where we fork sh. filie
	{
		val = exec(*tok, tok); // note that exec(*tok, tok) is the right way to invoke exec in xv6
		
		if(val != 0)
		{
			printf(1,"Cannot run this command %s\n", *tok);
			kill(f1);
			exit();
		}
	}
	else
	{
		if( bg != 0) // If the flag was set, then leave function, will comeback to finish the wait()
		{	    // do to the while loop found in main.

			printf(1,"[pid %d] runs as a background process\n",getpid());
			return 0;
		}
		else
		{
			wait();
		}
	}

    return 0;
}

int process_multiple(char **tok)
{
	int fds[2];
	int f1, f2;
	int i;
	char *token1[2] = {tok[0], NULL}; // Takes the first command and Null the rest (eg. |cmd2)   
        char *token2[2] = {tok[2], NULL};// same as above. However, we move cmd2 as cmd1 using '{}'

	if(pipe(fds) == -1){
		printf(1,"pipe failed");
		exit();
	}

	f1 = fork();
	if(f1 < 0){
		printf(1,"fork1 failed");
		kill(f1);
		exit();
	}

	if(f1 == 0)
	{
		close(1);	// close the stdout writter
		dup(fds[1]);   // connect the pipe to write
		close(fds[0]);// close the reader pipe
		if(exec(token1[0], token1) < 0){ 
			printf(1,"failed to exec 1st command");
			kill(f1);
			exit();
		}
	}

	f2 = fork();
	if(f2 < 0){
		printf(1,"fork2 failed");
		kill(f2);
		exit();
	}

	if(f2 == 0)
	{
		close(0);	// close the stdin reader
		dup(fds[0]);   // connect the pipe to read
		close(fds[1]);// close the writter pipe
		if(exec(token2[0], token2) < 0){
			printf(1,"failed to exec 2nd command");
			kill(f2);
			exit();
		}
	}

	close(fds[0]);
	close(fds[1]);

	for(i = 0; i < 2; i++){ wait(); } 

	return 0;
}

int redirection(char **tok)
{
	int a;
	int f1 = fork();
	char *cmd[2] = {tok[0],NULL};

	if(f1 < 0)
	{
		printf(1,"fork failed\n");
		kill(f1);
		exit();
	}
	else if(f1 == 0)
	{
		a = open(tok[2], 1);  // Open the file/ writter(1)
		close(1);	// close the write on stdout
		dup(a);        // copy the file/writter to the stdout connection

		if(exec(cmd[0],cmd) < 0){
			printf(1,"redirecting file failed");
			kill(f1);
			exit();
		}
	}
	else
	{
		wait();
	}

	return 0;
}

int process_one_cmd(char* buf)
{
    int i, num_tok;
    char **tok;
    int bg;
    int mp = 0, point = 0;
    i = (strlen(buf) - 1);
    num_tok = 1;

    while (i)
    {
        if (buf[i--] == ' ')
            num_tok++;
    }

    if (!(tok = malloc( (num_tok + 1) *   sizeof (char *)))) 
    {
        printf(1, "malloc failed\n");
        exit();
    }        


    i = bg = 0;
    tok[i++] = strtok(buf, " ");

    /* check special symbols */
    while ((tok[i] = strtok(NULL, " "))) 
    {
        switch (*tok[i]) 
        {
	    case '&': // If user indicates for bg, then set a flag
            	bg = i;
            	tok[i] = NULL;
            	break;
	    case '|': // multiple process command
		mp = i;
		tok[i] = "";
		break;
	    case'>': // redirecting the output
		point = i;
		break;
            default:
            	// do nothing
            	break;
        }
        i++;
    }

    int check = 0;
    /*Check buid-in exit command */
    if (exit_check(tok, num_tok))
    {
        /*some code here to wait till all children exit() before exit*/
	check = wait();
	while(0 <= check)
	{
		check = wait();
	}

	free(tok);
        exit();
    }

    if(mp != 0 ){
	    process_multiple(tok);
    }
    else if(point != 0){
	    redirection(tok);
    }
    else{
    /* to process one command*/
    process_normal(tok, bg);
    }

    free(tok);
    return 0;
}



char *
strtok(s, delim)
    register char *s;
    register const char *delim;
{
    register char *spanp;
    register int c, sc;
    char *tok;
    static char *last;


    if (s == NULL && (s = last) == NULL)
        return (NULL);

    /*
     * Skip (span) leading delimiters (s += strspn(s, delim), sort of).
     */
cont:
    c = *s++;
    for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
        if (c == sc)
            goto cont;
    }

    if (c == 0) {        /* no non-delimiter characters */
        last = NULL;
        return (NULL);
    }
    tok = s - 1;

    /*
     * Scan token (scan for delimiters: s += strcspn(s, delim), sort of).
     * Note that delim must have one NUL; we stop if we see that, too.
     */
    for (;;) {
        c = *s++;
        spanp = (char *)delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                last = s;
                return (tok);
            }
        } while (sc != 0);
    }
    /* NOTREACHED */
}

