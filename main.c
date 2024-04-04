/**--------------File Info---------------------------------------------------------------------------------
** File name:               main.c
** Descriptions:            Example of simple state machine implemented menu with an HTTP server 
**                          and FTP server with data in a SD card 
**
**--------------------------------------------------------------------------------------------------------
** Created by:              JPM (UAH)
** Created date:            2017-08-17
** Version:                 v1.0
** Descriptions:            The original version
**
**--------------------------------------------------------------------------------------------------------
** Modified by:             
** Modified date:           
** Version:                 
** Descriptions:            
**
*********************************************************************************************************/

/* Includes ------------------------------------------------------------------*/

#include <stdio.h>
#include <RTL.h>
#include <Net_Config.h>
#include <LPC17xx.h>                    /* LPC17xx definitions               */
#include <GLCD.h>

#include <TouchPanel.h>
#include <menu.h>


extern   U32 CheckMedia (void);



/*--------------------------- init ------------------------------------------*/

/*******************************************************************************
* Function Name  : init
* Description    : Initialize every subsystem
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/

/*******************************************************************************
* Function Name  : timer_poll
* Description    : Call timer_tick() if every 100ms (aprox) 
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
static void timer_poll () {
  /* System tick timer running in poll mode */

  if (SysTick->CTRL & 0x10000) {
    /* Timer tick every 100 ms */
    timer_tick ();
  }
}

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
