/*********************************************
*															*
* 		SMSP,Inc.			I2C OLED module		*																	
* 		Module		:		ER-OLEDM1602-4			*
* 		Lanuage		:		CCS 5.012 Code			*
* 		Create		:		sysypet					*
* 		Date			:		2013.11.04				*
* 		Drive IC		:		US2066					*
* 		INTERFACE	:		I2C						*
*		MCU			:		16Fxxxx/18Fxxxx		*
*		VDD			:		3.3V						*
*															*
**********************************************/

#use		i2c	(MASTER,FORCE_SW,SCL=PIN_A6,SDA=PIN_A7)

#define	OLED_Write 					0x78												// slave addresses with write
#define	OLED_Read 					0x79												// slave addresses with read
#define	LCD_line_two 				0x40 												// LCD RAM address for the 2nd line

void		LCD_init						();
void 		Write_CGRAM					(int);
short		Busy							();
void 		write_CGROM					(int);
void 		horizontal_scroll			();
void 		shift_to_right				();
void 		shift_to_left				();
void		LCD_Send_byte				(int,int);
void		LCD_Gotoxy					(int,int);


static	int font[]=
{
	0x1f,0x00,0x1f,0x00,0x1f,0x00,0x1f,0x00,
	0x00,0x1f,0x00,0x1f,0x00,0x1f,0x00,0x1f,
	0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,
	0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,0x0a,
	0x08,0x0f,0x12,0x0f,0x0a,0x1f,0x02,0x02,	//Äê
	0x0f,0x09,0x0f,0x09,0x0f,0x09,0x09,0x13,	//ÔÂ
	0x1f,0x11,0x11,0x1f,0x11,0x11,0x11,0x1F,	//ÈÕ
	0x0C,0x0a,0x11,0x1f,0x09,0x09,0x09,0x13,	//¡Ö
};

/********************************************************************/
short Busy(void)
{
short busy=TRUE;

	while((busy&0x80)==0x80)
 	{
		i2c_start();
		i2c_write(OLED_Read);														// slave addresses with read
		i2c_write(0x80);																// Continuation bit  command
		busy=i2c_read(1)&0x80;
		i2c_stop();
		return(busy);
	}
}

/********************************************************************/
void Write_CGRAM(int  a[])
{
int i=64,k;

	i2c_start();
	i2c_write(OLED_Write);															// slave addresses with write
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x40);																	// CORMA address
	i2c_write(0x40);																	// Continuation bit  data 
	for(k=0;k<64;k++)
	{
		i2c_write(a[k]);
	}
	i2c_stop();
}

/********************************************************************/
void LCD_init()
{
	 ResOLED=0;
	 delay_ms(200);
	 ResOLED=1;
	 delay_ms(200);

	 i2c_start();
	 i2c_write(OLED_Write);															// slave addresses with write
	 i2c_write(0x80);																	// Continuation bit  command
	 i2c_write(0x2a);																	// RE=1
	 i2c_write(0x80);																	// Continuation bit   command	 	 	 	 
	 i2c_write(0x71);																	// Function Selection A
	 i2c_write(0xc0);																	// Continuation bit  data
	 i2c_write(0x00);																	// Disable internal VDD
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x28);																	// RE=0,IS=0
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x08);																	// display OFF

	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x2A);																	// RE=1
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x79);																	// SD=1  OLED command set is enabled
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0xD5);																	// Set Display Clock Divide Ratio/ Oscillator Frequency 
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x70);  
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x78);																	// SD=0   OLED command set is disabled
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x08);																	// 5-dot font width, black/white inverting of cursor disable, 1-line or 2-line display mode
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x06);																	// COM0 -> COM31  SEG99 -> SEG0,
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x72);																	// Function Selection B. Select the character no. of character generator    Select character ROM
	 i2c_write(0xc0);																	// Continuation bit  data
	 i2c_write(0x01);																	// CGRAOM 248 COGRAM 8   Select  ROM A

	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x2a);																	// RE=1
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x79);																	// SD=1  OLED command set is enabled
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0xDA);																	// Set SEG Pins Hardware Configuration  
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x10);																	// Alternative (odd/even) SEG pin configuration, Disable SEG Left/Right remap
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0xDC);																	// Function Selection C  Set VSL & GPIO 
  	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x00);																	// Internal VSL  represents GPIO pin HiZ, input disabled (always read as low)
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x81);																	// Set Contrast Control 
 	 i2c_write(0x80);																	// Continuation bit   command 
	 i2c_write(0x8F);
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0xD9);																	// Set Phase Length 
 	 i2c_write(0x80);																	// Continuation bit   command 
	 i2c_write(0xF1); 
 	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0xDB);																	// Set VCOMH Deselect Level 
 	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x30);																	// 0.83 x VCC

	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x78);																	// SD=0   OLED command set is disabled

	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x28);																	// RE=0,IS=0
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x01);																	// Clear Display
	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x80);																	// Set DDRAM Address


	 i2c_write(0x80);																	// Continuation bit   command
	 i2c_write(0x0C);																	// Display ON
 	 i2c_stop();

	 Write_CGRAM(font);
}

/********************************************************************/
void write_CGROM(int a)
{
int j;

	i2c_start();
	i2c_write(OLED_Write);															// slave addresses with write
	i2c_write(0x80);																	// Continuation bit  command
 	i2c_write(0x80);																	// set 1,st line DDRAM address
	i2c_write(0x40);																	// Continuation bit  data
 	for(j=0;j<40;j++)
	{
		i2c_write(a);a+=1;
	}
	i2c_stop();

	i2c_start();
	i2c_write(OLED_Write);															// slave addresses with write
	i2c_write(0x80);																	// Continuation bit  command
 	i2c_write(0xc0);																	// set 2,st line DDRAM address
	i2c_write(0x40);																	// Continuation bit  data 
	for(j=0;j<40;j++)
	{i2c_write(a);a+=1;
	}
	 i2c_stop();
	delay_ms(100);
}

/********************************************************************/
void shift_to_left(void)
{
int x;

	i2c_start();
	i2c_write(OLED_Write);															// slave addresses with write
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x2a);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x78); 
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x29);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x2a);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x1f);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x28);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x2a);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x1d);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x28);

	for(x=0;x<20;x++)
	{
		i2c_write(0x80);																// Continuation bit  data
		i2c_write(0x1c);          													// shift left
		delay_ms(300);
	}
		 i2c_stop();
}

/********************************************************************/
void shift_to_right(void)
{
int x;

	i2c_start();
	i2c_write(OLED_Write);															// slave addresses with write
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x2a);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x78); 
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x29);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x2a);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x1f);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x28);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x2a);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x1d);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x28);

	for(x=0;x<20;x++)
	{
		i2c_write(0x80);																// Continuation bit  data
		i2c_write(0x18);																// shift right
		delay_ms(300);
	}
	i2c_stop();
}

/********************************************************************/
void horizontal_scroll(void)
{
int i;

	i2c_start();
	i2c_write(OLED_Write);															//slave addresses with write
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x2a);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x78); 
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x29);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x2a);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x1f);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x28);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x2a);
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x1C);																	// 1c
	for(i=0x80;i<0xC0;i++)															// 80/c0
	{
		i2c_write(0x80);																// Continuation bit  command	
		i2c_write(i);
		delay_ms(100);
	}
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x28);
	i2c_stop();
}

/********************************************************************/
void 
LCD_Gotoxy(int x, int y)
{
int	address;

	if(y != 1)
		address = LCD_line_two;
	else
		address=0;

	address+=x-1;
	i2c_start();
	i2c_write(OLED_Write);															// slave addresses with write
	i2c_write(0x80);																	// Continuation bit  command
	i2c_write(0x80|address);														// DDRAM address
	i2c_stop();
}

/********************************************************************/
void 
LCD_Putc(char C)
{
	switch(C)
	{
		case '\f':																		// clear screen
			LCD_Send_byte(0,1);
			delay_ms(2);
			break;
   
		case '\n':																		// new line
			LCD_Gotoxy(1,2);
			break;
   
		case '\b':																		// back space
			LCD_Send_byte(0,0x10);
			break;

   	case '\r':																		// home
			LCD_Send_byte(0,2);
			break;

		default:
			LCD_Send_byte(1,C);
			break;
   }
}

/********************************************************************/
void
LCD_Send_byte(int	CD,int N)
{

	i2c_start();
	i2c_write(OLED_Write);															// slave addresses with write
	if(!CD)
		i2c_write(0x80);																// Continuation bit with command
	else
		i2c_write(0x40);																// Continuation bit with data
	i2c_write(N);																		// write data
	i2c_stop();																			// stop
	
}

/********************************************************************/
