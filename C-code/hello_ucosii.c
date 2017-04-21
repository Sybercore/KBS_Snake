#include <stdio.h>
#include "includes.h"
#include "altera_up_avalon_character_lcd.h"
#include "altera_up_avalon_parallel_port.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"    // "VGA_Subsystem_VGA_Pixel_DMA"
#include "altera_up_avalon_video_dma_controller.h"		// "VGA_Subsystem_Char_Buf_Subsystem_Char_Buf_DMA"
#include "string.h"
#include <os/alt_sem.h>

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK    readKeyboard_stk[2][TASK_STACKSIZE];
OS_STK CreateField_stk[TASK_STACKSIZE];
OS_STK AddsnakePart_stk[TASK_STACKSIZE];
OS_STK CreateSnake_stk[TASK_STACKSIZE];
OS_STK task2_stk[2][TASK_STACKSIZE];

/* Definition of Task Priorities */

#define READ_KEYBOARD_PRIORITY  6
#define CREATEFIELD_PRIORITY    5
#define TASK2_PRIORITY			8
#define CREATESNAKE_PRIORITY	10


#define WHITE				0xFFFF
#define BLUE				0x000F
#define X_MAX_SIZE			273
#define Y_MAX_SIZE			232

ALT_SEM(sem);
ALT_SEM(directions1_sem);
ALT_SEM(directions2_sem);

alt_up_pixel_buffer_dma_dev* vgapixel;				//pixel buffer device
alt_up_video_dma_dev* vgachar;						//char buffer device
alt_up_character_lcd_dev *lcd_dev;

char text_top_row[40] = "-Snake-\0";

typedef struct snakelist *snake;

struct snakelist{
	int id;
	int x;
	int y;
	struct snake1 *next;
};

snake s1;
snake s2;
char  directions1 = 0;
char  directions2 = 0;

void readKeyboard(void* pdata);

void readKeyboard(void* pdata)
{
	snake i = pdata;
	int delay = 0;
	volatile int * PS2_ptr = (int *) 0x10000100;
	int PS2_data, RVALID;
	int byte = 0;
	int key = 0;
  while (1)
  {
    PS2_data = *(PS2_ptr);
    RVALID = PS2_data & 0x8000;
    if (RVALID){
    	byte = PS2_data & 0xFF;
    	printf ("key: %d", byte);
    	if (i->id == 1){
    	switch(byte)
    	{
    		/* Movement keys */
    		case 0x1D:
    	    	ALT_SEM_PEND(directions1_sem, 0);
    	    	directions1 = 'w';
    	    	ALT_SEM_POST(directions1_sem);
    			break;
    		case 0x1C:
    			ALT_SEM_PEND(directions1_sem, 0);
    			directions1 = 'a';
    			ALT_SEM_POST(directions1_sem);
    			break;
    		case 0x1B:
    			ALT_SEM_PEND(directions1_sem, 0);
    			directions1 = 's';
    			ALT_SEM_POST(directions1_sem);
    			break;
    		case 0x23:
    			ALT_SEM_PEND(directions1_sem, 0);
    			directions1 = 'd';
    			ALT_SEM_POST(directions1_sem);
    		}
    	}
    	if (i->id == 2){
    		switch(byte){
    		case 0x6B:
    			// 4
    			ALT_SEM_PEND(directions2_sem, 0);
    			directions2 = 'l';
    			ALT_SEM_POST(directions2_sem);
    			break;
    		case 0x73:
    			// 5
    			ALT_SEM_PEND(directions2_sem, 0);
    			directions2 = 'z';
    			ALT_SEM_POST(directions2_sem);
    			break;
    		case 0x74:
    			// 6
    			ALT_SEM_PEND(directions2_sem, 0);
    			directions2 = 'r';
    			ALT_SEM_POST(directions2_sem);
    			break;
    		case 0x75:
    			// 8
    			ALT_SEM_PEND(directions2_sem, 0);
    			directions2 = 'u';
    			ALT_SEM_POST(directions2_sem);
    		}
    	}

    	if (i->id == 1){
    		delay = 100;
    	} else {
    		delay = 110;
    	}

    }
    OSTimeDlyHMSM(0, 0, 0, delay);
  }
}

void CreateField(void* pdata) {
int x=42, y =0,i = 0, o = 0;

 for(i = 0 ; i < 30; i++){
	 y = y +8;
	alt_up_pixel_buffer_dma_draw_hline(vgapixel,50,X_MAX_SIZE,y,BLUE,BLUE);
 }

 for(i = 0 ; i < 29; i++){
 	 x = x +8;
 	alt_up_pixel_buffer_dma_draw_vline(vgapixel,x,8,Y_MAX_SIZE,BLUE,BLUE);
  }

 //OSTimeDlyHMSM(0,0,2,0);
 OSTaskDel(OS_PRIO_SELF);
}

/*
void MoveSnake(void* pdata)
{
	snake s = pdata;
	alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0xf000,0xf000); // upper left corner

}
*/

snake CreateSnake(int id){

	snake snake1;

	snake1 = (snake)malloc(sizeof(struct snakelist));

	snake1->next = NULL;
	snake1->id = id;
	if (snake1->id == 1)
	{
	 snake1->x = 67;
	 snake1->y = 25;
	 alt_up_pixel_buffer_dma_draw_box(vgapixel,snake1->x,snake1->y,snake1->x+6,snake1->y+6,0xf000,0xf000); // upper left corner
	}
	if(snake1->id == 2){
		snake1->x = 251;
		snake1->y = 209;
		alt_up_pixel_buffer_dma_draw_box(vgapixel,snake1->x,snake1->y,snake1->x+6,snake1->y+6,0x0ff0,0x0ff0); // upper left corner
	}



	printf("snake Created \n");
	return snake1;
}

snake AddsnakePart(snake head, int id, int x, int y)
{
 snake temp,p;
 temp = CreateSnake(1);
 temp->x  = x;
 temp->y  = y;
 temp->id = id;

 if(head == NULL){
	 head = temp;
 } else{
	 p = head;
	 while(p->next != NULL){
		 p = p->next;
	 }
	 p->next = temp;
 }

 return head;

}

void task2(void* pdata){
while(1){
	// upper left corner X0,Y0,X1,Y1 : 51,9,57,15
	// upper right corner X0,Y0,X1,Y1: 267,9,273,15
	// lower left corner X0,Y0,X1,Y1 : 51,225,57,231
	// lower right corner X0,Y0,X1,Y1: 267,225,273,231


	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51,9,57,15,0xf000,0xf000); // upper left corner
	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51+216,9,57+216,15,0xf000,0xf000);// upper right corner
	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51,9+216,57,15+216,0xf000,0xf000); // lower right corner
	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51+216,9+216,57+216,15+216,0xf000,0xf000); // lower left corner

	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,67,25,73,31,0xf000,0xf000); // upper left corner
	 snake s = pdata;
	// printf("SNAKE!!");
	// printf("%d",s->id);

if(s->id == 1){
	 if( directions1 == 'w'){
		ALT_SEM_PEND(directions1_sem, 0);
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0x0000,0x0000); // upper left corner
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y-8,s->x+6,s->y-2,0xf000,0xf000); // upper left corner
		s1->y = s1->y-8;
		ALT_SEM_POST(directions1_sem);
	 }
	 else if(directions1 == 'a'){
		ALT_SEM_PEND(directions1_sem, 0);
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0x0000,0x0000); // upper left corner
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x-8,s->y,s->x-2,s->y+6,0xf000,0xf000); // upper left corner
		s1->x = s1->x-8;
		ALT_SEM_POST(directions1_sem);
	 }
	 else if(directions1 == 's'){
		ALT_SEM_PEND(directions1_sem, 0);
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0x0000,0x0000); // upper left corner
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y+8,s->x+6,s->y+14,0xf000,0xf000); // upper left corner
		s1->y = s1->y+8;
		ALT_SEM_POST(directions1_sem);
	 }
	 else if(directions1 == 'd'){
		ALT_SEM_PEND(directions1_sem, 0);
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0x0000,0x0000); // upper left corner
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x+8,s->y,s->x+14,s->y+6,0xf000,0xf000); // upper left corner
		s1->x = s1->x+8;
		ALT_SEM_POST(directions1_sem);
	 }
}
if (s->id == 2){
	 if( directions2 == 'u'){
		ALT_SEM_PEND(directions2_sem, 0);
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0x0000,0x0000); // upper left corner
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y-8,s->x+6,s->y-2,0x0ff0,0x0ff0); // upper left corner
		s2->y = s2->y-8;
		ALT_SEM_POST(directions2_sem);
	 }
	 else if(directions2 == 'l'){
		ALT_SEM_PEND(directions2_sem, 0);
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0x0000,0x0000); // upper left corner
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x-8,s->y,s->x-2,s->y+6,0x0ff0,0x0ff0); // upper left corner
		s2->x = s2->x-8;
		ALT_SEM_POST(directions2_sem);
	 }
	 else if(directions2 == 'z'){
		ALT_SEM_PEND(directions2_sem, 0);
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0x0000,0x0000); // upper left corner
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y+8,s->x+6,s->y+14,0x0ff0,0x0ff0); // upper left corner
		s2->y = s2->y+8;
		ALT_SEM_POST(directions2_sem);
	 }
	 else if(directions2 == 'r'){
		ALT_SEM_PEND(directions2_sem, 0);
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0x0000,0x0000); // upper left corner
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x+8,s->y,s->x+14,s->y+6,0x0ff0,0x0ff0); // upper left corner
		s2->x = s2->x+8;
		ALT_SEM_POST(directions2_sem);
	 }

}

 	//alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0xf000,0xf000); // upper left corner
	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51+8,9,57+8,47,0xf000,0xf000);
	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51+8,41+8,57+8,47+8,0xf000,0xf000);
	//alt_up_pixel_buffer_dma_draw_box(vgapixel,51,17,57,17+6,0xf000,0xf000);
	 OSTimeDlyHMSM(0,0,0,200);
}
}



/* The main function creates two task and starts multi-tasking */
int main(void)
{
	OSInit();
	int errs1 = ALT_SEM_CREATE(&directions1,1);
	int errs2 = ALT_SEM_CREATE(&directions2,1);
	int err = ALT_SEM_CREATE(&sem, 1);
	if (err != 0)
		printf("Semaphore NOT created\n");

	vgapixel = alt_up_pixel_buffer_dma_open_dev("/dev/VGA_Subsystem_VGA_Pixel_DMA");		//open pixel buffer
	if (vgapixel == NULL) {
		printf("Error: could not open VGA_Pixel_Buffer device\n");
		return -1;
	}
	else
		printf("Opened VGA_Pixel_Buffer device\n");

	alt_up_pixel_buffer_dma_clear_screen(vgapixel, WHITE);							//clear screen

	vgachar = alt_up_video_dma_open_dev("/dev/VGA_Subsystem_Char_Buf_Subsystem_Char_Buf_DMA");				//open char buffer
	if (vgachar == NULL) {
		printf("Error: could not open VGA_Char_Buffer device\n");
		return -1;
	}
	else
		printf("Opened VGA_Char_Buffer device\n");


	/* output text message to the LCD */
	ALT_SEM_PEND(sem, 0);

	lcd_dev = alt_up_character_lcd_open_dev("/dev/Char_LCD_16x2");
	if (lcd_dev == NULL) {
		printf("Error: could not open character LCD device\n");
		return -1;
	} else
		printf("Opened character LCD device\n");
	ALT_SEM_POST(sem);


	alt_up_character_lcd_set_cursor_pos(lcd_dev, 0, 0); // set LCD cursor location to top row
	alt_up_character_lcd_string(lcd_dev, text_top_row);
	alt_up_character_lcd_cursor_off(lcd_dev); // turn off the LCD cursor

	s1 = CreateSnake(1);
	s2 = CreateSnake(2);

	OSTaskCreateExt(CreateField,
			NULL,
			(void *) &CreateField_stk[TASK_STACKSIZE - 1],
			CREATEFIELD_PRIORITY,
			CREATEFIELD_PRIORITY,
			CreateField_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(task2,
			s1,
			(void *) &task2_stk[0][TASK_STACKSIZE - 1],
			TASK2_PRIORITY, TASK2_PRIORITY,
			task2_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(task2,
			s2,
			(void *) &task2_stk[1][TASK_STACKSIZE - 1],
			TASK2_PRIORITY+1, TASK2_PRIORITY+1,
			task2_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	  OSTaskCreateExt(readKeyboard,
	        s1,
	        (void *)&readKeyboard_stk[1][TASK_STACKSIZE-1],
	        READ_KEYBOARD_PRIORITY+1,
	        READ_KEYBOARD_PRIORITY+1,
	        readKeyboard_stk,
	        TASK_STACKSIZE,
	        NULL,
	        0);

	  OSTaskCreateExt(readKeyboard,
	        s2,
	        (void *)&readKeyboard_stk[0][TASK_STACKSIZE-1],
	        READ_KEYBOARD_PRIORITY,
	        READ_KEYBOARD_PRIORITY,
	        readKeyboard_stk,
	        TASK_STACKSIZE,
	        NULL,
	        0);
	printf("end of main \n");

	OSStart();
	return 0;
}

