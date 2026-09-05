#include "project.h"
#include "drv_spi.h"
#include "binary.h"
#include "defines.h"

#ifdef SOFTSPI_4WIRE

void spi_init(void)
{    
	// spi port inits

		GPIO_InitTypeDef  GPIO_InitStructure;
	
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStructure.GPIO_Pin = SPI_MOSI_PIN;
	GPIO_Init(SPI_MOSI_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = SPI_CLK_PIN;
	GPIO_Init(SPI_CLK_PORT, &GPIO_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = SPI_SS_PIN;
	GPIO_Init(SPI_SS_PORT, &GPIO_InitStructure);
	
#ifdef RADIO_XN297_CE_PIN
	GPIO_InitStructure.GPIO_Pin = RADIO_XN297_CE_PIN;
	GPIO_Init(RADIO_XN297_CE_PORT, &GPIO_InitStructure);
	// CE high to enable radio
	RADIO_XN297_CE_PORT->BSRR = RADIO_XN297_CE_PIN;
#endif
	
	// CRITICAL FIX: STM32F0 GPIO defaults to ANALOG mode after reset.
	// "input by default" is NOT true - MISO must be explicitly configured as input!
	// Otherwise IDR always reads 0 and XN297 MISO data is never captured.
#ifdef SPI_MISO_PIN
	GPIO_InitTypeDef miso_init_struct;
	miso_init_struct.GPIO_Pin = SPI_MISO_PIN;
	miso_init_struct.GPIO_Mode = GPIO_Mode_IN;
	miso_init_struct.GPIO_OType = GPIO_OType_PP;
	miso_init_struct.GPIO_PuPd = GPIO_PuPd_NOPULL; // external pull-up R19 already on board
	miso_init_struct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(SPI_MISO_PORT, &miso_init_struct);
#endif
	
	spi_csoff();

}


#define gpioset( port , pin) port->BSRR = pin
#define gpioreset( port , pin) port->BRR = pin

#define MOSIHIGH gpioset( SPI_MOSI_PORT, SPI_MOSI_PIN)
#define MOSILOW gpioreset( SPI_MOSI_PORT, SPI_MOSI_PIN);
#define SCKHIGH gpioset( SPI_CLK_PORT, SPI_CLK_PIN);
#define SCKLOW gpioreset( SPI_CLK_PORT, SPI_CLK_PIN);

//#define READMISO (GPIO_ReadInputDataBit(SPI_MISO_PORT, SPI_MISO_PIN) )
#define READMISO (SPI_MISO_PORT->IDR & SPI_MISO_PIN)

#pragma push

#pragma Otime
#pragma O2

// 64MHz overclock slows software spi - XN297 max spi clock ~10MHz
#define SPI_NOP __asm__ volatile("nop")
#define SPI_CLK_HALF { SPI_NOP; SPI_NOP; SPI_NOP; SPI_NOP; SPI_NOP; SPI_NOP; SPI_NOP; SPI_NOP; }

void spi_cson( )
{
	SPI_SS_PORT->BRR = SPI_SS_PIN;
}

void spi_csoff( )
{
	SPI_SS_PORT->BSRR = SPI_SS_PIN;
}


void spi_sendbyte ( int data)
{
for ( int i =7 ; i >=0 ; i--)
	{
		if (  (data>>i)&1  ) 
		{
			MOSIHIGH;
		}
		else 
		{
			MOSILOW;
		}
	
		SCKHIGH;
		SPI_CLK_HALF;
		SCKLOW;
		SPI_CLK_HALF;
	}
}


int spi_sendrecvbyte2( int data)
{ 
	int recv = 0;
	for ( int i =7 ; i >=0 ; i--)
	{
		if ( (data) & (1<<7)  ) 
		{
			MOSIHIGH;
		}
		else 
		{
			MOSILOW;
		}
		SCKHIGH;
		SPI_CLK_HALF;
		data = data<<1;
		if ( READMISO ) recv= recv|(1<<7);
		recv = recv<<1;
		SCKLOW;
		SPI_CLK_HALF;
	}	
	  recv = recv>>8;
    return recv;
}


 int spi_sendrecvbyte( int data)
{ int recv = 0;

	for ( int i = 7 ; i >=0 ; i--)
	{
		recv = recv<<1;
		if ( (data) & (1<<7)  ) 
		{
			MOSIHIGH;
		}
		else 
		{
			MOSILOW;
		}
		
		data = data<<1;
		
		SCKHIGH;
		SPI_CLK_HALF;
		
		if ( READMISO ) recv= recv|1;

		SCKLOW;
		SPI_CLK_HALF;
		
	}	

    return recv;
}


 int spi_sendzerorecvbyte( )
{ int recv = 0;
	MOSILOW;

	for ( int i = 7 ; i >=0 ; i--)
	{
		recv = recv<<1;
		
		SCKHIGH;
		SPI_CLK_HALF;
		
		if ( READMISO ) recv= recv|1;

		SCKLOW;
		SPI_CLK_HALF;
		
	}	

    return recv;
}


#pragma pop

#endif