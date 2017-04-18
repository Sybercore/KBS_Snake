#include <stdio.h>
#include "includes.h"

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    task1_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define TASK1_PRIORITY      1


/* Prints "Hello World" and sleeps for three seconds */
void task1(void* pdata)
{
	volatile int * PS2_ptr = (int *) 0x10000100;
	int PS2_data, RVALID;
	char byte1 = 0, byte2 = 0, byte3 = 0;
  while (1)
  { 
    PS2_data = *(PS2_ptr);
    RVALID = PS2_data & 0x8000;
    if (RVALID){
    	byte1 = byte2;
    	byte2 = byte3;
    	byte3 = PS2_data & 0xFF;
    	printf ("byte1: %x, byte2: %x, byte3: %x", byte1, byte2, byte3);
    }
    OSTimeDlyHMSM(0, 0, 1, 0);
  }
}



/* The main function creates two task and starts multi-tasking */
int main(void)
{
  
  OSTaskCreateExt(task1,
                  NULL,
                  (void *)&task1_stk[TASK_STACKSIZE-1],
                  TASK1_PRIORITY,
                  TASK1_PRIORITY,
                  task1_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);
  OSStart();
  return 0;
}

