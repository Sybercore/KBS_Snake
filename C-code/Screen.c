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
OS_STK task1_stk[TASK_STACKSIZE];
OS_STK task2_stk[TASK_STACKSIZE];
OS_STK task3_stk[TASK_STACKSIZE];
OS_STK task4_stk[TASK_STACKSIZE];

/* Definition of Task Priorities */

#define TASK1_PRIORITY      10
#define TASK2_PRIORITY      11
#define TASK3_PRIORITY      12
#define TASK4_PRIORITY      9
#define WHITE				0xFFFF
#define BLUE				0x000F
#define X_MAX_SIZE			270
#define Y_MAX_SIZE			230

ALT_SEM(sem);

/*variables - devices*/
alt_up_pixel_buffer_dma_dev* vgapixel;				//pixel buffer device
alt_up_video_dma_dev* vgachar;						//char buffer device
alt_up_character_lcd_dev *lcd_dev;
alt_up_parallel_port_dev *red_LEDs_dev;
alt_up_parallel_port_dev *green_LEDs_dev;
alt_up_parallel_port_dev *slider_switches_dev;
alt_up_parallel_port_dev *hex3_hex0_dev, *hex7_hex4_dev;

/* create a message to be displayed on the VGA and LCD displays */

void task1(void* pdata) {
	int x=0, y =0,i = 0, o = 0;



		ALT_SEM_PEND(sem, 0);

/*for (y = 0; y <= 35; y++ ){
	i = i + 8;
		alt_up_pixel_buffer_dma_draw_hline(vgapixel,20,X_MAX_SIZE,i,BLUE,BLUE);
}

for (x = 0 ; x <= 40; x++){
	o=o+8;
		alt_up_pixel_buffer_dma_draw_vline(vgapixel,o,5,Y_MAX_SIZE,BLUE,BLUE);
}

*/


 for(i = 0 ; i < 25; i++){
	 y = y +8;
	alt_up_pixel_buffer_dma_draw_hline(vgapixel,50,X_MAX_SIZE,y,BLUE,BLUE);
 }

 for(i = 0 ; i < 25; i++){
 	 x = x +8;
 	alt_up_pixel_buffer_dma_draw_vline(vgapixel,x,10,Y_MAX_SIZE,BLUE,BLUE);
  }

}

/*
void task2(void* pdata)
{
	int toggle = 0;
	while (1)
	{
		ALT_SEM_PEND(sem, 0);

		if (toggle == 0)
		{
			strcpy(text_bottom_row, "Task2");
			toggle = 1;
		}
		else
		{
			strcpy(text_bottom_row, "     ");
			toggle = 0;
		}

		alt_up_character_lcd_set_cursor_pos(lcd_dev, 5, 1); // set LCD cursor location to bottom row
		alt_up_character_lcd_string(lcd_dev, text_bottom_row);

		OSTimeDlyHMSM(0, 0, 0, 500);
		ALT_SEM_POST(sem);
		OSTimeDlyHMSM(0, 0, 1, 0);
	}
}

void task3(void* pdata) {
	int sw_values, hex_values;

	while (1) {
		sw_values = alt_up_parallel_port_read_data(slider_switches_dev);

		sw_values &= 0x000000FF;
		hex_values = sw_values | sw_values << 8 | sw_values << 16 | sw_values << 24;

		alt_up_parallel_port_write_data(hex3_hex0_dev, hex_values);

		OSTimeDlyHMSM(0, 0, 0, 100);
	}
}

void task4(void* pdata) {
	unsigned int countR = 1;
	unsigned int countG = 1 << 8;
	int toggle = 1;

	while (1) {
		alt_up_parallel_port_write_data(red_LEDs_dev, countR);
		alt_up_parallel_port_write_data(green_LEDs_dev, countG);

		OSTimeDlyHMSM(0, 0, 0, 100);

		alt_up_parallel_port_write_data(red_LEDs_dev, 0x000000);
		alt_up_parallel_port_write_data(green_LEDs_dev, 0x000000);


		if (countR < 1 << 17)
			countR = countR << 1;
		else
			countR = 1;

		if (toggle)
		{
			if (countG == 1)
				countG = countG << 8;
			else
				countG = countG >> 1;
		}
		toggle = !toggle;
	}
}*/

/* The main function creates two task and starts multi-tasking */
//int main(void) {
//	OSInit();
//
//	int err = ALT_SEM_CREATE(&sem, 1);
//	if (err != 0)
//		printf("Semaphore NOT created\n");
//
//	vgapixel = alt_up_pixel_buffer_dma_open_dev("/dev/VGA_Subsystem_VGA_Pixel_DMA");		//open pixel buffer
//	if (vgapixel == NULL) {
//		printf("Error: could not open VGA_Pixel_Buffer device\n");
//		return -1;
//	}
//	else
//		printf("Opened VGA_Pixel_Buffer device\n");
//
//	alt_up_pixel_buffer_dma_clear_screen(vgapixel, WHITE);							//clear screen
//
//	vgachar = alt_up_video_dma_open_dev("/dev/VGA_Subsystem_Char_Buf_Subsystem_Char_Buf_DMA");				//open char buffer
//	if (vgachar == NULL) {
//		printf("Error: could not open VGA_Char_Buffer device\n");
//		return -1;
//	}
//	else
//		printf("Opened VGA_Char_Buffer device\n");
//
//	alt_up_video_dma_screen_clear(vgachar, WHITE);											//clear buffer
//
//
//	/* output text message to the LCD */
//	ALT_SEM_PEND(sem, 0);
//
//	lcd_dev = alt_up_character_lcd_open_dev("/dev/Char_LCD_16x2");
//	if (lcd_dev == NULL) {
//		printf("Error: could not open character LCD device\n");
//		return -1;
//	} else
//		printf("Opened character LCD device\n");
//	ALT_SEM_POST(sem);
//
//	//red_LEDs_dev = alt_up_parallel_port_open_dev("/dev/Red_LEDs");
//	//green_LEDs_dev = alt_up_parallel_port_open_dev("/dev/Green_LEDs");
//	//slider_switches_dev = alt_up_parallel_port_open_dev("/dev/Slider_Switches");
//
//	//hex3_hex0_dev = alt_up_parallel_port_open_dev("/dev/HEX3_HEX0");
//	//hex7_hex4_dev = alt_up_parallel_port_open_dev("/dev/HEX7_HEX4");
//
//	OSTaskCreateExt(task1, NULL, (void *) &task1_stk[TASK_STACKSIZE - 1],
//		TASK1_PRIORITY, TASK1_PRIORITY, task1_stk, TASK_STACKSIZE, NULL, 0);
//
//	//OSTaskCreateExt(task2, NULL, (void *) &task2_stk[TASK_STACKSIZE - 1],
//	//		TASK2_PRIORITY, TASK2_PRIORITY, task2_stk, TASK_STACKSIZE, NULL, 0);
//
//	//OSTaskCreateExt(task3, NULL, (void *) &task3_stk[TASK_STACKSIZE - 1],
//	//		TASK3_PRIORITY, TASK3_PRIORITY, task3_stk, TASK_STACKSIZE, NULL, 0);
//
//	//OSTaskCreateExt(task4, NULL, (void *) &task4_stk[TASK_STACKSIZE - 1],
//	//		TASK4_PRIORITY, TASK4_PRIORITY, task4_stk, TASK_STACKSIZE, NULL, 0);
//
//	OSStart();
//	return 0;
//}


