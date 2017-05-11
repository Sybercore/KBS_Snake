#include <stdio.h>
#include <stdlib.h>
#include "includes.h"
#include "altera_up_avalon_character_lcd.h"
#include "altera_up_avalon_parallel_port.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"    // "VGA_Subsystem_VGA_Pixel_DMA"
#include "altera_up_avalon_video_dma_controller.h"		// "VGA_Subsystem_Char_Buf_Subsystem_Char_Buf_DMA"
#include "string.h"
#include <os/alt_sem.h>

/* Definition of Task Stacks */
#define   TASK_STACKSIZE       2048
OS_STK ReadKeyboard_stk[2][TASK_STACKSIZE];
OS_STK CreateField_stk[TASK_STACKSIZE];
OS_STK AddsnakePart_stk[TASK_STACKSIZE];
OS_STK MoveSnake_stk[2][TASK_STACKSIZE];
OS_STK BorderCheck_stk[2][TASK_STACKSIZE];
OS_STK GameOver_stk[TASK_STACKSIZE];
OS_STK MainMenu_stk[TASK_STACKSIZE];
OS_STK GenerateApple_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define READ_KEYBOARD_PRIORITY  4
#define CREATE_FIELD_PRIORITY  	2
#define MOVE_SNAKE_PRIORITY		8
#define BORDER_CHECK_PRIORITY	6
#define GAME_OVER_PRIORITY		3
#define MAIN_MENU_PRIORITY		1
#define GENERATE_APPLE_PRIORITY  10

/* Other difine's */
#define WHITE				0xFFFF
#define BLUE				0x000F
#define collor				0xfeee
#define X_MAX_SIZE			273
#define Y_MAX_SIZE			232

/* Creating semaphore */
ALT_SEM(sem);
ALT_SEM(directions1_sem);
ALT_SEM(directions2_sem);
ALT_SEM(Snake_Length);
ALT_SEM(Apple_Loc);
ALT_SEM(Snake_Loc);

/* Screen variables */
alt_up_pixel_buffer_dma_dev* vgapixel;				//pixel buffer device
alt_up_video_dma_dev* vgachar;						//char buffer device
alt_up_character_lcd_dev *lcd_dev;
char text_top_row[40] = "-Snake-\0";

/* Snake struct */
typedef struct snakeparts *snake;
typedef struct applelocation *apple;
typedef struct locations *location;

struct locations{
	int x;
	int y;
};
struct snakeparts{
	int id;
	location loc[290];
	int length;
};

struct applelocation{
	int x;
	int y;
};

/* Variables */
snake s1;
snake s2;
apple a;
snake firsts1, firsts2,lasts1,lasts2;
int sizes1, sizes2;
char  directions1 = 0;
char  directions2 = 0;
char Eaten = 0;

/* Prototypes */
void ReadKeyboard(void* id);
void CreateField (void* the_snake);
void MoveSnake(void* the_snake);
void BorderCheck(void* the_snake);
void GameOver(void* pdata);
void MainMenu(void* pdata);
void GenerateApple(void* pdata);
snake CreateSnake(int id);
void AddSnake(snake head);
//snake AddsnakePart(snake head, int id, int x, int y);


/************************************************************************/
/*
	Prototype:		void ReadKeyboard(void* id)
	Include:		N/A
	Parameters:		id - The id of the snake.
	Returns:		Nothing
	Description:	Reads the keyboard input and sets the corresponding
					semaphore to the right direction.
*/
/************************************************************************/

void ReadKeyboard(void* id){
	/* Declare variables */
	snake Snake_id = id;
	int delay = 0;
	volatile int * PS2_ptr = (int *) 0x10000100;
	int PS2_data, RVALID;
	int byte = 0;

	/* Start checking for input */
	while (1){
		/* Initializing keyboard */
		PS2_data = *(PS2_ptr);
		RVALID = PS2_data & 0x8000;

		/* Check if input is available */
		if (RVALID){
			/* Decoding byte */
			byte = PS2_data & 0xFF;
			//printf ("key: %d", byte);

			if (Snake_id->id == 1){
				/* Movement keys snake 1 */
				switch (byte){
					// Up
					case 0x1D:
						ALT_SEM_PEND(directions1_sem, 0);
						directions1 = 'w';
						ALT_SEM_POST(directions1_sem);
						break;
					// Left
					case 0x1C:
						ALT_SEM_PEND(directions1_sem, 0);
						directions1 = 'a';
						ALT_SEM_POST(directions1_sem);
						break;
					// Down
					case 0x1B:
						ALT_SEM_PEND(directions1_sem, 0);
						directions1 = 's';
						ALT_SEM_POST(directions1_sem);
						break;
					// Right
					case 0x23:
						ALT_SEM_PEND(directions1_sem, 0);
						directions1 = 'd';
						ALT_SEM_POST(directions1_sem);
					}
				}

				if (Snake_id->id == 2){
					/* Movement keys snake 2 */
					switch (byte){
						// Up
						case 0x75:
							// 8
							ALT_SEM_PEND(directions2_sem, 0);
							directions2 = 'u';
							ALT_SEM_POST(directions2_sem);
							break;
						// Left
						case 0x6B:
							// 4
							ALT_SEM_PEND(directions2_sem, 0);
							directions2 = 'l';
							ALT_SEM_POST(directions2_sem);
							break;
						// Down
						case 0x73:
							// 5
							ALT_SEM_PEND(directions2_sem, 0);
							directions2 = 'z';
							ALT_SEM_POST(directions2_sem);
							break;
						// Right
						case 0x74:
							// 6
							ALT_SEM_PEND(directions2_sem, 0);
							directions2 = 'r';
							ALT_SEM_POST(directions2_sem);
					}
				}
			if (Snake_id->id == 1){
				delay = 150;
			} else {
				delay = 160;
			}
		}
    OSTimeDlyHMSM(0, 0, 0, delay);
	}
}


/************************************************************************/
/*
	Prototype:		void CreateField(void* pdata)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
	Parameters:		N/A
	Returns:		Nothing
	Description:	Creates the play field and then deletes its self.
*/
/************************************************************************/

void CreateField(void* the_snake){
	int x=42, y =0,i = 0;

	for(i = 0 ; i < 30; i++){
		y = y +8;
		alt_up_pixel_buffer_dma_draw_hline(vgapixel,50,X_MAX_SIZE,y,BLUE,BLUE);
	}

	for(i = 0 ; i < 29; i++){
		x = x +8;
		alt_up_pixel_buffer_dma_draw_vline(vgapixel,x,8,Y_MAX_SIZE,BLUE,BLUE);
	}

	snake s = the_snake;

	if (s->id == 1){
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[0]->x,s->loc[0]->y,s->loc[0]->x+6,s->loc[0]->y+6,collor,collor);
	} else if (s->id == 2){
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[0]->x,s->loc[0]->y,s->loc[0]->x+6,s->loc[0]->y+6,0x0ff0,0x0ff0);
	}
	OSTaskDel(OS_PRIO_SELF);
}


/************************************************************************/
/*
	Prototype:		void BorderCheck(void* the_snake)
	Include:		N/A
	Parameters:		the_snake - The snake that needs to be checked.
	Returns:		Nothing
	Description:	Checks if the snake is with in the border.
*/
/************************************************************************/

void BorderCheck(void* the_snake){
	/* Declare variables */
	snake s = the_snake;

	/* Starts checking if the snake is outside the border */
	while (1){
		if (s->id == 1){
			if (s->loc[0]->x < 51 || s->loc[0]->x > 267){
				ALT_SEM_PEND(directions1_sem, 0);
				directions1 = 'K';
				ALT_SEM_POST(directions1_sem);
			} else if (s->loc[0]->y < 9 || s->loc[0]->y > 225){
				ALT_SEM_PEND(directions1_sem, 0);
				directions1 = 'K';
				ALT_SEM_POST(directions1_sem);
			}
		}
		if (s->id == 2){
			if (s->loc[0]->x < 51 || s->loc[0]->x > 267){
				ALT_SEM_PEND(directions2_sem, 0);
				directions2 = 'K';
				ALT_SEM_POST(directions2_sem);
			} else if (s->loc[0]->y < 9 || s->loc[0]->y > 225){
				ALT_SEM_PEND(directions2_sem, 0);
				directions2 = 'K';
				ALT_SEM_POST(directions2_sem);
			}
		}
		OSTimeDlyHMSM(0,0,0,180);
	}
}


/************************************************************************/
/*
	Prototype:		snake CreateSnake(int id)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
	Parameters:		id - The id the snake get's.
	Returns:		var_snake on success.
	Description:	Creates a snake and draws it on the screen.
*/
/************************************************************************/

snake CreateSnake(int id){

	/* Declare variables */
	snake new_snake;


	/* malloc space for the snake */
	new_snake = (snake)malloc(sizeof(struct snakeparts));

	new_snake->length = 0;
	/* Set variables of the snake */
	new_snake->id = id;
	if (new_snake->id == 1)
	{
		new_snake->loc[0]->x = 67;
		new_snake->loc[0]->y = 25;
	}
	if (new_snake->id == 2){
		new_snake->loc[0]->x = 251;
		new_snake->loc[0]->y = 209;
	}
	sizes1,sizes2 = 0;
	printf("Snake %d created \n",id);
	return new_snake;
}

void AddSnake(snake head){
	int i;
	location temp;
	if(head->id == 1){
		temp = s1->loc;
		for(i = 0; i < s1->length;i++){
			s1->loc[i+1] = s1->loc[i];
			printf("%d %d\n",s1->loc[i]->x,s1->loc[i]->y);
		}
		//s1->loc[0] = temp;
	}

	if(head->id == 2){
		temp = s2->loc;
			for(i = 0; i < s2->length;i++){
				s2->loc[i+1] = s2->loc[i];
				printf("%d %d\n",s2->loc[i]->x,s2->loc[i]->y);
			}
			//s2->loc[0] = temp;
		}



}

/*
/************************************************************************/
/*
	Prototype:
	Include:
	Parameters:
	Returns:
	Description:
*/
/************************************************************************

snake AddsnakePart(snake head, int id, int x, int y){
	snake temp,p;
	temp = CreateSnake(1);
	temp->x  = x;
	temp->y  = y;
	temp->id = id;

	if(head == NULL){
		head = temp;
	} else {
		p = head;

	while (p->next != NULL){
		p = p->next;
	}
	p->next = temp;
	}

	return head;
}

*/


/************************************************************************/
/*
	Prototype:		void MoveSnake(void* the_snake)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
	Parameters:		the_snake - The snake that needs the move.
	Returns:		Nothing
	Description:	Move's the snake in the corresponding direction.
*/
/************************************************************************/

void MoveSnake(void* the_snake){

	/* Declare variables */
	snake s = the_snake;
	int i;
/*
	if (s->id == 1){
	   //for(int i = 0; i < 3; i++){
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,collor,collor);
		s->x = s->x+8;
	  //}
	} else {
		//for(int i = 0; i < 3; i++){
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s->x,s->y,s->x+6,s->y+6,0x0ff0,0x0ff0);
		s->x = s->x+8;
		//}
	}
*/
	while(1){
	// upper left corner X0,Y0,X1,Y1 : 51,9,57,15
	// upper right corner X0,Y0,X1,Y1: 267,9,273,15
	// lower left corner X0,Y0,X1,Y1 : 51,225,57,231
	// lower right corner X0,Y0,X1,Y1: 267,225,273,231

	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51,9,57,15,collor,collor); // upper left corner
	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51+216,9,57+216,15,collor,collor);// upper right corner
	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51,9+216,57,15+216,collor,collor); // lower right corner
	 //alt_up_pixel_buffer_dma_draw_box(vgapixel,51+216,9+216,57+216,15+216,collor,collor); // lower left corner

	// printf("%d",s->id);
		if(s->id == 1){
			ALT_SEM_PEND(Apple_Loc, 0);
		if(s->loc[0]->x == a->x && s->loc[0]->y == a->y){
			ALT_SEM_PEND(Snake_Length, 0);
							AddSnake(s1);
							s1->length++;
							printf("Apple Eaten snake1 = %d\n", s1->length);
							OSTaskCreateExt(GenerateApple,
							  	        NULL,
							  	        (void *)&GenerateApple_stk[TASK_STACKSIZE-1],
							  	        GENERATE_APPLE_PRIORITY,
							  	        GENERATE_APPLE_PRIORITY,
							  	        GenerateApple_stk,
							  	        TASK_STACKSIZE,
							  	        NULL,
							  	        0);

			}
			ALT_SEM_POST(Snake_Length);
		}
		ALT_SEM_POST(Apple_Loc);
		if(s->id == 2){
		ALT_SEM_PEND(Apple_Loc, 0);
		if(s->loc[0]->x == a->x && s->loc[0]->y == a->y){
			ALT_SEM_PEND(Snake_Length, 0);
							AddSnake(s2);
							s2->length++;
							printf("Apple Eaten snake2 = %d\n", s2->length);
							OSTaskCreateExt(GenerateApple,
									  	        NULL,
									  	        (void *)&GenerateApple_stk[TASK_STACKSIZE-1],
									  	        GENERATE_APPLE_PRIORITY,
									  	        GENERATE_APPLE_PRIORITY,
									  	        GenerateApple_stk,
									  	        TASK_STACKSIZE,
									  	        NULL,
									  	        0);
						}
		ALT_SEM_POST(Snake_Length);
		}
		ALT_SEM_POST(Apple_Loc);
		if (s->id == 1){
			ALT_SEM_PEND(directions1_sem, 0);
			// Move's up
			if (directions1 == 'w'){
				ALT_SEM_PEND(Snake_Length,0);
				for(i = 0; i <= s->length; i++){
				ALT_SEM_PEND(Snake_Loc, 0);

				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y,s->loc[i]->x+6,s->loc[i]->y+6,0x0000,0x0000);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y-8,s->loc[i]->x+6,s->loc[i]->y-2,collor,collor);

				s1->loc[i]->y = s1->loc[i]->y-8;
				ALT_SEM_POST(Snake_Loc);
				}
				ALT_SEM_POST(Snake_Length);

			}
			// Move's left
			else if (directions1 == 'a'){
				ALT_SEM_PEND(Snake_Length,0);
				for(i = 0; i <= s->length; i++){
				ALT_SEM_PEND(Snake_Loc, 0);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y,s->loc[i]->x+6,s->loc[i]->y+6,0x0000,0x0000);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x-8,s->loc[i]->y,s->loc[i]->x-2,s->loc[i]->y+6,collor,collor);
				s1->loc[i]->x = s1->loc[i]->x-8;
				ALT_SEM_POST(Snake_Loc);
				}
				ALT_SEM_POST(Snake_Length);
				/*
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y,s->loc[i]->x+6,s->loc[i]->y+6,0x0000,0x0000);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x-8,s->loc[i]->y,s->loc[i]->x-2,s->loc[i]->y+6,0x0ff0,0x0ff0);
				s2->loc[i]->x = s2->loc[i]->x-8;
				*/
			}
			// Move's down
			else if (directions1 == 's'){
				ALT_SEM_PEND(Snake_Length,0);
				for(i = 0; i <= s->length; i++){
				ALT_SEM_PEND(Snake_Loc, 0);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y,s->loc[i]->x+6,s->loc[i]->y+6,0x0000,0x0000);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y+8,s->loc[i]->x+6,s->loc[i]->y+14,collor,collor);
				s1->loc[i]->y = s1->loc[i]->y+8;
				ALT_SEM_POST(Snake_Loc);
				}
				ALT_SEM_POST(Snake_Length);
			}
			// Move's right
			else if (directions1 == 'd'){
				ALT_SEM_PEND(Snake_Length,0);
				for(i = 0; i <= s->length; i++){
				ALT_SEM_PEND(Snake_Loc, 0);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y,s->loc[i]->x+6,s->loc[i]->y+6,0x0000,0x0000);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x+8,s->loc[i]->y,s->loc[i]->x+14,s->loc[i]->y+6,collor,collor);
				s1->loc[i]->x = s1->loc[i]->x+8;
				ALT_SEM_POST(Snake_Loc);
				}
				ALT_SEM_POST(Snake_Length);
			}
			ALT_SEM_POST(directions1_sem);
		}
		if (s->id == 2){
			ALT_SEM_PEND(directions2_sem, 0);
			// Move's up
			if (directions2 == 'u'){
				ALT_SEM_PEND(Snake_Length,0);
				for(i = 0; i <= s->length; i++){
				ALT_SEM_PEND(Snake_Loc, 0);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y,s->loc[i]->x+6,s->loc[i]->y+6,0x0000,0x0000);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y-8,s->loc[i]->x+6,s->loc[i]->y-2,0x0ff0,0x0ff0);
				s2->loc[i]->y = s2->loc[i]->y-8;
				ALT_SEM_POST(Snake_Loc);
				}
				ALT_SEM_POST(Snake_Length);
			}
			// Move's left
			else if (directions2 == 'l'){
				ALT_SEM_PEND(Snake_Length,0);
				for(i = 0; i <= s->length; i++){
				ALT_SEM_PEND(Snake_Loc, 0);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y,s->loc[i]->x+6,s->loc[i]->y+6,0x0000,0x0000);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x-8,s->loc[i]->y,s->loc[i]->x-2,s->loc[i]->y+6,0x0ff0,0x0ff0);
				s2->loc[i]->x = s2->loc[i]->x-8;
				ALT_SEM_POST(Snake_Loc);
				}
				ALT_SEM_POST(Snake_Length);
			}
			// Move's down
			else if (directions2 == 'z'){
				ALT_SEM_PEND(Snake_Length,0);
				for(i = 0; i <= s->length; i++){
				ALT_SEM_PEND(Snake_Loc, 0);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y,s->loc[i]->x+6,s->loc[i]->y+6,0x0000,0x0000);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y+8,s->loc[i]->x+6,s->loc[i]->y+14,0x0ff0,0x0ff0);
				s2->loc[i]->y = s2->loc[i]->y+8;
				ALT_SEM_POST(Snake_Loc);
				}
				ALT_SEM_POST(Snake_Length);
			}
			// Move's right
			else if (directions2 == 'r'){

				ALT_SEM_PEND(Snake_Length,0);
				for(i = 0; i <= s->length; i++){
				ALT_SEM_PEND(Snake_Loc, 0);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x,s->loc[i]->y,s->loc[i]->x+6,s->loc[i]->y+6,0x0000,0x0000);
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s->loc[i]->x+8,s->loc[i]->y,s->loc[i]->x+14,s->loc[i]->y+6,0x0ff0,0x0ff0);
				s2->loc[i]->x = s2->loc[i]->x+8;
				ALT_SEM_POST(Snake_Loc);
				}
				ALT_SEM_POST(Snake_Length);
			}
			ALT_SEM_POST(directions2_sem);






		ALT_SEM_PEND(directions1_sem, 0);
		ALT_SEM_PEND(directions2_sem, 0);
		if (directions1 == 'K' || directions2 == 'K'){
		  OSTaskCreateExt(GameOver,
				NULL,
				(void *) &GameOver_stk[TASK_STACKSIZE - 1],
				GAME_OVER_PRIORITY,GAME_OVER_PRIORITY,
				GameOver_stk,
				TASK_STACKSIZE,
				NULL,
				0);
		}
		ALT_SEM_POST(directions1_sem);
		ALT_SEM_POST(directions2_sem);

		}
	OSTimeDlyHMSM(0,0,0,120);
	}
}


/************************************************************************/
/*
	Prototype:
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
					<altera_up_avalon_video_dma_controller.h>
	Parameters:
	Returns:		Nothing
	Description:
*/
/************************************************************************/

void GameOver(void* pdata){
	//alt_up_pixel_buffer_dma_draw_box(vgapixel,1,1,40,40,0x0000,0x0000);
	//alt_up_pixel_buffer_dma_draw_box(vgapixel,120,140,140,160,0x0000,0x0000);
	char str[20] = "GAME OVER";
		alt_up_pixel_buffer_dma_clear_screen(vgapixel, 0);
		alt_up_video_dma_draw_string(vgachar, str, 35,30, 0);

		if(directions2 == 'K'){
			printf("snake2 died\n");
		}else
			printf("Snake1 died\n");

		OSTimeDlyHMSM(0,0,2,0);
		// show score
		// press enter??
		// highscore??
		// to do go to main menu and delete other tasks.
}


void MainMenu(void* pdata){
	char str1[20] = "SNAKE!";
	char str2[20] = "Play";
	char str3[20] = "Multiplier";
	char str4[20] = "Instructions";
	alt_up_pixel_buffer_dma_clear_screen(vgapixel, 0);
	alt_up_pixel_buffer_dma_draw_box(vgapixel,145,76,174,86,0x6e00,0x6e00);
	while (1){
		alt_up_video_dma_draw_string(vgachar, str1, 37,20, 0);
		alt_up_video_dma_draw_string(vgachar, str2, 35,30, 0);
		alt_up_video_dma_draw_string(vgachar, str3, 35,35, 0);
		alt_up_video_dma_draw_string(vgachar, str4, 35,40, 0);

		OSTimeDlyHMSM(0,0,2,0);

	}
}


void GenerateApple(void* pdata){
	ALT_SEM_PEND(Apple_Loc, 0);
	srand(time());
	int yLocation = rand() % 28;
	srand(time());
	int xLocation = rand()% 28;

// if apple is eaten
	a->x =  51+(xLocation*8);
	a->y =  9+(yLocation*8);
	alt_up_pixel_buffer_dma_draw_box(vgapixel,51+(xLocation*8),9+(yLocation*8),57+(xLocation*8),15+(yLocation*8),0xf000,0xf000);
// else
	//do nothing;
	printf("Appel created\n");
	ALT_SEM_POST(Apple_Loc);
	OSTaskDel(OS_PRIO_SELF);

}


/* The main function creates tasks and starts multi-tasking */
int main(void){
	OSInit();
	int errs;
	errs = ALT_SEM_CREATE(&directions1,1);
	errs = ALT_SEM_CREATE(&directions2,1);
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
		alt_up_video_dma_draw_string(vgachar, "", 1, 1, 1);
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
			CREATE_FIELD_PRIORITY,CREATE_FIELD_PRIORITY,
			CreateField_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(BorderCheck,
			s1,
			(void *) &BorderCheck_stk[0][TASK_STACKSIZE - 1],
			BORDER_CHECK_PRIORITY,BORDER_CHECK_PRIORITY,
			BorderCheck_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(BorderCheck,
			s2,
			(void *) &BorderCheck_stk[1][TASK_STACKSIZE - 1],
			BORDER_CHECK_PRIORITY+1,BORDER_CHECK_PRIORITY+1,
			BorderCheck_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(MoveSnake,
			s1,
			(void *) &MoveSnake_stk[0][TASK_STACKSIZE - 1],
			MOVE_SNAKE_PRIORITY, MOVE_SNAKE_PRIORITY,
			MoveSnake_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(MoveSnake,
			s2,
			(void *) &MoveSnake_stk[1][TASK_STACKSIZE - 1],
			MOVE_SNAKE_PRIORITY+1, MOVE_SNAKE_PRIORITY+1,
			MoveSnake_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	  OSTaskCreateExt(ReadKeyboard,
	        s1,
	        (void *)&ReadKeyboard_stk[1][TASK_STACKSIZE-1],
	        READ_KEYBOARD_PRIORITY+1,
	        READ_KEYBOARD_PRIORITY+1,
	        ReadKeyboard_stk,
	        TASK_STACKSIZE,
	        NULL,
	        0);

	  OSTaskCreateExt(ReadKeyboard,
	        s2,
	        (void *)&ReadKeyboard_stk[0][TASK_STACKSIZE-1],
	        READ_KEYBOARD_PRIORITY,
	        READ_KEYBOARD_PRIORITY,
	        ReadKeyboard_stk,
	        TASK_STACKSIZE,
	        NULL,
	        0);

	  OSTaskCreateExt(GenerateApple,
	  			   	  	  	NULL,
	  			   	        (void *)&GenerateApple_stk[TASK_STACKSIZE-1],
	  			  	  	  	GENERATE_APPLE_PRIORITY,
	  			  	  	  	GENERATE_APPLE_PRIORITY,
	  			  	  	  	GenerateApple_stk,
	  			  	  	  	TASK_STACKSIZE,
	  			  	  	  	NULL,
	  			  	  	  	0);

	/*
	  OSTaskCreateExt(MainMenu,
	        NULL,
	        (void *)&MainMenu_stk[TASK_STACKSIZE-1],
	        MAIN_MENU_PRIORITY,
	        MAIN_MENU_PRIORITY,
	        MainMenu_stk,
	        TASK_STACKSIZE,
	        NULL,
	        0);
	*/

	printf("end of main \n");

	OSStart();
	return 0;
}
