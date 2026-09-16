/*
* Add NetID and names of all project partners
*
*/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>


/* Part 1 - Step 1 and 2: Do your tricks here
 * Your goal must be to change the stack frame of caller (main function)
 * such that you get to the line after "read_only_string[0] = 'X';"
 */
void signal_handle(int signalno) {

    printf("Handling segmentation fault!\n");

    /* Step 2: Handle segmentation fault and change the stack*/
    //offending instruction length is 6 bytes.

    int *ret_add_loc = &signalno + 15;
    *ret_add_loc += 6;
    
    return ;

}

int main(int argc, char *argv[]) {

    char *read_only_string = "This text cannot be modified.";

    /* Step 1: Register signal handler first*/
    signal(SIGSEGV, signal_handle);

    read_only_string[0] = 'X';
    
    printf("Your stack manipulation worked !!!\n");

    return 0;
}
