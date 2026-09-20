/*
* Add NetID and names of all project partners
*
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_TOP_BITS 4 //top bits to extract
#define BITMAP_SIZE 4 //size of the bitmap array
#define SET_BIT_INDEX 17 //bit index to set 
#define GET_BIT_INDEX 17 //bit index to read

static unsigned int myaddress = 4026544704;   // Binary  would be 11110000000000000011001001000000

/* 
 * Function 1: EXTRACTING FIRST CLEAR BIT TO THE RIGHT OF MSB 
 */
static unsigned int first_clear_bit(unsigned int value)
{
    unsigned int flipped = ~value;
    return sizeof(unsigned int) * 8 - __builtin_clz(flipped)-1;
}


/* 
 * Function 2: SETTING A BIT AT AN INDEX 
 * Function to set a bit at "index" bitmap
 */
static void set_bit_at_index(char *bitmap, int index)
{	
    bitmap[index / (sizeof(char) * 8)] |= (1 << (index % sizeof(char) * 8));
    printf("Bit set at index %d\n", index);
    return ;
}


/* 
 * Function 3: GETTING A BIT AT AN INDEX 
 * Function to get a bit at "index"
 */
static int get_bit_at_index(char *bitmap, int index)
{
    //Get to the location in the character bitmap array
    //Implement your code here
    printf("Bit at index %d is %d\n", index, bitmap[index / (sizeof(char) * 8)] & (1 << (index % sizeof(char) * 8)));
    return bitmap[index / (sizeof(char) * 8)] & (1 << (index % sizeof(char) * 8));
}


int main () {

    /* 
     * Function 1: Finding the location of the first clear bit. Suppose your
     * input is decimal 22 (binary 10110), it should return value 3   
    */
    unsigned int clear_bit_loc = first_clear_bit (myaddress);
    printf("Function 1: Clear bit location %u \n", clear_bit_loc); 
    printf("\n");

    /* 
     * Function 2 and 3: Checking if a bit is set or not
     */
    char *bitmap = (char *)malloc(BITMAP_SIZE);  //We can store 32 bits (4*8-bit per character)
    memset(bitmap,0, BITMAP_SIZE); //clear everything
    
    // printf("%d\n", get_bit_at_index(bitmap, GET_BIT_INDEX));

    /* 
     * Let's try to set the bit 
     */
    set_bit_at_index(bitmap, SET_BIT_INDEX);
    // printf("%d\n", get_bit_at_index(bitmap, SET_BIT_INDEX));
    /* 
     * Let's try to read bit)
     */
    printf("Function 3: The value at %dth location %d\n", 
            GET_BIT_INDEX, get_bit_at_index(bitmap, GET_BIT_INDEX));
            
    return 0;
}
