/*
  * Project 2: lazy page allocation test
  */

#include "types.h"
#include "user.h"

void usage(void)
{
    printf(1, "Usage: proj2_lpatest 0|1 \n"
              "\t0: Default page allocator \n"
              "\t1: Lazy page allocator \n");
}
  
int
main(int argc, char *argv[])
{
    int ret = 0;
    int a = 0;
    int size = 0;
    char * addr = (void *)0;
    char * cur_break = (void *)0;
    char * old_break = (void *)0;

    if (argc < 2)
    {
        usage();
        exit();
    }

    if (argv[1][0] == '0')
    {
        a = 0;
        printf(1, "\nUsing the DEFAULT page allocator ...\n");
    }
    else
    {
        a = 1;
        printf(1, "\nUsing the LAZY page allocator ...\n");
    }  

    ret = set_page_allocator(a);
    if ( ret == -1)
    {
    	printf(1, "\nImplement the LAZY page allocator before tests:%d\n", ret);
	exit();
    }

    //=========================
    size = 10;
    printf(1, "\n=========== TEST 1: sbrk(%d) ===========\n", size);
    cur_break = sbrk(0);
    printf(1, "Before sbrk(%d), break 0x%x ", size, cur_break);
    print_free_frame_cnt();
    printf(1, "Calling sbrk(%d) ... \n", size);
    sbrk(size);
    cur_break = sbrk(0);
    printf(1, "After sbrk(%d), break 0x%x ", size, cur_break);
    print_free_frame_cnt();


    printf(1, "\n=========== TEST 2: writing valid bytes ===========\n", addr);
    cur_break = sbrk(0);
    printf(1, "Before the write, break 0x%x ", cur_break);
    print_free_frame_cnt();
    addr = cur_break;
    printf(1, "Writing byte 0x%x ...\n", addr);
    *addr = 1;
    printf(1, "After the write, ");
    print_free_frame_cnt();

    printf(1, "\nBefore the write, ");
    print_free_frame_cnt();
    addr = cur_break -  1;
    printf(1, "Writing byte 0x%x ...\n", addr);
    *addr = 1;
    printf(1, "After the write, ");
    print_free_frame_cnt();


    printf(1, "\n=========== TEST 3: sbrk(+) --> sbrk(-) --> write ===========\n", addr);
    cur_break = sbrk(0);
    printf(1, "Before the sbrk(+), break 0x%x ", cur_break);
    print_free_frame_cnt();
    size = 0x10;
    printf(1, "Calling sbrk(%d) ...\n", size);
    sbrk(size);
    cur_break = sbrk(0);
    printf(1, "After the sbrk(+), break 0x%x ", cur_break);
    print_free_frame_cnt();

    cur_break = sbrk(0);
    printf(1, "\nBefore the sbrk(-), break 0x%x ", cur_break);
    print_free_frame_cnt();
    size = -1;
    printf(1, "Calling sbrk(%d) ...\n", size);
    sbrk(size);
    cur_break = sbrk(0);
    printf(1, "After the sbrk(-), break 0x%x ", cur_break);
    print_free_frame_cnt();

    printf(1, "\nBefore the write, ");
    print_free_frame_cnt();
    addr = cur_break;
    printf(1, "Writing byte 0x%x ...\n", addr);
    *addr = 1;
    printf(1, "After the write, ");
    print_free_frame_cnt();


    printf(1, "\n==========="
              "TEST 4: sbrk(3 pages) --> "
              "write in 1st/2nd pages --> "
              "sbrk(-1 page) --> "
              "sbrk(-1 page) --> "
              "sbrk(-1 page)"
              "===========\n");
    cur_break = sbrk(0);
    old_break = cur_break;
    printf(1, "Before the sbrk(3 pages), break 0x%x ", cur_break);
    print_free_frame_cnt();
    size = 4096 * 3;
    printf(1, "Calling sbrk(%d) ...\n", size);
    sbrk(size);
    cur_break = sbrk(0);
    printf(1, "After the sbrk(3 pages), break 0x%x ", cur_break);
    print_free_frame_cnt();

    printf(1, "\nBefore the write (in 1st page), ");
    print_free_frame_cnt();
    addr = old_break + 4096;
    printf(1, "Writing byte 0x%x ...\n", addr);
    *addr = 1;
    printf(1, "After the write, ");
    print_free_frame_cnt();

    printf(1, "\nBefore the write (in 2nd page), ");
    print_free_frame_cnt();
    addr = old_break + 2 * 4096;
    printf(1, "Writing byte 0x%x ...\n", addr);
    *addr = 1;
    printf(1, "After the write, ");
    print_free_frame_cnt();

    cur_break = sbrk(0);
    old_break = cur_break;
    printf(1, "\nBefore the sbrk(-1 page), break 0x%x ", cur_break);
    print_free_frame_cnt();
    size = -4096;
    printf(1, "Calling sbrk(%d) ...\n", size);
    sbrk(size);
    cur_break = sbrk(0);
    printf(1, "After the sbrk(-1 page), break 0x%x ", cur_break);
    print_free_frame_cnt();

    cur_break = sbrk(0);
    old_break = cur_break;
    printf(1, "\nBefore the sbrk(-1 page), break 0x%x ", cur_break);
    print_free_frame_cnt();
    size = -4096;
    printf(1, "Calling sbrk(%d) ...\n", size);
    sbrk(size);
    cur_break = sbrk(0);
    printf(1, "After the sbrk(-1 page), break 0x%x ", cur_break);
    print_free_frame_cnt();

    cur_break = sbrk(0);
    old_break = cur_break;
    printf(1, "\nBefore the sbrk(-1 page), break 0x%x ", cur_break);
    print_free_frame_cnt();
    size = -4096;
    printf(1, "Calling sbrk(%d) ...\n", size);
    sbrk(size);
    cur_break = sbrk(0);
    printf(1, "After the sbrk(-1 page), break 0x%x ", cur_break);
    print_free_frame_cnt();


    printf(1, "\n=========== TEST 5: allocating too much memory ===========\n", size);
    size = 0x7FFFFFFF;
    cur_break = sbrk(0);
    printf(1, "Before sbrk(0x%x), break 0x%x ", size, cur_break);
    print_free_frame_cnt();
    printf(1, "Calling sbrk(0x%x) ... \n", size);
    sbrk(size);
    cur_break = sbrk(0);
    printf(1, "After sbrk(0x%x), break 0x%x ", size, cur_break);
    print_free_frame_cnt();

    
    printf(1, "\n=========== TEST 6: writing in an unallocated page above the progam break ===========\n", addr);
    cur_break = sbrk(0);
    printf(1, "Before the write, break 0x%x ", cur_break);
    print_free_frame_cnt();
    addr = cur_break + 4096;
    printf(1, "Writing byte 0x%x ...\n", addr);
    *addr = 1;
    printf(1, "After the write, "); // shouldn't reach here
    print_free_frame_cnt(); // shouldn't reach here

    printf(1, "\n");
    //=========================
    
    exit();
}

