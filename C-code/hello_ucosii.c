#include <stdio.h>
#include "includes.h"

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    read_keyboard_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define READ_KEYBOARD_PRIORITY      1

void read_keyboard(void* pdata);

void read_keyboard(void* pdata)
{
	volatile int * PS2_ptr = (int *) 0x10000100;
	int PS2_data, RVALID;
	int byte = 0;
	char key;
  while (1)
  {
    PS2_data = *(PS2_ptr);
    RVALID = PS2_data & 0x8000;
    if (RVALID){
    	byte = PS2_data & 0xFF;
    	printf ("key: %d", byte);
    	switch(byte)
    	{
    		/* LETTERS */
    		case 0x1D:
    			key = 'w';
    			break;
    		case 0x1C:
    			key = 'a';
    			break;
    		case 0x1B:
    			key = 's';
    			break;
    		case 0x23:
    			key = 'd';
    			break;
    		case 0x6B:
    			// 4
    			key = 'l';
    			break;
    		case 0x73:
    			// 5
    			key = 'z';
    			break;
    		case 0x74:
    			// 6
    			key = 'r';
    			break;
    		case 0x75:
    			// 8
    			key = 'u';
    	}

    	printf ("letter: %c\n", key);

    }
    OSTimeDlyHMSM(0, 0, 0, 10);
  }
}



/* The main function creates two task and starts multi-tasking */
int main(void)
{

  OSTaskCreateExt(read_keyboard,
                  NULL,
                  (void *)&read_keyboard_stk[TASK_STACKSIZE-1],
                  READ_KEYBOARD_PRIORITY,
                  READ_KEYBOARD_PRIORITY,
                  read_keyboard_stk,
                  TASK_STACKSIZE,
                  NULL,
                  0);
  OSStart();
  return 0;
}

