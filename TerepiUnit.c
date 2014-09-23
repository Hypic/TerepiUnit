/******************************************************************************************
*	  Filename:   TerepiUnit.c																					*
*******************************************************************************************
*	  Author:     SW: Peter Szilagyi (SMSP HyPIC)  														*
*	  Company:    Smart Micro Solution Projects, Inc. (SMSP, Inc.) © All Right Reserved!	*
*	  Revision:   Rev 1.00																						*
*	  Date:       2014.09.15.																					*
*																														*
*	  Vizóra terepi vezérlõje																					*
*																														*
*	  MCU:		PIC18F26K80																						*
*																														*
*	  2 analog input 1 Bluetooth serial I/O																*
*																														*
*	  MPLAB 8.92 Compiled using CCS    PIC_C compiler v.5.024										*
*																														*
******************************************************************************************/
#include <18F26K80.h>


#device	adc=10, *=16

#FUSES	INTRC_HP,NOPLLEN,NOWDT,STVREN,NOPUT,NOFCMEN,NOIESO,BORV30,NOPROTECT,NOCPD,NODEBUG,MCLR

#use		delay(int=16MHz,restart_wdt)
#use		RS232(baud=9600,UART1)
#use		SPI(FORCE_HW,SPI1,MODE=0)

//***************************************************************************************

//#define								RTC_Settings								// dátum és idõ beállításkor
#define								LCD												// OLED kijelzõ csatlakoztatva

const	float	Null		=			1344.0;											// 0C°-nál ennyi az ADC értéke
const	float	TK			=			0.270;											// ennyi a TK/C°
const	float	VK			=			236.67;											// voltage koefficient
const	float	Beta		=			3950.0;

const	float	LSB		=			3300.0/1024.0;									// A/D felbontás 2048mV/10bit
const float	Rserial	=			10000.0;											// soros áramgenerátor ellenállás (felsõ ellenállás ohm)
const float	Vref		=			3300.0;											// mérõfeszültség -> feszültségosztóhoz (3300mV)

const	long	Homeres	=			0x7D00;											// hõmérések kezdete (0x300db)
const	long	Atlag		=			10;												// áltlagolások száma

const	int	TmpDiode	=			0x1D;												// hõmérõ dióda nyitófesz mérése
const	int	Vdd		=			0x1E;												// tápfesz mérés
const	int	BandGap	=			0x1F;												// 1024mV reference


#byte			PORTA		=		0x0F80
#byte			PORTB		=		0x0F81
#byte			PORTC		=		0x0F82
#byte			PORTE		=		0x0F84

#byte			LATA		=		0x0F89
#byte			LATB		=		0x0F8A
#byte			LATC		=		0x0F8B

#byte			RCREG1	=		0x0FAE
#byte			CTMUICON	=		0x0F53
#byte			CTMUCONH	=		0x0F55
#byte			CTMUCONL	=		0x0F54


#bit			LED		=		LATC.0
#bit			CS			=		LATA.5
#bit			RF			=		LATC.1
#bit			DAT		=		PORTC.2
#bit			BtReset	=		LATB.4
#bit			BtMode	=		LATB.3
#bit			Impi		=		PORTB.0
#bit			Seci		=		PORTB.1
#bit			Tamper	=		PORTB.2
#bit			Out1		=		LATA.2
#bit			Out2		=		LATA.3
#bit			ResOLED	=		LATB.5
#bit			Settings	=		PORTB.7

#zero_RAM
#case

//***************************************************************************************

#define		WREN				0x06													// Write enable
#define		WRDI				0x04													// Write disable
#define		RDSR				0x05													// Read Status Register
#define		WRSR				0x01													// Write Status Register
#define		READ				0x03													// Read memory data
#define		WRITE				0x02													// Write memory data
#define		RDPC				0x13													// Read Processor Companion
#define		WRPC				0x12													// Write Processor Companion

#define		Clear				LCD_Putc('\f')
#define		First				LCD_Gotoxy(1,1)
#define		Second			LCD_Gotoxy(1,2)
#define		RTC_En			CS=0;spi_write(WREN);CS=1

#define		MeasTime			1														// mérési intervallum (sec)
#define		Accu				0														// akkumulátor analog bemenet
#define		Therm				1														// thermistor bemenet

#define		On					1
#define		Off				0



//***************************************************************************************

long			Celsius;
float			Temp,TempLCD;

long			Cels,V_Batt,Tav,TempAdd,Tadc;
long			TempAddr		=		Homeres;											// Temp RAM kezdõcíme

signed	int	TempByte;
int			i,j=0;
int			Meres=3;																	// mérést idõzítõ számláló (sec)
int			Buff;
int			Yr=0x14;
int			Mnt=0x08;
int			Dat=0x22;
int			Dow=6;
int			Hrs=0x16;
int			Min=0x37;
int			Sec=0;

int			Y=0x14;
int			Mo=0x09;
int			D=0x12;
int			W=6;
int			H=0x01;
int			Mi=0x30;
int			S=0;
int			Z=0;

int			RxBuff[64];
int			TxBuff[64];

short			Rdy,R=0;


static	char			Text1[]		=	{"Egyes text "};
static	char			Text2[]		=	{"Kettes text "};
static	char			Text3[]		=	{"Default text "};

static	char			Days[7][5]	=	{"Vasar","Hetfo","Kedd ","Szer ","Csut ","Pent ","Szom "};

//***************************************************************************************

#include	<math.h>
#include	<i2c_oled.c>
//#include	<Card.c>

//************************  Prototypes   ************************************************

void		PICinit			();
void		TxFlush			();
void		RxFlush			();
void		Set_Serial		();
void		Read_Time		();
void		Set_Time			();
void		Send_Byte_PC	(int,int);
int		Read_Byte_PC	(int);
float 	Homero			(float);
void		Write_RAM		(long,int);
long		Read_EvntCnt	();
void		Write_EvntCnt	(long);

//*************************************   Interrupts  ************************************************
#int_ext1
void	RTC_Sec()
{
float	Buff;
long 	EC;																				// Event Counter
int	Buf;

	LED^=1;
	Tav++;
#ifdef	LCD
	if(Tav>=Atlag)
	{
		if(j<5)
			j++;																			// szöveg kiíratás pointer
		Tav=0;
		Celsius=Celsius/Atlag;
		Buff=(float)Celsius;
		TempLCD=(Null-Buff)*TK;
		printf("Temp: %3.1g",TempLCD);											// send via Bluetooth
		putc(0x0A);
		Celsius=0;
	}
#endif

	set_adc_channel(Accu);															// akksi mérés
	delay_us(10);
	V_Batt=read_adc();

	set_adc_channel(TmpDiode);														// PIC hõmérséklet mérés
	delay_us(100);
	Tadc=read_adc();
	Celsius+=read_adc();
	Rdy=1;

	switch(j)
	{
		
		case	0:
					First;
					printf(LCD_Putc,"Tamper counter:");
					Second;
					printf(LCD_Putc,"T:%5.0w",Read_EvntCnt());				// kiírjuk az Event Counter értékét
					break;
		case	1:
					First;
					printf(LCD_Putc,"Accumulator Vin:");
					Second;
					printf(LCD_Putc,"%3.1gV    ",V_Batt/VK);					// kiírjuk az akksi feszültségét
					break;
		case	2:
					First;
					printf(LCD_Putc,"RTC calibration:");
					Second;
					printf(LCD_Putc,"K:%X ",Read_Byte_PC(0x01));				// kiírjuk az óra kompenzáció értékét
					break;
		case	3:
					First;
					printf(LCD_Putc,"PIC inner temp: ");
					Second;
					printf(LCD_Putc,"%3.1g ",TempLCD);							// kiírjuk a PIC hõmérsékletét
					LCD_Putc('C');
					break;
		default:
					First;
					LCD_Putc("20");													// cetury
					Read_Time();

					printf(LCD_Putc,"%X",Y);										// year
					LCD_Putc('.');
					printf(LCD_Putc,"%X",Mo);										// month
					LCD_Putc('.');
					printf(LCD_Putc,"%X",D);										// date
					LCD_Putc(' ');														// space

					for(i=0;i<5;i++)
					{
						LCD_Putc(Days[W-1][i]);										// day of week
					}
					Second;
					printf(LCD_Putc,"%X",H);										// hours
					LCD_Putc(':');
					printf(LCD_Putc,"%X",Mi);										// minutes
					LCD_Putc(':');
					printf(LCD_Putc,"%X  ",S);										// seconds
					printf(LCD_Putc,"%3.1g",TempLCD);							// kiírjuk a PIC hõmérsékletét
					LCD_Putc('C');
					printf("Temp: %3.1g ",TempLCD);								// BT
					printf("C\n");														// Celsius
	}
}

//***************************************************************************************
#int_rda
void RS232_Bt()
{
static int	RxIndx;																	// Rx buffer index

	Buff=RCREG1;
	Buff-=0x30;
	
}

//***************************************************************************************
#int_timer0
void Timer0()
{
	
	LED^=1;
//	RF^=1;
	Out1^=1;
	if(Out1)
		Out2=0;
	else
		Out2=1;
} 

//***************************************************************************************



//***************************************************************************************
void
main()
{


	PICinit();
	
look:	goto	look;

}

//***************************************************************************************
void
TxFlush()
{
int	i;

	for(i=0;i<64;i++)
		TxBuff[i]=0;
}

//***************************************************************************************
void
RxFlush()
{
int	i;

	for(i=0;i<64;i++)
		RxBuff[i]=0;
}

//***************************************************************************************
void
Set_Serial()
{

	RTC_En;
	CS=0;
	spi_write(WRPC);
	spi_write(0x10);
	for(i=0;i<8;i++)
	{
		spi_write(i+1);
	}
	CS=1;
}

//***************************************************************************************
void
Read_Time()
{

	Send_Byte_PC(0,1);																// set "R" to copy timekeeping registers
	CS=0;
	spi_write(RDPC);
	spi_write(0x02);																	// timekeeping address 0x02
	S=spi_read(0);
	Mi=spi_read(0);
	H=spi_read(0);
	W=spi_read(0);
	D=spi_read(0);
	Mo=spi_read(0);
	Y=spi_read(0);
	CS=1;
	Send_Byte_PC(0,0);
}

//***************************************************************************************
void
Set_Time()
{

	RTC_En;
	Send_Byte_PC(0,2);																//  set "W" bit
	RTC_En;
	CS=0;
	spi_write(WRPC);
	spi_write(0x02);
	for(i=0;i<8;i++)
	{
		spi_write(S);
		spi_write(Mi);
		spi_write(H);
		spi_write(W);
		spi_write(D);
		spi_write(Mo);
		spi_write(Y);
	}
	CS=1;
	Send_Byte_PC(0,0);
}

//***************************************************************************************
void
Send_Byte_PC(int Addr, int Data)
{
	RTC_En;
	CS=0;
	spi_write(WRPC);
	spi_write(Addr);
	spi_write(Data);
	CS=1;
	
}

//***************************************************************************************
int
Read_Byte_PC(int Addr)
{
int	Buff;

	CS=0;
	spi_write(RDPC);
	spi_write(Addr);
	Buff=spi_read(0);
	CS=1;
	return(Buff);

	
}

//***************************************************************************************
float Homero(float THERM)															// thermistor által mért hõmérséklet számítása
{
float	QQ;
float	TEMP;

	QQ=(Null-THERM)*TK;

/*
																							// Steinhart-Hart equation
																							// homeres: K_actual=1/(1/Beta(ln(R_actual/R_20)+0.003354))
	THERM=THERM*LSB;																	// beskálázzuk a mért értéket (mV)
	THERM=THERM*Rserial/(Vref-THERM) ;
	TEMP=3.354E-3+(log(1E-4*THERM)*(1/Beta));									// (1/Beta*ln(Rtherm*1/Rserial))+(1/298.15Kelvin)
	QQ=1/TEMP-273.15;																	// 1/TEMP-273.15 Kelvin
	Cels=(long)1/TEMP-273.15;
*/
	return(QQ);
}	

//***************************************************************************************
int
Read_RAM(long Addr)
{
int	Buff;

	CS=0;
	spi_write(WRITE);
	spi_write(Addr>>8);
	spi_write(Addr);
	Buff=spi_read(0);
	CS=1;	
	
	
	
}

//***************************************************************************************
void
Write_RAM(long Addr,int Data)
{
	RTC_En;
	CS=0;
	spi_write(READ);
	spi_write(Addr>>8);
	spi_write(Addr);
	spi_write(Data);
	CS=1;
	
}

//***************************************************************************************
long
Read_EvntCnt()
{
int	Buff;
long	EC;

	Send_Byte_PC(0x0D,0x0B);														// snapshot from Event Counter
	CS=0;
	spi_write(RDPC);																	// olvasni akarjuk az
	spi_write(0x0E);																	// Event Counter-t
	Buff=spi_read(0);																	// alsó byte
	EC  =spi_read(0);																	// felsõ byte
	CS=1;
	EC<<=8;
	EC+=Buff;																			// összepasszítjuk egy longba
	return(EC);
}

//***************************************************************************************
void
Write_EvntCnt(long EC)
{

	Send_Byte_PC(0x0D,0x04);														// kiadjuk az írási parancsot WC=1
	RTC_En;
	CS=0;
	spi_write(WRPC);																	// írjuk a registereket
	spi_write(0x0E);																	// Event Counter low byte
	spi_write(EC);
	spi_write(EC>>8);																	// Event Counter high byte
	CS=1;
	Send_Byte_PC(0x0D,0x03);														// engedélyezzük a számlálást WC=0
}

//***************************************************************************************


//***************************************************************************************


//***************************************************************************************


//***************************************************************************************
void
PICinit()
{
	port_b_pullups(0xFF);
	set_tris_a(0b11010011);
	set_tris_b(0b11100111);
	set_tris_c(0b10010100);
	set_tris_e(0xFF);
	CTMUICON=0x01;																			// CTMU On
	CTMUCONH=0x80;
	CTMUCONL=0x01;
	CS=1;
	BtReset=0;																				// reset Bluetooth module
	BtMode=0;
	RF=Off;
	delay_ms(100);
	BtReset=1;
	setup_ccp1(CCP_OFF);
	setup_ccp2(CCP_OFF);
	setup_comparator(NC_NC_NC_NC);
	setup_adc(ADC_CLOCK_DIV_16|ADC_TAD_MUL_16);
	setup_adc_ports(sAN0|sAN1|VSS_2V048);
	set_adc_channel(0);
	read_adc(ADC_START_ONLY);
	delay_ms(100);

#ifdef RTC_Settings
	Write_EvntCnt(12345);															// Event Counter beállítása
	Send_Byte_PC(0x00,0x04);														// engedélyezzük az óra kalibrálását
	Send_Byte_PC(0x01,0x24);														// óra kalibrációs érték írása
	
	Send_Byte_PC(0x18,0x0C);														// 1Hz square wave output
	Send_Byte_PC(0x00,0x00);														// start oscillator

	while(!Settings);
	Set_Time();
#endif

	Send_Byte_PC(0x18,0x0C);														// 1Hz square wave output
	Send_Byte_PC(0x00,0x00);														// start oscillator

#ifdef	LCD
	LCD_init();
	Clear;
#endif
	setup_timer_0(T0_INTERNAL|T0_DIV_256);
	clear_interrupt(INT_RDA);
	enable_interrupts(INT_RDA);
	enable_interrupts(INT_EXT1_L2H);
//	enable_interrupts(INT_TIMER0);
	enable_interrupts(GLOBAL);
}

//***************************************************************************************


