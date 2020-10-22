#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "LPC17xx.h"                    
#include "Open1768_LCD.h"
#include "LCD_ILI9325.h"
#include "asciiLib.h"
#include "PIN_LPC17xx.h"
#include "GPIO_LPC17xx.h"
#include "Board_LED.h"
#include "TP_Open1768.h"

int sinus[100];
int level = 15000;
const int width = 320;
const int height = 240;
const int piano_height = 0.9 * height;
const int menu_height = height - piano_height;
const int gap = 2;
const int wc_width = width/8 - gap;
const int bc_width = width/16;

int convertX(int x_old){
	return 320 - (x_old * 320 / 4096);
}

int convertY(int y_old){
	return y_old * 240 / 4096;
}

void stringToPC(const char * string){
	int i = 0;
	while(string[i]!='\0'){
		if((LPC_UART0->LSR & ( 1 << 5 )) >> 5 == 1){
			LPC_UART0->THR = string[i];
			i++;
		}
	}
}

void delay(int value){
	LPC_TIM0->TCR = 0x02;
	LPC_TIM0->TCR = 0x01;
	while(LPC_TIM0->TC < value);
	LPC_TIM0->TCR = 0x00;
}

int is_black_key(int i, int heigh, int wc_width, int bc_width, int gap){
    int row = i/320;
    int col = i%320;
    int k, l_pocz, l_kon;
    int width_white_and_gap = wc_width + gap;
    if(row  > (2.0/3.0) * heigh)
        return 0;

    for (k = 1; k < 7; k++){
        if(k == 3)
            continue;
        l_pocz = k*width_white_and_gap - bc_width/2;
        l_kon = k*width_white_and_gap + bc_width/2;


        if(col > l_pocz && col < l_kon)
            return k;
    }
    return 0;
}

void EINT3_IRQHandler(void){
	int i = 0;
	char str[30];
	int x = 0;
  int y = 0;
	int j;
	int coord;
  int black_keys[] = {210, 286, 152, 134, 117, 117};
  int white_keys[] = {224, 194, 171, 163, 140, 126, 112, 103, 103};
  int delay_value;
  touchpanelGetXY(&x, &y);
	x = convertX(x);
	y = convertY(y);
	
	if(x > 5 && y > 5){
	if(y < 35) {
		delay_value = 0;
        if(x < (320/2)) {
            level = (level - 1000) > 0 ? level - 1000 : level;
						sprintf(str, "LEVEL-- %d X-> %d  Y-> %d \n", level, x, y);
						stringToPC(str);
						for(i = 0; i < 100; i++)
							sinus[i] = (int) (level + level * sinf((float) i * (2.0f * 3.1415 / 100)));
        } else if(x >= (320/2)){
            level = (level + 1000) < 16000 ? level + 1000 : level;
						sprintf(str, "LEVEL++ %d X-> %d  Y-> %d \n", level, x, y); 
						stringToPC(str);
						for(i = 0; i < 100; i++)
							sinus[i] = (int) (level + level * sinf((float) i * (2.0f * 3.1415 / 100)));
        }
	} else if(is_black_key(coord, piano_height,  wc_width, bc_width, gap)){
        delay_value = black_keys[is_black_key(coord, piano_height,  wc_width, bc_width, gap) - 1];
  } else {
        delay_value = white_keys[x / (320/8) - 1];
  }
	
		while(i++ < 10){
		for(j = 0; j < 100; j++){
			LPC_DAC->DACR = sinus[j] << 6;
			delay(delay_value);
		}
	}
}
  
  LPC_GPIOINT->IO0IntClr |= 1 << 19;
   
  NVIC_ClearPendingIRQ(EINT3_IRQn);
}

int is_gap(int i, int wc_width, int gap){
    int col = i%320;
    int k, l_pocz, l_kon;
    int width_white_and_gap = wc_width + gap;
    for (k = 1; k < 8; k++){
        l_pocz = k * width_white_and_gap - gap;
        l_kon = k * width_white_and_gap;

        if(col > l_pocz && col <= l_kon)
            return 1;
    }
    return 0;
}

void draw_keys(void){
int i,j;
int row, column;
 for (i = 0; i < 240*320; i++){
        row = i/320;
        column = i%320;

        if(row < menu_height) {
            if(column < (320/2))
                lcdWriteData(LCDBlue);
            else
                lcdWriteData(LCDRed);
        }
        else if(row == menu_height) {
            lcdWriteData(LCDBlack);
        } else{
            if(is_black_key(i, piano_height,  wc_width, bc_width, gap) || is_gap(i, wc_width, gap)){
                lcdWriteData(LCDBlack);
            } else {
                lcdWriteData(LCDWhite);
            }
        }
    }
    
}

void dac_conf(void){
	LPC_TIM0->PR = 25;
	LPC_TIM0->MR0 = 500;
	LPC_TIM0->MCR = 3;
	LPC_TIM0->TCR = 1;
}

int main (void){
	int x = 0, y = 0;
	int index;
	char tab[30];
	double value = 4.75; 
  	int i,j;
		
	lcdConfiguration();
	LED_Initialize();
	init_ILI9325();
	lcdWriteReg(0x03, 0x1018);
	lcdWriteIndex(DATA_RAM);

	draw_keys();
	
	touchpanelInit();
	
	NVIC_EnableIRQ(EINT3_IRQn);
	
  	LPC_GPIOINT->IO0IntEnF |= 1 << 19;
	
	PIN_Configure(0, 26, 2, 0, 0);	

	PIN_Configure(0, 2, 1, 0, 0);
	
	PIN_Configure(0, 3, 1, 0, 0);
	
	LPC_UART0->LCR = 3 | 1 << 7;
	LPC_UART0->DLM = 0;
	LPC_UART0->DLL = 14;
	LPC_UART0->FCR = 3 << 1;
	LPC_UART0->LCR = 3 | 0 << 7;
		
	dac_conf();
	
	for(i = 0; i < 100; i++){
		sinus[i] = (int) (level + level * sinf((float)i * (2.0f * 3.1415 / 100)));
	}

	j = 0;
		
	while(1);

	return 0;
}
