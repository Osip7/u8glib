/*
  
  gps.c
  
  ARM Based GPS Tracker 

  Universal 8bit Graphics Library
  
  Copyright (c) 2012, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

*/

#include "u8g_lpc11xx.h"
#include "pq.h"

#define SYS_TICK_PERIOD_IN_MS 10

/*============================================================*/
pq_t pq;

/*============================================================*/
/* LPC11xx UART */

void UARTInit(uint8_t is_48_mhz)
{
  LPC_SYSCON->SYSAHBCLKCTRL |= 1<<16;	/* enable IOCON clock */
  LPC_SYSCON->SYSAHBCLKCTRL |= 1<<12;	/* enable UART clock */
  LPC_SYSCON->UARTCLKDIV = 1;			/* PCLK = Master Clock / 1 */
  
  /*
    LPC1114:
      TXD PIO1_7
      RXD PIO1_6
  */
  LPC_IOCON->PIO1_6 = 1;		/* connect UART to RXD pin */
  LPC_IOCON->PIO1_7 = 1;		/* connect UART to TXD pin */
  
  /*
    12MHz/9600				DLM=0,DLL=71,DIVADDVAL=1,MULVAL=10		<===
    48MHz/9600				DLM=0,DLL=250,DIVADDVAL=1,MULVAL=4
    50MHz/9600				DLM=0,DLL=217,DIVADDVAL=5,MULVAL=10

    12MHz/38400			DLM=0,DLL=16,DIVADDVAL=2,MULVAL=9
    48MHz/38400			DLM=0,DLL=71,DIVADDVAL=1,MULVAL=10
    50MHz/38400			DLM=0,DLL=46,DIVADDVAL=10,MULVAL=13

    12MHz/115200			DLM=0,DLL=4,DIVADDVAL=5,MULVAL=8
    48MHz/115200			DLM=0,DLL=22,DIVADDVAL=2,MULVAL=11
    50MHz/115200			DLM=0,DLL=19,DIVADDVAL=3,MULVAL=7
  */
  LPC_UART->LCR = 3 | 128;		/* 8 data bits, one stop bit, enable divider register */
  if ( is_48_mhz != 0 )
    LPC_UART->DLL = 250 /* dll */;
  else
    LPC_UART->DLL = 71 /* dll */;
  LPC_UART->DLM = 0;
  LPC_UART->LCR = 3;			/* 8 data bits, one stop bit, disable divider register */
  if ( is_48_mhz != 0 )
    LPC_UART->FDR  = (4 /* mulval */ << 4) | 1 /* divaddval */;
  else
    LPC_UART->FDR  = (10 /* mulval */ << 4) | 1 /* divaddval */;
  LPC_UART->IER = 0;			/* no interrupts */
  LPC_UART->FCR = 1;			/* FIFO enable, generate IRQ qith one char in the buffer */
  LPC_UART->MCR = 0;  
  LPC_UART->TER = 1<<7;			/* enable transmit */
  LPC_UART->IER = 1;			/* enable receive data interrupt */
  NVIC_EnableIRQ(UART_IRQn);
}

int UARTIsDataAvailable(void)
{
  if ( ( LPC_UART->LSR & 1 ) != 0 )
    return 1;
  return 0;
}

int UARTReadData(void)
{
  if ( UARTIsDataAvailable() != 0 )
    return LPC_UART->RBR;
  return -1;
}

volatile int16_t uart_data;
volatile int32_t uart_byte_cnt = 0;
volatile int32_t uart_avg_byte_cnt = 0;
void __attribute__ ((interrupt)) UART_Handler(void)
{
  uint32_t iir = LPC_UART->IIR;
  if ( (iir & 1) == 0 )
  {
    while ( ( LPC_UART->LSR & 1 ) != 0 )
    {
      uart_byte_cnt++;
      uart_data = LPC_UART->RBR;
      pq_AddChar(&pq, (uint8_t)uart_data);
      
    }
  }
}


void UARTSendData(int data)
{
  while( (LPC_UART->LSR & (1<<5)) == 0 )
    ;
  LPC_UART->THR = data;
}

void UARTSendStr(const char *str)
{
  while( *str != '\0' )
    UARTSendData( (int)(unsigned char)*str++ );
}

/*========================================================================*/
/* SystemInit */


uint32_t SystemCoreClock;

void SystemInit()
{    
 /* SystemInit() is called by the startup code */
 
  /* init gps parser */  
  pq_Init(&pq);

  /* increase clock speed to max */
#if F_CPU >= 48000000
  lpc11xx_set_irc_48mhz();
#endif
  
  /* according to system_LPC11xx.h it is expected, that the clock freq is set int SystemInit() */
  SystemCoreClock = F_CPU;
  
  /* SysTick is defined in core_cm0.h */
  SysTick->LOAD = (SystemCoreClock/1000UL*(unsigned long)SYS_TICK_PERIOD_IN_MS) - 1;
  SysTick->VAL = 0;
  SysTick->CTRL = 7;   /* enable, generate interrupt, do not divide by 2 */

  /* listen to gps device */
  #if F_CPU >= 48000000
  UARTInit(1);
#else
  UARTInit(0);  
#endif
 
}


/*========================================================================*/
/* SysTick */

volatile uint32_t sys_tick_irq_cnt=0;
volatile uint32_t sys_tick_10s_cnt=0;
volatile uint32_t sys_cnt_10s  = 0;

void __attribute__ ((interrupt)) SysTick_Handler(void)
{
  sys_tick_irq_cnt++;
  
  sys_tick_10s_cnt++;
  if ( sys_tick_10s_cnt >= 1000 )
  {
    /* this will be executed every 10s */
    sys_tick_10s_cnt = 0;
    sys_cnt_10s++;
    uart_avg_byte_cnt = uart_byte_cnt;
    uart_avg_byte_cnt /= 10L;			/* characters per seconds */
    uart_byte_cnt = 0;
  }
}

/*========================================================================*/
/* gps board u8g com procedure */

/*========================================================================*/
/* u8glib com procedure */

#define A0_GPIO	LPC_GPIO0
#define A0_PIN 	11

#define CS_GPIO	LPC_GPIO0
#define CS_PIN 	6

#define RST_GPIO	LPC_GPIO0
#define RST_PIN 	5


uint8_t u8g_com_hw_spi_gps_board_fn(u8g_t *u8g, uint8_t msg, uint8_t arg_val, void *arg_ptr)
{
  switch(msg)
  {
    case U8G_COM_MSG_STOP:
      break;
    
    case U8G_COM_MSG_INIT:

      LPC_SYSCON->SYSAHBCLKCTRL |= 1<<16;	/* enable IOCON clock */

      spi_init(700);

      LPC_IOCON->PIO0_5 = 1<<8;			/* select standard GPIO mode, disable I2C */
      LPC_IOCON->PIO0_6 = 128+64;				/* select GPIO mode */
      LPC_IOCON->R_PIO0_11 = 128+64+1;			/* select GPIO mode */
        
      A0_GPIO->DIR |= 1 << A0_PIN;
      CS_GPIO->DIR |= 1 << CS_PIN;
      RST_GPIO->DIR |= 1 << RST_PIN;
      
      u8g_MicroDelay();      
      break;
    
    case U8G_COM_MSG_ADDRESS:                     /* define cmd (arg_val = 0) or data mode (arg_val = 1) */
      u8g_10MicroDelay();
      if ( arg_val != 0 )
	  A0_GPIO->DATA |= 1 << A0_PIN;
      else
	  A0_GPIO->DATA &= ~(1 << A0_PIN);
      u8g_10MicroDelay();
     break;

    case U8G_COM_MSG_CHIP_SELECT:
      if ( arg_val == 0 )
      {
        /* disable */
	uint8_t i;
	
	/* this delay is required to avoid that the display is switched off too early --> DOGS102 with LPC1114 */
	for( i = 0; i < 5; i++ )
	  u8g_10MicroDelay();
	CS_GPIO->DATA |= 1 << CS_PIN;
	u8g_MicroDelay();
      }
      else
      {
        /* enable */
	CS_GPIO->DATA &= ~(1 << CS_PIN);
	u8g_MicroDelay();
      }
      break;
      
    case U8G_COM_MSG_RESET:
      if ( arg_val != 0 )
	  RST_GPIO->DATA |= 1 << RST_PIN;
      else
	  RST_GPIO->DATA &= ~(1 << RST_PIN);
      u8g_10MicroDelay();
      break;
      
    case U8G_COM_MSG_WRITE_BYTE:
      spi_out(arg_val);
      u8g_MicroDelay();
      break;
    
    case U8G_COM_MSG_WRITE_SEQ:
    case U8G_COM_MSG_WRITE_SEQ_P:
      {
        register uint8_t *ptr = arg_ptr;
        while( arg_val > 0 )
        {
          spi_out(*ptr++);
          arg_val--;
        }
      }
      break;
  }
  return 1;
}

/*========================================================================*/
/* main */

#define LED_GPIO	LPC_GPIO1
#define LED_PIN 8			

u8g_t u8g;


void draw_crb(void)
{
  uint16_t crb_pos, end_pos, disp_pos;
  u8g_SetFont(&u8g, u8g_font_4x6r);
  
  crb_pos = pq.crb.start; 
  end_pos = pq.crb.start; 
  disp_pos = 0;
  
  while( disp_pos < CRB_LEN )
  {
    if ( (disp_pos >> 4) >= 9 )
      break;
    
    u8g_DrawGlyph(&u8g, 5*(disp_pos & 15),  6+6*(disp_pos >> 4), pq.crb.buf[crb_pos]);
    crb_pos++;
    if ( crb_pos >= CRB_LEN )
    {
      crb_pos = 0;
    }
    if ( crb_pos == end_pos )
      break;
    disp_pos++;
  }
}

void draw(void)
{
  char buf[4] = "[ ]";
  buf[1] = uart_data;
  u8g_SetFont(&u8g, u8g_font_4x6r);
  
  u8g_DrawStr(&u8g,  0, 6, buf);
  u8g_DrawStr(&u8g,  0, 6*2, "10s:");
  u8g_DrawStr(&u8g,  38, 6*2, u8g_u16toa(sys_cnt_10s, 5));
  u8g_DrawStr(&u8g,  0, 6*3, "UART RX:");
  u8g_DrawStr(&u8g,  38, 6*3, u8g_u16toa(uart_avg_byte_cnt, 5));
  
  u8g_DrawStr(&u8g,  0, 6*4, "UART Buf:");
  u8g_DrawStr(&u8g,  38, 6*4, u8g_u16toa(pq.crb.start, 4));
  u8g_DrawStr(&u8g,  60, 6*4, u8g_u16toa(pq.crb.end, 4));
  
  u8g_DrawStr(&u8g,  0, 6*5, "Msg Cnt:");
  u8g_DrawStr(&u8g,  38, 6*5, u8g_u16toa(pq.processed_sentences, 5));
  
  u8g_DrawStr(&u8g,  0, 6*6, "RMC Cnt:");
  u8g_DrawStr(&u8g,  38, 6*6, u8g_u16toa(pq.processed_gprmc, 4));
  u8g_DrawStr(&u8g,  56, 6*6, u8g_u16toa(pq.invalid_gprmc, 4));
  u8g_DrawStr(&u8g,  74, 6*6, u8g_u16toa(pq.parser_error_gprmc, 4));

  u8g_DrawStr(&u8g,  0, 6*7, "GGA Cnt:");
  u8g_DrawStr(&u8g,  38, 6*7, u8g_u16toa(pq.processed_gpgga, 4));
  u8g_DrawStr(&u8g,  56, 6*7, u8g_u16toa(pq.invalid_gpgga, 4));
  u8g_DrawStr(&u8g,  74, 6*7, u8g_u16toa(pq.parser_error_gpgga, 4));
  
  u8g_DrawStr(&u8g,  0, 6*8, "Sat:");
  u8g_DrawStr(&u8g,  38, 6*8, u8g_u8toa(pq.sat_cnt, 3));
  u8g_DrawStr(&u8g,  55, 6*8, "Quality:");
  u8g_DrawStr(&u8g,  90, 6*8, u8g_u8toa(pq.gps_quality, 2));
  
  u8g_DrawStr(&u8g,  0, 6*9, "Unknown:");
  u8g_DrawStr(&u8g,  38, 6*9, pq.last_unknown_msg);

}

void main()
{
  uint8_t c = 0;
  uint8_t y = 0;
  volatile uint32_t i, cnt = 100000;
  LED_GPIO->DIR |= 1 << LED_PIN;	  



  //spi_init(700);
  
  //u8g_InitComFn(&u8g, &u8g_dev_ssd1351_128x128_hicolor_hw_spi, u8g_com_hw_spi_fn);
  //u8g_InitComFn(&u8g, &u8g_dev_ssd1351_128x128_332_hw_spi, u8g_com_hw_spi_fn);
  //u8g_InitComFn(&u8g, &u8g_dev_ssd1325_nhd27oled_bw_hw_spi, u8g_com_hw_spi_fn);
  u8g_InitComFn(&u8g, &u8g_dev_uc1701_dogs102_hw_spi, u8g_com_hw_spi_gps_board_fn);
  u8g_SetRot180(&u8g);

  for(;;)
  {
    u8g_FirstPage(&u8g);
    do
    {
      //draw();
      draw_crb();
    } while ( u8g_NextPage(&u8g) );
    for( y = 0; y < 20; y++ )
    {
      u8g_Delay(100);
      pq_ParseSentence(&pq);
    }
  }
  
  /*
  while (1)
  {
    for( i = 0; i < cnt; i++ ) 
      ;
    LED_GPIO->DATA |= 1 << LED_PIN;
    for( i = 0; i < cnt; i++ )
      ;
    LED_GPIO->DATA &= ~(1 << LED_PIN);
  }
  */
  
  while (1)
  {
    for( i = 0; i < cnt; i++ ) 
      spi_out(0x0a0);
    LED_GPIO->DATA |= 1 << LED_PIN;
    for( i = 0; i < cnt; i++ )
      spi_out(0x0a0);
    LED_GPIO->DATA &= ~(1 << LED_PIN);
  }
}
