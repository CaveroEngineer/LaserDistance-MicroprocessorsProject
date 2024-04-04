
/* Includes ------------------------------------------------------------------*/
#include "GLCD.h"
#include "TouchPanel.h"
#include <string.h>
#include <Net_Config.h>
#include <stdio.h>

uint8_t messageText[25+1] = {"Esto es una prueba 1000"};

void  screenMessageIP(void);
void  screenMessage(void);


extern LOCALM localm[];                       /* Local Machine Settings      */
#define MY_IP localm[NETIF_ETH].IpAdr


/*******************************************************************************
* Function Name  : squareButton
* Description    : Dibuja un cuadrado en las coordenadas especificadas colocando 
*                  un texto en el centro del recuadro
* Input          : zone: zone struct
*                  text: texto a representar en el cuadro
*                  textColor: color del texto
*                  lineColor: color de la línea
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/


/*******************************************************************************
* Function Name  : screenMain
* Description    : Visualiza la pantalla principal
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/

/*******************************************************************************
* Function Name  : screenToggle
* Description    : Visualiza la pantalla secundaria
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/

/*******************************************************************************
* Function Name  : screenMessage
* Description    : Visualiza la pantalla de mensajes
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/

/*******************************************************************************
* Function Name  : screenMessageIP
* Description    : Visualiza la pantalla de mensajes
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/

/*******************************************************************************
* Function Name  : checkTouchPanel
* Description    : Lee el TouchPanel y almacena las coordenadas si detecta pulsación
* Input          : None
* Output         : Modifica pressedTouchPanel
*                    0 - si no se detecta pulsación
*                    1 - si se detecta pulsación
*                        En este caso se actualizan las coordinadas en la estructura display
* Return         : None
* Attention		  : None
*******************************************************************************/

/*******************************************************************************
* Function Name  : zonePressed
* Description    : Detecta si se ha producido una pulsación en una zona contreta
* Input          : zone: Estructura con la información de la zona
* Output         : Modifica zone->pressed
*                    0 - si no se detecta pulsación en la zona
*                    1 - si se detecta pulsación en la zona
* Return         : 0 - si no se detecta pulsación en la zona
*                  1 - si se detecta pulsación en la zona
* Attention		  : None
*******************************************************************************/

/*******************************************************************************
* Function Name  : zoneNewPressed
* Description    : Detecta si se ha producido el flanco de una nueva pulsación en 
*                  una zona contreta
* Input          : zone: Estructura con la información de la zona
* Output         : Modifica zone->pressed
*                    0 - si no se detecta pulsación en la zona
*                    1 - si se detecta pulsación en la zona
* Return         : 0 - si no se detecta nueva pulsación en la zona
*                  1 - si se detecta una nueva pulsación en la zona
* Attention		  : None
*******************************************************************************/



/*******************************************************************************
* Function Name  : fillRect
* Description    : Rellena de un color determinado el interior de una zona 
* Input          : zone: Estructura con la información de la zona
*                  color: color de relleno
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/


/*******************************************************************************
* Function Name  : updateLEDs
* Description    : Actualiza en función de las variables led1 y led2 la visualización 
*                  de los LEDs de la Mini-DK2 y el color de los cuadros en pantalla
*                  relacionados. 
* Input          : zone: Estructura con la información de la zona
*                  color: color de relleno
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/

/*******************************************************************************
* Function Name  : initScreenStateMachine
* Description    : Inicializa la máquina de estados al primer estado. 
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/


/*******************************************************************************
* Function Name  : screenStateMachine
* Description    : Máquina de estados de la aplicación. 
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/

/*********************************************************************************************************
      END FILE
*********************************************************************************************************/
