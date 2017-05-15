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
OS_STK ReadKeyboard_stk[TASK_STACKSIZE];
OS_STK CreateField_stk[TASK_STACKSIZE];
OS_STK AddsnakePart_stk[TASK_STACKSIZE];
OS_STK MoveSnake_stk[2][TASK_STACKSIZE];
OS_STK BorderCheck_stk[2][TASK_STACKSIZE];
OS_STK GameOver_stk[TASK_STACKSIZE];
OS_STK MainMenu_stk[TASK_STACKSIZE];
OS_STK GenerateApple_stk[TASK_STACKSIZE];
OS_STK ReadKeyboardMenu_stk[TASK_STACKSIZE];
OS_STK StartSinglePlayer_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define READ_KEYBOARD_PRIORITY  4
#define CREATE_FIELD_PRIORITY  	2
#define MOVE_SNAKE_PRIORITY		7
#define BORDER_CHECK_PRIORITY	5
#define GAME_OVER_PRIORITY		3
#define MAIN_MENU_PRIORITY		11
#define GENERATE_APPLE_PRIORITY  9
#define READ_KEYBOARD_MENU_PRIORITY  10
#define START_SINGLE_PLAYER_PRIORITY 1

/* Other difine's */
#define MainGreen 			0x6e00
#define BLACK 				0x0000
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
ALT_SEM(menu_sem);

/* Screen variables */
alt_up_pixel_buffer_dma_dev* vgapixel;				//pixel buffer device
alt_up_video_dma_dev* vgachar;						//char buffer device
alt_up_character_lcd_dev *lcd_dev;
volatile int * PS2_ptr = (int *) 0x10000100;
char text_top_row[40] = "-Snake-\0";

/*  Struct's  */
typedef struct snakeparts snake;
typedef struct applelocation apple;
typedef struct locations location;

typedef struct locations{
	int x;
	int y;
} locations;

typedef struct snakeparts{
	int id;
	location loc[290];
	int length;
} snakeparts;

typedef struct applelocation{
	int x;
	int y;
} applelocation;

/* Variables */
snake s1;
snake s2;
apple a;
char  directions1 = 0;
char  directions2 = 0;
char Eaten = 0;
char menu_id = 0;

/* Prototypes */
void ReadKeyboard(void* pdata);
void CreateField (void* the_snake);
void MoveSnake(void* the_snake);
void BorderCheck(void* the_snake);
void GameOver(void* pdata);
void MainMenu(void* pdata);
void GenerateApple(void* pdata);
snake CreateSnake(int id);
void AddSnake(snake head);
void ReadKeyboardMenu(void* pdata);
void StartSinglePlayer(void* pdata);


/************************************************************************/
/*
	Prototype:		void ReadKeyboard(void* pdata)
	Include:		N/A
	Parameters:		N/A
	Returns:		Nothing
	Description:	Reads the keyboard input and sets the corresponding
					semaphore to the right direction.
*/
/************************************************************************/

void ReadKeyboard(void* pdata){
	/* Declare variables */
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
						break;
					/* Movement keys snake 2 */
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
    OSTimeDlyHMSM(0, 0, 0, 120);
	}
}


/************************************************************************/
/*
	Prototype:		void ReadKeyboardMenu(void* pdata)
	Include:		N/A
	Parameters:		N/A
	Returns:		Nothing
	Description:	Reads the keyboard input, makes it possible to start
					a game and to go back to the main menu.
*/
/************************************************************************/

void ReadKeyboardMenu(void* pdata){
	/* Declare variables */
	int PS2_data, RVALID;
	int byte = 0, loc = 1, count = 0;

	/* Start checking for input */
	while (1){
		/* Initializing keyboard */
		PS2_data = *(PS2_ptr);
		RVALID = PS2_data & 0x8000;

		/* Check if input is available */
		if (RVALID){
			/* Decoding byte */
			byte = PS2_data & 0xFF;

			if (menu_id == 0){
				/* Main screen keys */
				switch (byte){
					// Next
					case 0x29:
						// Space
						if (count >= 2){
							if (loc == 0){
								// Bottom word
								alt_up_pixel_buffer_dma_draw_box(vgapixel,130,139,135,144,BLACK,BLACK);
								alt_up_pixel_buffer_dma_draw_box(vgapixel,130,119,135,124,MainGreen,MainGreen);
								loc = loc + 1;
							} else if (loc == 1){
								// Top word
								alt_up_pixel_buffer_dma_draw_box(vgapixel,130,119,135,124,BLACK,BLACK);
								alt_up_pixel_buffer_dma_draw_box(vgapixel,130,139,135,144,MainGreen,MainGreen);
								loc = loc - 1;
							}
							count = 0;
						}
						break;
					// Go
					case 0x5A:
						// Enter
						if (count >= 2){
							if (loc == 1){
								// Start single player
								ALT_SEM_PEND(menu_sem, 0);
								menu_id = 1; // Create Game
								ALT_SEM_POST(menu_sem);

								OSTaskCreateExt(StartSinglePlayer,
										NULL,
										(void *) &StartSinglePlayer_stk[TASK_STACKSIZE - 1],
										START_SINGLE_PLAYER_PRIORITY, START_SINGLE_PLAYER_PRIORITY,
										StartSinglePlayer_stk,
										TASK_STACKSIZE,
										NULL,
										0);

								OSTaskDel(OS_PRIO_SELF); // not longer needed

							} else if (loc == 0){
								// Start multiplier
								ALT_SEM_PEND(menu_sem, 0);
								menu_id = 2; // Create Game
								ALT_SEM_POST(menu_sem);
								// TO DO
								// Task StartMultiplier starten
							}
							count = 0;
						}
					}
				} else if (menu_id == 3){
					switch(byte){
						case 0x5A:
						// Enter
						if (count >= 2){
							  OSTaskCreateExt(MainMenu,
							        NULL,
							        (void *)&MainMenu_stk[TASK_STACKSIZE-1],
							        MAIN_MENU_PRIORITY,
							        MAIN_MENU_PRIORITY,
							        MainMenu_stk,
							        TASK_STACKSIZE,
							        NULL,
							        0);

							  ALT_SEM_PEND(menu_sem, 0);
							  menu_id = 0; // Go back to the main menu
							  ALT_SEM_POST(menu_sem);
							  count = 0;
						}
					}
				}
				count++;
			}
		OSTimeDlyHMSM(0, 0, 0, 200);
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

	/* Horizontal line's */
	for(i = 0 ; i < 30; i++){
		y = y +8;
		alt_up_pixel_buffer_dma_draw_hline(vgapixel,50,X_MAX_SIZE,y,BLUE,BLUE);
	}

	/* Vertical line's */
	for(i = 0 ; i < 29; i++){
		x = x +8;
		alt_up_pixel_buffer_dma_draw_vline(vgapixel,x,8,Y_MAX_SIZE,BLUE,BLUE);
	}

	snake s = *((snake*) the_snake);

	/* Draw snake's */
	if (s.id == 1){
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s.loc[0].x,s.loc[0].y,s.loc[0].x+6,s.loc[0].y+6,collor,collor);
	} else if (s.id == 2){
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s.loc[0].x,s.loc[0].y,s.loc[0].x+6,s.loc[0].y+6,0x0ff0,0x0ff0);
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
	snake s = *((snake*) the_snake);

	/* Starts checking if the snake is outside the border */
	while (1){
		if (s.id == 1){
			ALT_SEM_PEND(Snake_Loc, 0);
			if (s1.loc[0].x < 51 || s1.loc[0].x > 267){
				/* Snake is outside the border */
				ALT_SEM_PEND(directions1_sem, 0);
				directions1 = 'K';
				ALT_SEM_POST(directions1_sem);
			} else if (s1.loc[0].y < 9 || s1.loc[0].y > 225){
				/* Snake is outside the border */
				ALT_SEM_PEND(directions1_sem, 0);
				directions1 = 'K';
				ALT_SEM_POST(directions1_sem);
			}
			ALT_SEM_POST(Snake_Loc);
		}
		if (s.id == 2){
			if (s2.loc[0].x < 51 || s2.loc[0].x > 267){
				/* Snake is outside the border */
				ALT_SEM_PEND(directions2_sem, 0);
				directions2 = 'K';
				ALT_SEM_POST(directions2_sem);
			} else if (s2.loc[0].y < 9 || s2.loc[0].y > 225){
				/* Snake is outside the border */
				ALT_SEM_PEND(directions2_sem, 0);
				directions2 = 'K';
				ALT_SEM_POST(directions2_sem);
			}
		}
		OSTimeDlyHMSM(0,0,0,80);
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

	/* Set variables of the snake */
	new_snake.id = id;
	new_snake.length = 0;

	if (new_snake.id == 1)
	{
		/* Set location snake 1 */
		new_snake.loc[0].x = 67;
		new_snake.loc[0].y = 25;

	}
	if (new_snake.id == 2){
		/* Set location snake 2 */
		new_snake.loc[0].x = 251;
		new_snake.loc[0].y = 209;
	}

	printf("Snake %d created loc: %d %d\n",id, new_snake.loc[0].x, new_snake.loc[0].y);
	return new_snake;
}


/************************************************************************/
/*
	Prototype:		void AddSnake(snake head)
	Include:		N/A
	Parameters:		head - The front of the snake
	Returns:		Nothing
	Description:
*/
/************************************************************************/

void AddSnake(snake head){
	int i;
	location temp;
		temp = s1.loc[0];
		for(i = 0; i < s1.length;i++){
			s1.loc[i+1] = s1.loc[i];
			printf("%d %d\n",s1.loc[i].x,s1.loc[i].y);
		}
		s1.loc[0] = temp;
		//s1->loc[0] = temp;
/*
	if(head.id == 2){
		//temp = s2.loc;
			for(i = 0; i < s2.length;i++){
				s2.loc[i+1] = s2.loc[i];
				printf("%d %d\n",s2.loc[i].x,s2.loc[i].y);
			}
			//s2->loc[0] = temp;
		}

*/

}


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
	snake s = *((snake*) the_snake);
	int i;

	printf("%d", directions1);
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


	if(s1.loc[0].x == a.x && s1.loc[0].y == a.y){
		//AddSnake(s1);
		//s1.length++;
		printf("Apple Eaten snake1 = %d\n", s1.length);
		OSTaskCreateExt(GenerateApple,
				NULL,
				(void *)&GenerateApple_stk[TASK_STACKSIZE-1],
				GENERATE_APPLE_PRIORITY, GENERATE_APPLE_PRIORITY,
				GenerateApple_stk,
				TASK_STACKSIZE,
				NULL,
				0);
			}

/*
		ALT_SEM_PEND(Apple_Loc, 0);
		if(s2.loc[0].x == a.x && s2.loc[0].y == a.y){
			ALT_SEM_PEND(Snake_Length, 0);
							//AddSnake(s2);
							s2.length++;
							printf("Apple Eaten snake2 = %d\n", s2.length);
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
		ALT_SEM_POST(Apple_Loc);
*/

	if (s.id == 1){
		ALT_SEM_PEND(directions1_sem, 0);
		if (directions1 == 'w'){
			 printf("Start locatie : x = %d y = %d\n",s1.loc[i].x,s1.loc[i].y);
			 //for(i=0; i <= s1.length;i++){
			 alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[i].x,s1.loc[i].y,s1.loc[i].x+6,s1.loc[i].y+6,0x0000,0x0000);
			 s1.loc[i].y = s1.loc[i].y-8;
			 alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[i].x,s1.loc[i].y,s1.loc[i].x+6,s1.loc[i].y+6,collor,collor);
			 //}
			 printf("Volgende locatie : x = %d y = %d\n",s1.loc[i].x,s1.loc[i].y);
		}
		else if (directions1 == 'a'){
			printf("Start locatie : x = %d y = %d\n",s1.loc[i].x,s1.loc[i].y);
			//for(i=0; i <= s1.length;i++){
			alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[i].x,s1.loc[i].y,s1.loc[i].x+6,s1.loc[i].y+6,0x0000,0x0000);
			s1.loc[i].x = s1.loc[i].x-8;
			alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[i].x,s1.loc[i].y,s1.loc[i].x+6,s1.loc[i].y+6,collor,collor);
			//}
			printf("Volgende locatie : x = %d y = %d\n",s1.loc[i].x,s1.loc[i].y);
		}
		else if (directions1 == 's'){
			printf("Start locatie : x = %d y = %d\n",s1.loc[i].x,s1.loc[i].y);
			//for(i=0; i <= s1.length;i++){
			alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[i].x,s1.loc[i].y,s1.loc[i].x+6,s1.loc[i].y+6,0x0000,0x0000);
			s1.loc[i].y = s1.loc[i].y+8;
			alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[i].x,s1.loc[i].y,s1.loc[i].x+6,s1.loc[i].y+6,collor,collor);
			//}
			printf("Volgende locatie : x = %d y = %d\n",s1.loc[i].x,s1.loc[i].y);
		}
		else if (directions1 == 'd'){
			printf("Start locatie : x = %d y = %d\n",s1.loc[i].x,s1.loc[i].y);
			//for(i=0; i <= s1.length;i++){
			alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[i].x,s1.loc[i].y,s1.loc[i].x+6,s1.loc[i].y+6,0x0000,0x0000);
			s1.loc[i].x = s1.loc[i].x+8;
			alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[i].x,s1.loc[i].y,s1.loc[i].x+6,s1.loc[i].y+6,collor,collor);
			//}
			printf("Volgende locatie : x = %d y = %d\n",s1.loc[i].x,s1.loc[i].y);
		}
		ALT_SEM_POST(directions1_sem);


		/* Check's if the snake has been outside the border */
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
	Prototype:		void GameOver(void* pdata)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
					<altera_up_avalon_video_dma_controller.h>
	Parameters:		N/A
	Returns:		Nothing
	Description:	Deletes the game tasks, resets value's and shows
					game over screen.
*/
/************************************************************************/

void GameOver(void* pdata){
	ALT_SEM_PEND(menu_sem, 0);
	if (menu_id == 1){
		OSTaskDel(READ_KEYBOARD_PRIORITY);
		OSTaskDel(MOVE_SNAKE_PRIORITY);
		OSTaskDel(BORDER_CHECK_PRIORITY);
	} else if (menu_id == 2){
		OSTaskDel(READ_KEYBOARD_PRIORITY);
		OSTaskDel(MOVE_SNAKE_PRIORITY);
		OSTaskDel(MOVE_SNAKE_PRIORITY+1);
		OSTaskDel(BORDER_CHECK_PRIORITY);
		OSTaskDel(BORDER_CHECK_PRIORITY+1);
	}
	ALT_SEM_POST(menu_sem);

	/* Reset variables */
	directions1 = 0;
	s1 = CreateSnake(1);

	/* Clear screen */
	alt_up_pixel_buffer_dma_clear_screen(vgapixel, BLACK);
	alt_up_video_dma_screen_fill(vgachar, BLACK, 0);
	alt_up_video_dma_screen_fill(vgachar, BLACK, 1);

	/* Print game over */
	char GameOverString[25];
	strcpy(GameOverString, "GAME OVER");
	alt_up_video_dma_draw_string(vgachar, GameOverString, 35,30, 0);

	ALT_SEM_PEND(menu_sem, 0);
	menu_id = 3; // Make keyboard responsive
	ALT_SEM_POST(menu_sem);

	  OSTaskCreateExt(ReadKeyboardMenu,
	        NULL,
	        (void *)&ReadKeyboardMenu_stk[TASK_STACKSIZE-1],
	        READ_KEYBOARD_MENU_PRIORITY,
	        READ_KEYBOARD_MENU_PRIORITY,
	        ReadKeyboardMenu_stk,
	        TASK_STACKSIZE,
	        NULL,
	        0);

	// show score

	if(directions2 == 'K'){
		printf("snake2 died\n");
	}else{
		printf("Snake1 died\n");
	}

	OSTimeDlyHMSM(0,0,2,0);

	/* Draw's press enter to continue */
	strcpy(GameOverString, "Press enter to continue");
	alt_up_video_dma_draw_string(vgachar, GameOverString, 28,32, 0);

	OSTaskDel(OS_PRIO_SELF);
}


/************************************************************************/
/*
	Prototype:		void GenerateApple(void* pdata)
	Include:		N/A
	Parameters:		N/A
	Returns:		Nothing
	Description:	Generates an apple at a random location.
*/
/************************************************************************/

void GenerateApple(void* pdata){
	ALT_SEM_PEND(Apple_Loc, 0);
	/* Randomizer */
	srand(time());
	int yLocation = rand() % 28;
	srand(time());
	int xLocation = rand()% 28;

	/* Set's the location in the struct and draw's the apple */
	a.x =  51+(xLocation*8);
	a.y =  9+(yLocation*8);
	alt_up_pixel_buffer_dma_draw_box(vgapixel,a.x,a.y,a.x + 6,a.y+6,0xf000,0xf000);

	printf("Appel created\n");
	ALT_SEM_POST(Apple_Loc);
	OSTaskDel(OS_PRIO_SELF);
}


/************************************************************************/
/*
	Prototype:		void MainMenu(void* pdata)
	Include:		N/A
	Parameters:		N/A
	Returns:		Nothing
	Description:	Draws the main screen.
*/
/************************************************************************/

void MainMenu(void* pdata){
	/* Creating menu strings */
	char menu_string1[20];
	char menu_string2[20];
	char menu_string3[20];

	/* Fill menu strings */
	strcpy(menu_string1, "SNAKE!");
	strcpy(menu_string2, "Play");
	strcpy(menu_string3, "Multiplier");

	/* Creating player 1 strings */
	char player1_string1[20];
	char player1_string2[20];
	char player1_string3[21];

	/* Fill player 1 strings */
	strcpy(player1_string1, "Player 1 controls:");
	strcpy(player1_string2, "w -> up   a -> left");
	strcpy(player1_string3, "s -> down d -> right");

	/* Creating menu control strings */
	char menu_controls1[20];
	char menu_controls2[20];
	char menu_controls3[20];

	/* Fill menu control strings */
	strcpy(menu_controls1, "Menu controls:");
	strcpy(menu_controls2, "space -> next");
	strcpy(menu_controls3, "enter -> confirm");

	/* Creating player 2 strings */
	char player2_string1[20];
	char player2_string2[20];
	char player2_string3[21];

	/* Fill player 2 strings */
	strcpy(player2_string1, "Player 2 controls:");
	strcpy(player2_string2, "8 -> up   4 -> left");
	strcpy(player2_string3, "5 -> down 6 -> right");

	/* Clear screen */
	alt_up_video_dma_screen_fill(vgachar, BLACK, 0);
	alt_up_video_dma_screen_fill(vgachar, BLACK, 1);

	/* Draw first option */
	alt_up_pixel_buffer_dma_draw_box(vgapixel,145,76,174,86,MainGreen,MainGreen);

	/* Draw back box */
	alt_up_pixel_buffer_dma_draw_box(vgapixel,130,119,135,124,MainGreen,MainGreen);

	/* Draw menu strings */
	alt_up_video_dma_draw_string(vgachar, menu_string1, 37,20, 0);
	alt_up_video_dma_draw_string(vgachar, menu_string2, 35,30, 0);
	alt_up_video_dma_draw_string(vgachar, menu_string3, 35,35, 0);

	/* Draws horizontal line */
	alt_up_pixel_buffer_dma_draw_hline(vgapixel,10,310,210,MainGreen,MainGreen);

	/* Draw player 1 controls */
	alt_up_video_dma_draw_string(vgachar, player1_string1, 3,54, 0);
	alt_up_video_dma_draw_string(vgachar, player1_string2, 3,56, 0);
	alt_up_video_dma_draw_string(vgachar, player1_string3, 3,58, 0);

	/* Draw menu controls */
	alt_up_video_dma_draw_string(vgachar, menu_controls1, 32,54, 0);
	alt_up_video_dma_draw_string(vgachar, menu_controls2, 32,56, 0);
	alt_up_video_dma_draw_string(vgachar, menu_controls3, 32,58, 0);

	/* Draw player 2 controls */
	alt_up_video_dma_draw_string(vgachar, player2_string1, 57,54, 0);
	alt_up_video_dma_draw_string(vgachar, player2_string2, 57,56, 0);
	alt_up_video_dma_draw_string(vgachar, player2_string3, 57,58, 0);

	OSTaskDel(OS_PRIO_SELF);
}


/************************************************************************/
/*
	Prototype:		void StartSinglePlayer(void* pdata)
	Include:		N/A
	Parameters:		N/A
	Returns:		Nothing
	Description:	Start up the task for the game and does the countdown.
*/
/************************************************************************/

void StartSinglePlayer(void* pdata){
	/* Clear screen */
	alt_up_pixel_buffer_dma_clear_screen(vgapixel, BLACK);
	alt_up_video_dma_screen_fill(vgachar, BLACK, 0);
	alt_up_video_dma_screen_fill(vgachar, BLACK, 1);

	s1 = CreateSnake(1);

	/* Create tasks for single player */
	OSTaskCreateExt(CreateField,
			NULL,
			(void *) &CreateField_stk[TASK_STACKSIZE - 1],
			CREATE_FIELD_PRIORITY, CREATE_FIELD_PRIORITY,
			CreateField_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(ReadKeyboard,
			NULL,
			(void *)&ReadKeyboard_stk[TASK_STACKSIZE-1],
			READ_KEYBOARD_PRIORITY, READ_KEYBOARD_PRIORITY,
			ReadKeyboard_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(BorderCheck,
			&s1,
			(void *) &BorderCheck_stk[0][TASK_STACKSIZE - 1],
			BORDER_CHECK_PRIORITY, BORDER_CHECK_PRIORITY,
			BorderCheck_stk,
			TASK_STACKSIZE,
			NULL,
			0);

	OSTaskCreateExt(GenerateApple,
			NULL,
			(void *)&GenerateApple_stk[TASK_STACKSIZE-1],
			GENERATE_APPLE_PRIORITY, GENERATE_APPLE_PRIORITY,
			GenerateApple_stk,
			TASK_STACKSIZE,
			NULL,
			0);

		 // TO DO
		 // Grow task

	/* Ask player to press the 'up' button */
	char AskPressUpp[20];
	strcpy(AskPressUpp, "Ready?");
	alt_up_video_dma_draw_string(vgachar, AskPressUpp, 70,20, 0);
	strcpy(AskPressUpp, "Press 'up'!");
	alt_up_video_dma_draw_string(vgachar, AskPressUpp, 70,22, 0);

	/* If player is ready string */
	char Player1PressUpp[20];
	strcpy(Player1PressUpp, "Ready!");

while(1){
	ALT_SEM_PEND(directions1_sem, 0);
	/* If player is ready */
	if (directions1 == 'w'){
		alt_up_video_dma_draw_string(vgachar, Player1PressUpp, 70,26, 1);
		OSTimeDlyHMSM(0, 0, 1, 0);

		/* Clear words on screen */
		strcpy(AskPressUpp, "      ");
		alt_up_video_dma_draw_string(vgachar, AskPressUpp, 70,20, 0);
		strcpy(AskPressUpp, "           ");
		alt_up_video_dma_draw_string(vgachar, AskPressUpp, 70,22, 0);
		strcpy(Player1PressUpp, "      ");
		alt_up_video_dma_draw_string(vgachar, Player1PressUpp, 70,26, 1);


		directions1 = 0;

		/* Start count down */
		char CountDown[5];
		strcpy(CountDown, "3!");
		alt_up_video_dma_draw_string(vgachar, CountDown, 70,22, 0);
		OSTimeDlyHMSM(0,0,1,0);
		strcpy(CountDown, "2!");
		alt_up_video_dma_draw_string(vgachar, CountDown, 70,22, 0);
		OSTimeDlyHMSM(0,0,1,0);
		strcpy(CountDown, "1!");
		alt_up_video_dma_draw_string(vgachar, CountDown, 70,22, 0);
		OSTimeDlyHMSM(0,0,1,0);
		strcpy(CountDown, "GO!");
		alt_up_video_dma_draw_string(vgachar, CountDown, 70,22, 0);
		OSTimeDlyHMSM(0,0,0,800);

		OSTaskCreateExt(MoveSnake,
				&s1,
				(void *) &MoveSnake_stk[0][TASK_STACKSIZE - 1],
				MOVE_SNAKE_PRIORITY, MOVE_SNAKE_PRIORITY,
				MoveSnake_stk,
				TASK_STACKSIZE,
				NULL,
				0);

		/* Clear 'GO!' */
		strcpy(CountDown, "   ");
		alt_up_video_dma_draw_string(vgachar, CountDown, 70,22, 0);

		OSTimeDlyHMSM(0,0,0,200);
		ALT_SEM_POST(directions1_sem);
		OSTaskDel(OS_PRIO_SELF);
	}
	ALT_SEM_POST(directions1_sem);
	OSTimeDlyHMSM(0,0,0,300);
}

}

/* The main function creates tasks and starts multi-tasking */
int main(void){
	OSInit();
	int errs;
	errs = ALT_SEM_CREATE(&directions1_sem,1);
	errs = ALT_SEM_CREATE(&directions2_sem,1);
	int err = ALT_SEM_CREATE(&sem, 1);
	int err2 = ALT_SEM_CREATE(&menu_sem, 1);
	int err3 = ALT_SEM_CREATE(&Snake_Length, 1);
	int err4 = ALT_SEM_CREATE(&Snake_Loc, 1);
	int err5 = ALT_SEM_CREATE(&Apple_Loc, 1);


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

	OSTaskCreateExt(ReadKeyboardMenu,
		        NULL,
		        (void *)&ReadKeyboardMenu_stk[TASK_STACKSIZE-1],
		        READ_KEYBOARD_MENU_PRIORITY,
		        READ_KEYBOARD_MENU_PRIORITY,
		        ReadKeyboardMenu_stk,
		        TASK_STACKSIZE,
		        NULL,
		        0);


		  OSTaskCreateExt(MainMenu,
		        NULL,
		        (void *)&MainMenu_stk[TASK_STACKSIZE-1],
		        MAIN_MENU_PRIORITY,
		        MAIN_MENU_PRIORITY,
		        MainMenu_stk,
		        TASK_STACKSIZE,
		        NULL,
		        0);

	printf("end of main \n");

	OSStart();
	return 0;
}
