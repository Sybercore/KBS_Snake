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
OS_STK GenerateApple_stk[TASK_STACKSIZE];
OS_STK SegScore_stk[TASK_STACKSIZE];
OS_STK StartSinglePlayer_stk[TASK_STACKSIZE];
OS_STK StartMultiPlayer_stk[TASK_STACKSIZE];
OS_STK CreateField_stk[TASK_STACKSIZE];
OS_STK ReadKeyboard_stk[TASK_STACKSIZE];
OS_STK BorderCheck_stk[2][TASK_STACKSIZE];
OS_STK MoveSnake_stk[2][TASK_STACKSIZE];
OS_STK GenerateApple_stk[TASK_STACKSIZE];
OS_STK SegScore_stk[TASK_STACKSIZE];
OS_STK MainMenu_stk[TASK_STACKSIZE];
OS_STK GameOver_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */
#define START_SINGLE_PLAYER_PRIORITY	1
#define START_MULTI_PLAYER_PRIORITY 	2
#define CREATE_FIELD_PRIORITY  			3
#define READ_KEYBOARD_PRIORITY  		4
#define BORDER_CHECK_PRIORITY			5
/* Other BORDER_CHECK_PRIORITY			6 */
#define MOVE_SNAKE_PRIORITY				7
/* Other MOVE_SNAKE_PRIORITY			8 */
#define GENERATE_APPLE_PRIORITY 		9
#define SEG_SCORE_PRIORITY 				10
#define MAIN_MENU_PRIORITY				11
#define GAME_OVER_PRIORITY				12

/* Other difine's */
#define MainGreen 			0x6e00
#define BLACK 				0x0000
#define WHITE				0xFFFF
#define BLUE				0x000F
#define YELLOW				0xe600
#define GREEN				0x0ff0
#define RED					0xf000
#define X_MAX_SIZE			273
#define Y_MAX_SIZE			232

/* Creating semaphore */
ALT_SEM(sem);
ALT_SEM(directions1_sem);
ALT_SEM(directions2_sem);
ALT_SEM(snake_length_sem);
ALT_SEM(apple_loc_sem);
ALT_SEM(snake_loc_sem);
ALT_SEM(menu_sem);

/* Screen variables */
alt_up_pixel_buffer_dma_dev* vgapixel;				//pixel buffer device
alt_up_video_dma_dev* vgachar;						//char buffer device
alt_up_character_lcd_dev *lcd_dev;
alt_up_parallel_port_dev *hex3_hex0_dev, *hex7_hex4_dev;
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

/* Global variables */
snake s1;
snake s2;
apple a;
char  directions1 = 0;
char  directions2 = 0;
char menu_id = 0;

/* Prototypes */
void StartSinglePlayer(void* pdata);
void StartMultiPlayer(void* pdata);
void CreateField (void* the_snake);
void ReadKeyboard(void* pdata);
void BorderCheck(void* the_snake);
void MoveSnake(void* the_snake);
void GenerateApple(void* pdata);
void SegScore(int id);
void MainMenu(void* pdata);
void GameOver(void* pdata);
snake CreateSnake(int id);
void EditSnake(int id);



/************************************************************************/
/*
	Prototype:		void ReadKeyboard(void* pdata)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
					<os/alt_sem.h>
	Parameters:		N/A
	Returns:		Nothing
	Description:	menu_id = 0 & 3: Reads the keyboard input, makes it
							possible to start a game and to go back to
							the main menu.
					menu_id = 1 & 2: Reads the keyboard input and sets
							the corresponding semaphore to the right
							direction.
*/
/************************************************************************/

void ReadKeyboard(void* pdata){
	/* Declare variables */
	int PS2_data, RVALID;
	int byte = 0, count = 0, loc = 0;

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
			ALT_SEM_PEND(menu_sem, 0);
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
								/* Start single player */
								menu_id = 1;

								/* Create single player game */
								OSTaskCreateExt(StartSinglePlayer,
										NULL,
										(void *) &StartSinglePlayer_stk[TASK_STACKSIZE - 1],
										START_SINGLE_PLAYER_PRIORITY, START_SINGLE_PLAYER_PRIORITY,
										StartSinglePlayer_stk,
										TASK_STACKSIZE,
										NULL,
										0);
						} else if (loc == 0){
							/* Start multiplier */
							menu_id = 2;

							/* Create multiplayer game */
							OSTaskCreateExt(StartMultiPlayer,
									NULL,
									(void *) &StartMultiPlayer_stk[TASK_STACKSIZE - 1],
									START_MULTI_PLAYER_PRIORITY, START_MULTI_PLAYER_PRIORITY,
									StartMultiPlayer_stk,
									TASK_STACKSIZE,
									NULL,
									0);
						}
						count = 0;
					}
				}
			}
			ALT_SEM_POST(menu_sem);
			ALT_SEM_PEND(menu_sem, 0);
			/* In-game movement */
			if (menu_id == 1 || menu_id == 2){
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
			ALT_SEM_POST(menu_sem);
			ALT_SEM_PEND(menu_sem, 0);
			/* Game over key */
			if (menu_id == 3){
				switch(byte){
					case 0x29:
						// Space
						if (count >= 2){
							/* Reset values */
							menu_id = 0;
							count = 0;
							loc = 0;

							/* Create the main menu */
							OSTaskCreateExt(MainMenu,
									NULL,
									(void *)&MainMenu_stk[TASK_STACKSIZE-1],
									MAIN_MENU_PRIORITY, MAIN_MENU_PRIORITY,
									MainMenu_stk,
									TASK_STACKSIZE,
									NULL,
									0);
						}
				}
			}
			ALT_SEM_POST(menu_sem);
			count++;
		}
    OSTimeDlyHMSM(0, 0, 0, 70);
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
	/* Declare variables */
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
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s.loc[0].x,s.loc[0].y,s.loc[0].x+6,s.loc[0].y+6,GREEN,GREEN);
	} else if (s.id == 2){
		alt_up_pixel_buffer_dma_draw_box(vgapixel,s.loc[0].x,s.loc[0].y,s.loc[0].x+6,s.loc[0].y+6,YELLOW,YELLOW);
	}
	OSTaskDel(OS_PRIO_SELF);
}


/************************************************************************/
/*
	Prototype:		void BorderCheck(void* the_snake)
	Include:		<os/alt_sem.h>
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
			ALT_SEM_PEND(snake_loc_sem, 0);
			if (s1.loc[0].x < 51 || s1.loc[0].x > 267){
				/* Snake is outside the border */
				ALT_SEM_PEND(directions1_sem, 0);
				directions1 = 'K';
				ALT_SEM_POST(directions1_sem);
				ALT_SEM_POST(snake_loc_sem);
				OSTaskDel(OS_PRIO_SELF);
			} else if (s1.loc[0].y < 9 || s1.loc[0].y > 225){
				/* Snake is outside the border */
				ALT_SEM_PEND(directions1_sem, 0);
				directions1 = 'K';
				ALT_SEM_POST(directions1_sem);
				ALT_SEM_POST(snake_loc_sem);
				OSTaskDel(OS_PRIO_SELF);
			}
			ALT_SEM_POST(snake_loc_sem);
		}
		if (s.id == 2){
			ALT_SEM_PEND(snake_loc_sem, 0);
			if (s2.loc[0].x < 51 || s2.loc[0].x > 267){
				/* Snake is outside the border */
				ALT_SEM_PEND(directions2_sem, 0);
				directions2 = 'K';
				ALT_SEM_POST(directions2_sem);
				ALT_SEM_POST(snake_loc_sem);
				OSTaskDel(OS_PRIO_SELF);
			} else if (s2.loc[0].y < 9 || s2.loc[0].y > 225){
				/* Snake is outside the border */
				ALT_SEM_PEND(directions2_sem, 0);
				directions2 = 'K';
				ALT_SEM_POST(directions2_sem);
				ALT_SEM_POST(snake_loc_sem);
				OSTaskDel(OS_PRIO_SELF);
			}
			ALT_SEM_POST(snake_loc_sem);
		}
		OSTimeDlyHMSM(0,0,0,80);
	}
}


/************************************************************************/
/*
	Prototype:		snake CreateSnake(int id)
	Include:		N/A
	Parameters:		id - The id the snake get's.
	Returns:		var_snake on success.
	Description:	Creates a snake and set's it values.
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
	Prototype:		void AddSnake(int id)
	Include:		<os/alt_sem.h>
	Parameters:		id - The front of the snake
	Returns:		Nothing
	Description:	Shifts the locations of the snake and add a new
					location if needed.
*/
/************************************************************************/

void EditSnake(int id){
	/* Declare variables */
	int i = 0;

	ALT_SEM_PEND(snake_length_sem, 0);
	ALT_SEM_PEND(snake_loc_sem, 0);
	if (id == 1){
		/* Shifts snake 1 */
		for (i = s1.length; i >= 1; i--){
			s1.loc[i] = s1.loc[i-1];
		}
	} else if (id == 2){
		/* Shifts snake 2 */
		for (i = s2.length; i >= 1; i--){
			s2.loc[i] = s2.loc[i-1];
		}
	}
	ALT_SEM_POST(snake_length_sem);
	ALT_SEM_POST(snake_loc_sem);
}


/************************************************************************/
/*
	Prototype:		void MoveSnake(void* the_snake)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
					<os/alt_sem.h>
	Parameters:		the_snake - The snake that needs the move.
	Returns:		Nothing
	Description:	Move's the snake in the corresponding direction.
*/
/************************************************************************/

void MoveSnake(void* the_snake){
	/* Declare variables */
	snake s = *((snake*) the_snake);
	int y,i;
	int hex_values = 63 | 63 << 8 | 63 << 16 | 63 << 24;

	/* Set the 7 segment displays to display 0*/
	ALT_SEM_PEND(menu_sem, 0);
	if (menu_id == 1){
		alt_up_parallel_port_write_data(hex7_hex4_dev, hex_values);
	} else if (menu_id == 2) {
		alt_up_parallel_port_write_data(hex7_hex4_dev, hex_values);
		alt_up_parallel_port_write_data(hex3_hex0_dev, hex_values);
	}
	ALT_SEM_POST(menu_sem);

	while(1){
		/* Draws apple */
		alt_up_pixel_buffer_dma_draw_box(vgapixel,a.x,a.y,a.x + 6,a.y+6,RED,RED);

		/* Player 1 check if touched a snake */
		if(s.id == 1){
			for(y = 0; y <= s1.length;y++){
				ALT_SEM_PEND(directions1_sem, 0);
				if(s1.loc[0].x == s1.loc[y+1].x && s1.loc[0].y == s1.loc[y+1].y){
					/* Snake hit it's self */
					directions1 = 'K';
				}
				ALT_SEM_POST(directions1_sem);

				ALT_SEM_PEND(menu_sem,0);
				if(menu_id == 2){
					ALT_SEM_PEND(directions1_sem,0);
					if(s1.loc[0].x == s2.loc[0].x && s1.loc[0].y == s2.loc[0].y){
						/* Snake hit other snake's head */
						directions1 = 'K';
					} else {
						for(i = 0; i < s2.length; i++){
							if(s1.loc[0].x == s2.loc[i+1].x && s1.loc[0].y == s2.loc[i+1].y){
								/* Snake hit other snake */
								directions1 = 'K';
							}
						}
					}
					ALT_SEM_POST(directions1_sem);
				}
				ALT_SEM_POST(menu_sem);
			}
		}


		/* player 2 check if touched a snake */
		if(s.id == 2){
			for(y = 0; y <= s2.length;y++){
				ALT_SEM_PEND(directions2_sem, 0);
				if(s2.loc[0].x == s2.loc[y+1].x && s2.loc[0].y == s2.loc[y+1].y){
					/* Snake hit it's self */
					directions2 = 'K';
				}
				ALT_SEM_POST(directions2_sem);

				ALT_SEM_PEND(menu_sem,0);
				if(menu_id == 2){
					ALT_SEM_PEND(directions2_sem,0);
					if(s2.loc[0].x == s1.loc[0].x && s2.loc[0].y == s1.loc[0].y){
						/* Snake hit other snake's head */
						directions2 = 'K';
					} else {
						for(i = 0; i < s1.length; i++){
							if(s2.loc[0].x == s1.loc[i+1].x && s2.loc[0].y == s1.loc[i+1].y){
								/* Snake hit other snake */
								directions2 = 'K';
							}
						}
					}
					ALT_SEM_POST(directions2_sem);
				}
				ALT_SEM_POST(menu_sem);
			}
		}


		/* Checks if apple is eaten by player 1 */
		if(s1.loc[0].x == a.x && s1.loc[0].y == a.y){
			/* Apple has been eaten */
			s1.length = s1.length + 2;
			/* Update the 7 segment display*/
			OSTaskCreateExt(SegScore,
					1,
					(void *)&SegScore_stk[TASK_STACKSIZE-1],
					SEG_SCORE_PRIORITY, SEG_SCORE_PRIORITY,
					SegScore_stk,
					TASK_STACKSIZE,
					NULL,
					0);
			printf("Apple Eaten snake1 = %d\n", s1.length);
			/* Make apple green */
			alt_up_pixel_buffer_dma_draw_box(vgapixel,a.x,a.y,a.x + 6,a.y+6,GREEN,GREEN);

			/* Generate a new apple */
			OSTaskCreateExt(GenerateApple,
					NULL,
					(void *)&GenerateApple_stk[TASK_STACKSIZE-1],
					GENERATE_APPLE_PRIORITY, GENERATE_APPLE_PRIORITY,
					GenerateApple_stk,
					TASK_STACKSIZE,
					NULL,
					0);
		}

		/* Checks if apple is eaten by player 2 */
		if(s2.loc[0].x == a.x && s2.loc[0].y == a.y){
			/* Apple has been eaten */
			s2.length++;
			/* Update the 7 segment display*/
			OSTaskCreateExt(SegScore,
					2,
					(void *)&SegScore_stk[TASK_STACKSIZE-1],
					SEG_SCORE_PRIORITY, SEG_SCORE_PRIORITY,
					SegScore_stk,
					TASK_STACKSIZE,
					NULL,
					0);

			printf("Apple Eaten snake2 = %d\n", s2.length);
			/* Make apple yellow */
			alt_up_pixel_buffer_dma_draw_box(vgapixel,a.x,a.y,a.x + 6,a.y+6,YELLOW,YELLOW);

			/* Generate a new apple */
			OSTaskCreateExt(GenerateApple,
					NULL,
					(void *)&GenerateApple_stk[TASK_STACKSIZE-1],
					GENERATE_APPLE_PRIORITY, GENERATE_APPLE_PRIORITY,
					GenerateApple_stk,
					TASK_STACKSIZE,
					NULL,
					0);
		}


		/* Draw the movement of the snake */
		if (s.id == 1){
			ALT_SEM_PEND(directions1_sem, 0);
			if (s1.length == 0){
				/* Old location becomes black */
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[0].x,s1.loc[0].y,s1.loc[0].x+6,s1.loc[0].y+6,BLACK,BLACK);
			} else {
				/* Remove last of old snake */
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[s1.length].x,s1.loc[s1.length].y,s1.loc[s1.length].x+6,s1.loc[s1.length].y+6,BLACK,BLACK);
				EditSnake(1);
			}

			if (directions1 == 'w'){
				s1.loc[0].y = s1.loc[0].y-8; // Change the location of the snake
			}
			else if (directions1 == 'a'){
				s1.loc[0].x = s1.loc[0].x-8; // Change the location of the snake
			}
			else if (directions1 == 's'){
				s1.loc[0].y = s1.loc[0].y+8; // Change the location of the snake
			}
			else if (directions1 == 'd'){
				s1.loc[0].x = s1.loc[0].x+8; // Change the location of the snake
			}

			if (s1.length == 0){
				/* Draw new location */
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[0].x,s1.loc[0].y,s1.loc[0].x+6,s1.loc[0].y+6,GREEN,GREEN);
			} else {
				/* Draw the new head of the snake */
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s1.loc[0].x,s1.loc[0].y,s1.loc[0].x+6,s1.loc[0].y+6,GREEN,GREEN);
			}

			ALT_SEM_POST(directions1_sem);
		}

		/* Draw the movement of the snake */
		if(s.id == 2){
			ALT_SEM_PEND(directions2_sem,0);
			if (s2.length == 0){
				/* Old location becomes black */
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s2.loc[0].x,s2.loc[0].y,s2.loc[0].x+6,s2.loc[0].y+6,BLACK,BLACK);
			} else {
				/* Remove last of old snake */
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s2.loc[s2.length].x,s2.loc[s2.length].y,s2.loc[s2.length].x+6,s2.loc[s2.length].y+6,BLACK,BLACK);
				EditSnake(2);
			}

			if (directions2 == 'u'){
				s2.loc[0].y = s2.loc[0].y-8; // Change the location of the snake
			}
			else if (directions2 == 'l'){
				s2.loc[0].x = s2.loc[0].x-8; // Change the location of the snake
			}
			else if (directions2 == 'z'){
				s2.loc[0].y = s2.loc[0].y+8; // Change the location of the snake
			}
			else if (directions2 == 'r'){
				s2.loc[0].x = s2.loc[0].x+8; // Change the location of the snake
			}

			if (s2.length == 0){
				/* Draw new location */
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s2.loc[0].x,s2.loc[0].y,s2.loc[0].x+6,s2.loc[0].y+6,YELLOW,YELLOW);
			} else {
				/* Draw the new head of the snake */
				alt_up_pixel_buffer_dma_draw_box(vgapixel,s2.loc[0].x,s2.loc[0].y,s2.loc[0].x+6,s2.loc[0].y+6,YELLOW,YELLOW);
			}

			ALT_SEM_POST(directions2_sem);
		}


		/* Checks if the snake has been outside the border */
		ALT_SEM_PEND(directions1_sem, 0);
		ALT_SEM_PEND(directions2_sem, 0);
		if (directions1 == 'K' || directions2 == 'K'){
			/* Create game over task */
			OSTaskCreateExt(GameOver,
					NULL,
					(void *) &GameOver_stk[TASK_STACKSIZE - 1],
					GAME_OVER_PRIORITY,GAME_OVER_PRIORITY,
					GameOver_stk,
					TASK_STACKSIZE,
					NULL,
					0);

		  	ALT_SEM_POST(directions1_sem);
		  	ALT_SEM_POST(directions2_sem);
		}
		ALT_SEM_POST(directions1_sem);
		ALT_SEM_POST(directions2_sem);
		OSTimeDlyHMSM(0,0,0,110);
	}
}


/************************************************************************/
/*
	Prototype:		void GameOver(void* pdata)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
					<altera_up_avalon_video_dma_controller.h>
					<os/alt_sem.h>
	Parameters:		N/A
	Returns:		Nothing
	Description:	Resets value's and shows game over screen.
*/
/************************************************************************/

void GameOver(void* pdata){
	/* Declare variables */
	char i = 0, scorep1 = 0,scorep2 = 0;

	/* Check who died */
	if (directions1 == 'K'){
		i = '2';
	} else {
		i = '1';
	}

	ALT_SEM_PEND(menu_sem, 0);
	/* Reset variables */
	if (menu_id == 1){
		directions1 = 0;
		OSTaskDel(MOVE_SNAKE_PRIORITY);
	} else if (menu_id == 2){
		directions1 = 0;
		directions2 = 0;
		OSTaskDel(MOVE_SNAKE_PRIORITY);
		OSTaskDel(MOVE_SNAKE_PRIORITY+1);
	}
	ALT_SEM_POST(menu_sem);

	/* Set score's */
	if(menu_id == 1){
		scorep1 = s1.length;
	}
	if(menu_id == 2){
		scorep1 = s1.length;
		scorep2 = s2.length;
	}

	/* Clear screen */
	alt_up_pixel_buffer_dma_clear_screen(vgapixel, BLACK);
	alt_up_video_dma_screen_fill(vgachar, BLACK, 0);
	alt_up_video_dma_screen_fill(vgachar, BLACK, 1);

	/* Draw game over screen */
	char GameOverString[25];
	strcpy(GameOverString, "GAME OVER");
	alt_up_video_dma_draw_string(vgachar, GameOverString, 35,30, 0);

	ALT_SEM_PEND(menu_sem, 0);
	if (menu_id == 2){
	snprintf(GameOverString, sizeof(GameOverString), "WINNER: Player %c", i);
	alt_up_video_dma_draw_string(vgachar, GameOverString, 33,36, 0);

	snprintf(GameOverString, sizeof(GameOverString), "P2 score: %d", scorep2);
	alt_up_video_dma_draw_string(vgachar, GameOverString, 35,42, 0);
	}
	ALT_SEM_POST(menu_sem);

	snprintf(GameOverString, sizeof(GameOverString), "P1 score: %d", scorep1);
	alt_up_video_dma_draw_string(vgachar, GameOverString, 35,40, 0);

	OSTimeDlyHMSM(0,0,2,0);

	/* Draw's press enter to continue */
	strcpy(GameOverString, "Press space to continue");
	alt_up_video_dma_draw_string(vgachar, GameOverString, 29,32, 0);

	ALT_SEM_PEND(menu_sem, 0);
	 /* Make keyboard responsive to the correct keys */
	menu_id = 3;
	ALT_SEM_POST(menu_sem);

	OSTaskDel(OS_PRIO_SELF);
}


/************************************************************************/
/*
	Prototype:		void GenerateApple(void* pdata)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
					<os/alt_sem.h>
	Parameters:		N/A
	Returns:		Nothing
	Description:	Generates an apple at a random location and draw's it.
*/
/************************************************************************/

void GenerateApple(void* pdata){
	ALT_SEM_PEND(apple_loc_sem, 0);
	/* Randomizer */
	srand(time());
	int yLocation = rand() % 28;
	srand(time());
	int xLocation = rand()% 28;

	/* Set's the location in the struct and draw's the apple */
	a.x =  51+(xLocation*8);
	a.y =  9+(yLocation*8);

	alt_up_pixel_buffer_dma_draw_box(vgapixel,a.x,a.y,a.x + 6,a.y+6,RED,RED);

	printf("Appel created\n");
	ALT_SEM_POST(apple_loc_sem);
	OSTaskDel(OS_PRIO_SELF);
}


/************************************************************************/
/*
	Prototype:		void MainMenu(void* pdata)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
					<altera_up_avalon_video_dma_controller.h>
	Parameters:		N/A
	Returns:		Nothing
	Description:	Draws the main screen and resets the 7 segment
					displays.
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
	strcpy(menu_string3, "Multiplayer");

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

	/* Clear 7 segment displays */
	int hex_values = 0 | 0 << 8 | 0 << 16 | 0 << 24;
	alt_up_parallel_port_write_data(hex7_hex4_dev, hex_values);
	alt_up_parallel_port_write_data(hex3_hex0_dev, hex_values);

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
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
					<altera_up_avalon_video_dma_controller.h>
					<os/alt_sem.h>
	Parameters:		N/A
	Returns:		Nothing
	Description:	Start up the tasks for the game and does the countdown.
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

	OSTaskCreateExt(BorderCheck,
			&s1,
			(void *) &BorderCheck_stk[0][TASK_STACKSIZE - 1],
			BORDER_CHECK_PRIORITY, BORDER_CHECK_PRIORITY,
			BorderCheck_stk[0],
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

	/* Ask player to press the 'down' button */
	char AskPressDown[20];
	strcpy(AskPressDown, "Ready?");
	alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,20, 0);
	strcpy(AskPressDown, "Press");
	alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,22, 0);
	strcpy(AskPressDown,"'Down!'");
	alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,24,0);

	/* If player is ready string */
	char Player1PressDown[20];
	strcpy(Player1PressDown, "Ready!");

	while(1){
		ALT_SEM_PEND(directions1_sem, 0);
		/* If player is ready */
		if (directions1 == 's'){

			alt_up_video_dma_draw_string(vgachar, Player1PressDown, 70,28, 1);
			OSTimeDlyHMSM(0, 0, 1, 0);

			/* Clear words on screen */
			strcpy(AskPressDown, "       ");
			alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,20, 0);
			alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,22, 0);
			alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,24,0);
			strcpy(Player1PressDown, "       ");
			alt_up_video_dma_draw_string(vgachar, Player1PressDown, 70,28, 1);

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

			/* Reset direction */
			directions1 = 0;

			OSTaskCreateExt(MoveSnake,
					&s1,
					(void *) &MoveSnake_stk[0][TASK_STACKSIZE - 1],
					MOVE_SNAKE_PRIORITY, MOVE_SNAKE_PRIORITY,
					MoveSnake_stk[0],
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


/************************************************************************/
/*
	Prototype:		void StartMultiPlayer(void* pdata)
	Include:		<altera_up_avalon_video_pixel_buffer_dma.h>
					<altera_up_avalon_video_dma_controller.h>
					<os/alt_sem.h>
	Parameters:		N/A
	Returns:		Nothing
	Description:	Start up the tasks for the game and does the countdown.
*/
/************************************************************************/

void StartMultiPlayer(void* pdata){
	/* Clear screen */
	alt_up_pixel_buffer_dma_clear_screen(vgapixel, BLACK);
	alt_up_video_dma_screen_fill(vgachar, BLACK, 0);
	alt_up_video_dma_screen_fill(vgachar, BLACK, 1);

	s1 = CreateSnake(1);
	s2 = CreateSnake(2);

	/* Create tasks for multiplayer */
	OSTaskCreateExt(CreateField,
			NULL,
			(void *) &CreateField_stk[TASK_STACKSIZE - 1],
			CREATE_FIELD_PRIORITY, CREATE_FIELD_PRIORITY,
			CreateField_stk,
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

	OSTaskCreateExt(BorderCheck,
			&s2,
			(void *) &BorderCheck_stk[1][TASK_STACKSIZE - 1],
			BORDER_CHECK_PRIORITY+1, BORDER_CHECK_PRIORITY+1,
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

	/* Ask player to press the 'Down' button */
	char AskPressDown[20];
	strcpy(AskPressDown, "Ready?");
	alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,20, 0);
	strcpy(AskPressDown, "Press");
	alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,22, 0);
	strcpy(AskPressDown,"'Down!'");
	alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,24,0);

	/* If player is ready string */
	char Player1PressDown[20];
	strcpy(Player1PressDown, "Ready!");

	while(1){
		ALT_SEM_PEND(directions1_sem, 0);
		/* If players are ready */
		if (directions1 == 's' && directions2 == 'z'){
			alt_up_video_dma_draw_string(vgachar, Player1PressDown, 70,28, 1);
			OSTimeDlyHMSM(0, 0, 1, 0);

			/* Clear words on screen */
			strcpy(AskPressDown, "        ");
			alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,20, 0);
			alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,22, 0);
			alt_up_video_dma_draw_string(vgachar, AskPressDown, 70,24,0);
			strcpy(Player1PressDown, "        ");
			alt_up_video_dma_draw_string(vgachar, Player1PressDown, 70,28, 1);

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

			/* Reset directions */
			directions1 = 0;
			directions2 = 0;

			OSTaskCreateExt(MoveSnake,
					&s1,
					(void *) &MoveSnake_stk[0][TASK_STACKSIZE - 1],
					MOVE_SNAKE_PRIORITY, MOVE_SNAKE_PRIORITY,
					MoveSnake_stk,
					TASK_STACKSIZE,
					NULL,
					0);

			OSTaskCreateExt(MoveSnake,
					&s2,
					(void *) &MoveSnake_stk[1][TASK_STACKSIZE - 1],
					MOVE_SNAKE_PRIORITY+1, MOVE_SNAKE_PRIORITY+1,
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


/************************************************************************/
/*
	Prototype:		void SegScore(int id)
	Include:		<altera_up_avalon_parallel_port.h>
					<os/alt_sem.h>
	Parameters:		id - The id of the snake.
	Returns:		Nothing
	Description:	Updates the score on the 7 segment displays.
*/
/************************************************************************/

void SegScore(int id){
	/* Declare variables */
	unsigned int seg_values;
	int seg = 0, score = 0, number1 = 0, number2 = 0, number3 = 0, number4 = 0;

	while (1) {
		/* Check who's score it is that needs to be adjusted */
		ALT_SEM_PEND(snake_length_sem,0);
		if (id == 1){
			score = s1.length;
		}
		if (id == 2){
			score = s2.length;
		}
		ALT_SEM_POST(snake_length_sem);

			for (int i = 0; i < 4; i++){
				int temp = score % 10;
				/* Checks what the last number is of score and sets hex to the correct number */
				switch (temp){
					case 0:
						seg = 63;
						break;
					case 1:
						seg = 6;
						break;
					case 2:
						seg = 91;
						break;
					case 3:
						seg = 79;
						break;
					case 4:
						seg = 102;
						break;
					case 5:
						seg = 109;
						break;
					case 6:
						seg = 125;
						break;
					case 7:
						seg = 7;
						break;
					case 8:
						seg = 127;
						break;
					case 9:
						seg = 111;
					}

				/* Sets the value to the right display */
				if (i == 0){
					number1 = seg;
				}
				if (i == 1){
					number2 = seg;
				}
				if (i == 2){
					number3 = seg;
				}
				if (i == 3){
					number4 = seg;
				}

				score = score / 10;
			}

			/* Writes the data to the 7 segment display */
			if (id == 1){
				seg_values = number1 | number2 << 8 | number3 << 16 | number4 << 24;
				alt_up_parallel_port_write_data(hex7_hex4_dev, seg_values);
			}
			if (id == 2){
				seg_values = number1 | number2 << 8 | number3 << 16 | number4 << 24;
				alt_up_parallel_port_write_data(hex3_hex0_dev, seg_values);
			}
			OSTaskDel(OS_PRIO_SELF);
		}
}

/* The main function creates tasks and starts multi-tasking */
int main(void){
	OSInit();
	/* Declare variables */
	int errs;

	/* Create semaphore's */
	errs = ALT_SEM_CREATE(&directions1_sem,1);
	errs = ALT_SEM_CREATE(&directions2_sem,1);
	errs = ALT_SEM_CREATE(&sem, 1);
	errs = ALT_SEM_CREATE(&menu_sem, 1);
	errs = ALT_SEM_CREATE(&snake_length_sem, 1);
	errs = ALT_SEM_CREATE(&snake_loc_sem, 1);
	errs = ALT_SEM_CREATE(&apple_loc_sem, 1);

	if (errs != 0)
		printf("Semaphore NOT created\n");

	vgapixel = alt_up_pixel_buffer_dma_open_dev("/dev/VGA_Subsystem_VGA_Pixel_DMA");	//open pixel buffer
	if (vgapixel == NULL) {
		printf("Error: could not open VGA_Pixel_Buffer device\n");
		return -1;
	}
	else
		printf("Opened VGA_Pixel_Buffer device\n");

	alt_up_pixel_buffer_dma_clear_screen(vgapixel, WHITE);	//clear screen

	vgachar = alt_up_video_dma_open_dev("/dev/VGA_Subsystem_Char_Buf_Subsystem_Char_Buf_DMA");	//open char buffer
	if (vgachar == NULL) {
		printf("Error: could not open VGA_Char_Buffer device\n");
		return -1;
	}
	else
		alt_up_video_dma_draw_string(vgachar, "", 1, 1, 1);
		printf("Opened VGA_Char_Buffer device\n");


	/* Output text message to the LCD */
	ALT_SEM_PEND(sem, 0);

	lcd_dev = alt_up_character_lcd_open_dev("/dev/Char_LCD_16x2");
	if (lcd_dev == NULL) {
		printf("Error: could not open character LCD device\n");
		return -1;
	} else
		printf("Opened character LCD device\n");
	ALT_SEM_POST(sem);

	/* Open segment displays */
	hex3_hex0_dev = alt_up_parallel_port_open_dev("/dev/HEX3_HEX0");
	hex7_hex4_dev = alt_up_parallel_port_open_dev("/dev/HEX7_HEX4");

	alt_up_character_lcd_set_cursor_pos(lcd_dev, 0, 0); // set LCD cursor location to top row
	alt_up_character_lcd_string(lcd_dev, text_top_row);
	alt_up_character_lcd_cursor_off(lcd_dev); // turn off the LCD cursor

	/* Create beginning tasks */
	OSTaskCreateExt(ReadKeyboard,
			NULL,
			(void *)&ReadKeyboard_stk[TASK_STACKSIZE-1],
			READ_KEYBOARD_PRIORITY, READ_KEYBOARD_PRIORITY,
			ReadKeyboard_stk,
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
