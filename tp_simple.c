#include <LPC17xx.H>
#include <glcd.h>
#include <TouchPanel.h>
#include <stdio.h>
#include <string.h>
#include <uart.h>
#include <Math.h>
#include <Net_Config.h>
#include <RTL.h>
#include "tp_simple.h"
#include "VL53L1X.h"

#define Fpclk 25e6	// Fcpu/4 (defecto después del reset)
#define Tpwm 1e-3	// Perido de la señal PWM (1ms)
#define prescaler (25-1) // prescaler para contar a 1 us
#define Desplazamiento 205 //Cada vez que se mueve el encoder se incrementa 18º el angulo del motor
#define F_cpu 100e6		// Defecto Keil (xtal=12Mhz)
#define F_pclk (F_cpu/4) // Defecto despues del reset
#define F_muestreo 8000 // Fs=100Hz (Cada 10ms se toma una muestra del canal 0)
#define pi 3.14159
#define F_out 1000
#define N_muestras 16000
#define V_refp 3.3
//Alarma
#define ON 1
#define OFF 0

//Variables microfono-Altavoz
uint8_t muestras[50];
uint8_t muestras_voz[N_muestras];			// Array para guardar las muestras de un ciclo de un seno
uint16_t indice_muestra=0;
uint16_t a=0;
uint16_t incrementos=0;
int modo;
int MicrofonoActivado;
//Variables I2C
uint16_t last_TC;
uint16_t distancia;
//Variables UART
char buffer[30];		// Buffer de recepción de 30 caracteres
char NombreUsuario[30];
char buffer_tx[90];		// Buffer de recepción de 30 caracteres
char *ptr_rx;			// puntero de recepción
char rx_completa;		// Flag de recepción de cadena que se activa a "1" al recibir la tecla return CR(ASCII=13)
char *ptr_tx0;			// puntero de transmisión
char *ptr_tx3;			// puntero de transmisión
char tx_completa;		// Flag de transmisión de cadena que se activa al transmitir el caracter null (fin de cadena)
char fin=0;
int mostrar_UART=0;
//Menu UART
#define MENU_INICIAL 1
#define MODOFUNC 2
#define MODO 3
#define TX_ENVIADO 4
#define RX_RECIBIDO 5
#define MODO_SELECCIONADO 6
#define ESCRITURA_MODO_SELECCIONADO 7
#define ESPERA_ACCION 8
#define NOMBRE 11
#define DATOS 13
#define MOSTRAR_DATOS 14
// Defines de MODO DE FUNCIONAMIENTO
#define ESTADO_AUTOMATICO 9
#define ESTADO_MANUAL 10
#define ESTADO_CONFIGURACION 12


int contadorAutomatico=0;
int estadouart=MENU_INICIAL;
int siguiente_estado;
int estado_anterior;
int MostrarDatos=0;

//State Chart
#define IDLE 1
#define MANUAL 2
#define AUTOMATICO 3
#define CONFIGURACION 4

int MedidasPeoriodicas=OFF;
int statechart=IDLE;
int flagISP=0, flagKey1=0, ContadorFlag=0, ContadorReset=0;
int flagPulsacion=OFF, ContadorDeFlagInactivo=0;
//Alarma
int AlarmaActivada=OFF;

/* Estructura que define una zona de la pantalla */
struct t_screenZone
{
  uint16_t x;         
	uint16_t y;
	uint16_t size_x;
	uint16_t size_y;
	uint8_t  pressed;
};

/* Definicion de las diferentes zonas de la pantalla */
#define ZONA2 2
#define ZONA3 3
struct t_screenZone zone_1 = { 20,  20, 200,  50, 0}; /* Mensaje "Angulo Motor "    */
struct t_screenZone zone_2 = { 20,  80, 200,  50, 0}; /* Mensaje "Umbral Temperatura"       */
struct t_screenZone zone_3 = { 20, 140, 200,  50, 0}; /* Mensaje "Umbral Distancia"              */
struct t_screenZone zone_4 = { 20, 200, 200,  50, 0}; /* Mensaje "Brillo TFT"             */
struct t_screenZone zone_5 = { 40, 260,  60,  60, 0}; /* Botón  Aumentar con flanco   */
struct t_screenZone zone_6 = {140, 260,  60,  60, 0}; /* Botón  Disminuir con flanco   */
/* Flag que indica si se detecta una pulsación válida */
uint8_t pressedTouchPanel = 0;
uint8_t i;
#define VALORTEMP 70.0
#define VALORDIST 2.0
/* Variables con los datos del programa */
int seleccion=0; //Variable que se encarga de la seleccion de modo
int16_t  AnguloMotor=0;//Variable que almacena el angulo del motor
float UmbralTemp=VALORTEMP; //Variable que almacena el umbral de temperatura
float UmbralDistancia=VALORDIST; //Variable que almacena el umbral de distancia
float Temperatura=0.0; //Variable que almacena la temperatura
float DistanciaMedida=0.0; //Variable que almacena la distancia
uint16_t brilloTFT=0; //Variable que almacena el brillo
int16_t NumPulsos = 0, pulsador=0; 
int secuencia[8]={1,3,2,6,4,12,8,16};//Secuencia del motor
int Dir, Encoder, ADC_int=0;
int NumPulsosAutomatico=Desplazamiento*20;

/* Variable temporal donde almacenar cadenas de caracteres */
char texto[25];

/*******************************************************************************
* Function Name  : ConfigGPIO
* Description    : Configura los pines de salida y entrada de nuestro programa
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void ConfigGPIO(void){
  LPC_GPIO0->FIODIR|=(1<<27)|(1<<28)|(1<<29)|(1<<30);// Salidas del motor P2.2, P2.3, P2.4 y P2.5
	LPC_PINCON->PINSEL4|=(0x01<<20); //Configuro P2.10 como entrada de interrupcion EXT0, boton ISP
	LPC_PINCON->PINSEL4|=(0x01<<22); //Configuro P2.11 como entrada de interrupcion EXT1, boton Key 1
	LPC_SC->EXTMODE|=(0<<0); //EXT0 activa por sensibilidad de nivel, en este caso nivel de subida 
	LPC_SC->EXTPOLAR|=(0<<0); //EXT0 activa a nivel bajo
	LPC_SC->EXTMODE|=(0<<1); //EXT1 activa por sensibilidad de nivel 
	LPC_SC->EXTPOLAR|=(0<<1); //EXT1 activa a nivel bajo
	//Borro los flags de inicio de EINT1-3
	LPC_SC->EXTINT|=(0xF);
	//Hacemos un Clear de las peticiones de interrupcion
	NVIC_ClearPendingIRQ(EINT0_IRQn);
	NVIC_ClearPendingIRQ(EINT1_IRQn);
	//Habilitación de las interrupciones
  NVIC_EnableIRQ(EINT0_IRQn);	// Habilitar interrupción EINT0
  NVIC_EnableIRQ(EINT1_IRQn);	// Habilitar interrupción EINT1
	//Prioridades de las interrupciones
	NVIC_SetPriorityGrouping(3);
	//Timer 1
	NVIC_SetPriority(TIMER1_IRQn,((2<<6)+0));  // Prioridad Interrupción 0  
	NVIC_SetPriority(TIMER3_IRQn,((2<<6)+0));  // Prioridad Interrupción 0   
	//Configura la interrupción del TIMER 2 (captura)
	NVIC_SetPriority(TIMER2_IRQn, ((2<<6)+0));
	//Configura la interrupción del ADC y el TIMER 0
	NVIC_SetPriority(ADC_IRQn,((2<<6)+2));					
 	NVIC_SetPriority(TIMER0_IRQn,((1<<6)+0));  
	//Configura la interrupción del SysTick
	NVIC_SetPriority(SysTick_IRQn, ((0<<6)+0));
	//Configura la interrupción de las EINTx
	NVIC_SetPriority(EINT0_IRQn, ((0<<6)+1));		
	NVIC_SetPriority(EINT1_IRQn, ((0<<6)+1));			
}
/*******************************************************************************
* Function Name  : Config_SysTick
* Description    : Configura el SysTick para que interrumpa cada 0,1s
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void Configura_SysTick(void){
	SysTick->LOAD=99999; //Configuramos para que se introduzca interrupcion cada 1ms
	SysTick->VAL=0; //Valor inicial=0
	SysTick->CTRL|=0x7;//Activamos el reloj y el tickin
}
static void timer_poll () {
  /* System tick timer running in poll mode */
  if (SysTick->CTRL & 0x10000) {
    /* Timer tick every 100 ms */
    timer_tick ();
  }
}
/*******************************************************************************
* Function Name  : ADC
* Description    : Configura los pines de salida y entrada de nuestro programa
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void init_ADC(void){	
   LPC_SC->PCONP|= (1<<12);                 // Power ON
	 LPC_PINCON->PINSEL3|=(0x0F<<28);         // AD0.4 Y (AD0.5)  (P1.30) y P1.31
   LPC_PINCON->PINMODE3|=(1<<31)|(1<<29);       	// Deshabilita pullup/pulldown
   LPC_SC->PCLKSEL0|= (0x00<<8);        		// CCLK/4 (Fpclk después del reset) (100 Mhz/4 = 25Mhz)
   LPC_ADC->ADCR|=(1<<4)|(1<<5)            //canal 7 y canal 6
                        |(0x01<<8)         	//CLKDIV=1   (Fclk_ADC=25Mhz /(1+1)= 12.5Mhz)
                        |(0x01<<21)
												|(0x00<<24);      		// PDN=1
	
	 LPC_ADC->ADINTEN|=(1<<5); 					//Habilitacion de interrupcion tras fin de conversion del canal 7
	 LPC_ADC->ADINTEN&=~(1<<8);					//ADGINTEN deshabilitado porque estamos en modo burst
   NVIC_EnableIRQ(ADC_IRQn);					//Habilitamos la interrupcion tras fin de conversion
}	
/*******************************************************************************
* Function Name  : ADC microfono
* Description    : Configura los pines de salida y entrada de nuestro programa
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void init_ADC_microfono(void){
	LPC_SC->PCONP|= (1<<12);                 // Power ON
	LPC_PINCON->PINSEL1|=(1<<14);
	LPC_PINCON->PINMODE1|=(1<<15);
	LPC_SC->PCLKSEL0|=(0x00<<8);
	LPC_ADC->ADCR=(1<<0)| // AD0.5 -> canal 5 (P1.31)
		(1<<8)| // FclkADC= 25e6/2= 12,5 MHz
		(1<<21)| // PDN=1
		(4<<24); // MAT0.1 -> frecuencia de muestreo
	LPC_ADC->ADINTEN=(1<<0);  // Hab. Int. canal 5 para DMA
	NVIC_DisableIRQ(ADC_IRQn); // Ojo! ADC  interrumpe!!
}
void init_DAC(void){
	LPC_PINCON->PINSEL1|= (2<<20); 	 	// DAC output = P0.26 (AOUT)
	LPC_PINCON->PINMODE1|= (2<<20); 	// Deshabilita pullup/pulldown
	LPC_SC->PCLKSEL0|= (0x00<<22); 	 	// CCLK/4 (Fpclk después del reset) (100 Mhz/4 = 25Mhz)	
	LPC_DAC->DACCTRL=0;								// ? 
}
void genera_muestras(){
	uint8_t i;
	for(i=0;i<50;i++)
		muestras[i]=1023*(0.5 + 0.5*sin(2*pi*i/50)); // Ojo! el DAC es de 10bits
}
/*******************************************************************************
* Function Name  : configPWM
* Description    : Configura una señal PWM de 1ms de periodo por P3.26
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void configPWM(void) {
	LPC_PINCON->PINSEL3|=(1<<21); // P1.26 salida PWM (PWM1.6)
	LPC_SC->PCONP|=(1<<6);
	LPC_PWM1->MR0=Fpclk*Tpwm-1;
  LPC_PWM1->PCR|=(1<<14); //configurado el ENA6 (1.6)
  LPC_PWM1->MCR|=(1<<1);
  LPC_PWM1->TCR|=(1<<0)|(1<<3);
}
/********************* WATCHDOG TIMER *********************/
void WDTFeed(void){
LPC_WDT->WDFEED=0xAA; /*Feeding sequence*/
LPC_WDT->WDFEED=0x55;
}
/*--------------------------- WDTinit ------------------------------------*/
void WDTinit(void){
LPC_WDT->WDTC=Fpclk*4;
LPC_WDT->WDCLKSEL=0x01;
LPC_WDT->WDMOD=0x03; // Enable y Reset if Timeout
	
LPC_WDT->WDFEED=0xAA; /*Feeding sequence*/
LPC_WDT->WDFEED=0x55;
}
void init_TIMER0(void){
	  LPC_SC->PCONP|=(1<<1);						// ? 
	  //LPC_PINCON->PINSEL3|= 0x0C000000; // ?
    LPC_TIM0->PR = 0x00;							// ?   
    LPC_TIM0->MCR = 0x10;   					// ?   
    LPC_TIM0->MR1 = (F_pclk/F_muestreo/2)-1; // Se han de producir DOS Match para iniciar la conversión!!!!   
    LPC_TIM0->EMR = 0x00C2 ;   				// ? 
    LPC_TIM0->TCR = 0x01;     				//Stop y Reset
}	


/*  Timer 1 en modo Output Compare (reset T0TC on Match 0)
	Counter clk: 25 MHz 	MAT1.0 : On match, salida de una muestra hacia el DAC */
void init_TIMER1(void){
  LPC_SC->PCLKSEL0|=(1<<3); //Configuro el CCLK/2 a 50MHz-> 20ns
  LPC_TIM1->MR0=(F_pclk/F_muestreo/2)-1;//Configuro la interrupción por match del Timer1 que depende del valor de la frecuencia
	LPC_TIM1->PR=0; //Cada vez que el Prescaler alcance este valor se incrementa en uno el contador del Timer1
	LPC_TIM1->MCR=0x3; //Habilito la interrupción por match del MR0 y reseteo por match del MR0
	LPC_TIM1->TCR = 0x02;							// Timer STOP y RESET
  NVIC_EnableIRQ(TIMER1_IRQn);			// 
}
/*******************************************************************************
* Function Name  : configTimer2
* Description    : Configura el Timer 2 para hacer captura de flancos ascendentes 
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void ConfigTimer2(void){
	LPC_SC->PCONP |= 1<<22;					// TIMER2
	LPC_PINCON->PINSEL0 |= 3<<8;		// P0.4 --> CAP2.0 (A)
	LPC_TIM2->TCR = 1<<1;						// reset timer
	LPC_TIM2->PR  = prescaler;			// PR=24
	LPC_TIM2->CCR = 1<<2 | 1<<0;		// capture rising CAP2.0 -> CR0, interrupt on CR0
	NVIC_EnableIRQ(TIMER2_IRQn);
	LPC_TIM2->TCR = 1<<0;						// start timer			// start timer
}
/*******************************************************************************
* Function Name  : ConfigTimer3
* Description    : Configuración del TIMER3 para lectura de láser cada microsegundo
* Input          : None
* Output         : None                
* Return         : None
* Attention		  : None
*******************************************************************************/
void ConfigTimer3(void){  
	LPC_SC->PCONP |= 1<<23;			// Power TIM3
	LPC_TIM3->TCR = 1<<1; 			// Reset TIM3
	LPC_TIM3->PR = 25000-1;			// 25 MHz -> 1 ms Tick
	LPC_TIM3->MR0 = 0xFFFF;			// Reset en 16 bits
	LPC_TIM3->MCR = 1<<1;				// Reset on MR0	
	LPC_TIM3->TCR = 1<<0; 			// Start TIM3
}
/*******************************************************************************
* Function Name  : setBrillo
* Description    : Actualiza el valor de la señal PWM entre 0.3ms y 1ms
* Input          : Brillo - Debe tomar valores de 30 a 100
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void setBrillo(uint8_t brilloTFT){
	 LPC_PWM1->MR6=(Fpclk*Tpwm*brilloTFT/100); // TH
   LPC_PWM1->LER|=(1<<6)|(1<<0);
}
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
void squareButton(struct t_screenZone* zone, char * text, uint16_t textColor, uint16_t lineColor){
   LCD_DrawLine( zone->x, zone->y, zone->x + zone->size_x, zone->y, lineColor);
   LCD_DrawLine( zone->x, zone->y, zone->x, zone->y + zone->size_y, lineColor);
   LCD_DrawLine( zone->x, zone->y + zone->size_y, zone->x + zone->size_x, zone->y + zone->size_y, lineColor);
   LCD_DrawLine( zone->x + zone->size_x, zone->y, zone->x + zone->size_x, zone->y + zone->size_y, lineColor);
	 GUI_Text(zone->x + zone->size_x/2 - (strlen(text)/2)*8, zone->y + zone->size_y/2 - 16,
            (uint8_t*) text, textColor, Black);	
}
/*******************************************************************************
* Function Name  : drawCircle
* Description    : Dibuja un circulo
* Input          : Xpos: posición x
*                  Ypos: color posición y
*                  radio: radio circulo
*                  color: color de la circulo
* Output         : None                
* Return         : None
* Attention		  : None
*******************************************************************************/
void drawCircle(uint16_t Xpos, uint16_t Ypos, uint16_t radio, uint16_t color) {
    int x, y;
    for (y = -radio; y <= radio; y++){
			for (x = -radio; x <= radio; x++){
				if (x*x + y*y <= radio*radio){
				LCD_SetPoint(Xpos + x, Ypos + y, color);
        }
      }
		}
}
/*******************************************************************************
* Function Name  : drawMinus
* Description    : Draw a minus sign in the center of the zone
* Input          : zone: zone struct
*                  lineColor
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void drawMinus(struct t_screenZone* zone, uint16_t lineColor){
   LCD_DrawLine( zone->x + 5 , zone->y + zone->size_y/2 - 1, 
                 zone->x + zone->size_x-5, zone->y + zone->size_y/2 - 1,
                 lineColor);
   LCD_DrawLine( zone->x + 5 , zone->y + zone->size_y/2, 
                 zone->x + zone->size_x-5, zone->y + zone->size_y/2,
                 lineColor);
   LCD_DrawLine( zone->x + 5 , zone->y + zone->size_y/2 + 1, 
                 zone->x + zone->size_x-5, zone->y + zone->size_y/2 + 1,
                 lineColor);
}
/*******************************************************************************
* Function Name  : drawMinus
* Description    : Draw a minus sign in the center of the zone
* Input          : zone: zone struct
*                  lineColor
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void drawAdd(struct t_screenZone* zone, uint16_t lineColor){
   drawMinus(zone, lineColor);
   
   LCD_DrawLine( zone->x + zone->size_x/2 - 1,  zone->y + 5 ,
                 zone->x + zone->size_x/2 - 1,  zone->y + zone->size_y - 5, 
                 lineColor);
   LCD_DrawLine( zone->x + zone->size_x/2 ,  zone->y + 5 ,
                 zone->x + zone->size_x/2 ,  zone->y + zone->size_y - 5, 
                 lineColor);
   LCD_DrawLine( zone->x + zone->size_x/2 + 1,  zone->y + 5 ,
                 zone->x + zone->size_x/2 + 1,  zone->y + zone->size_y - 5, 
                 lineColor);
}
/*******************************************************************************
* Function Name  : screenMain
* Description    : Visualiza la pantalla principal
* Input          : None
* Output         : None
* Return         : None
* Attention		  : None
*******************************************************************************/
void screenMain(void){
	if (statechart==IDLE){
	//drawCircle(&zone_4, "BIENVENIDO", White, Red);
	//drawCircle(20,170,20, Red);
	squareButton(&zone_2, "BIENVENIDO", White, Red);
	drawCircle(50,260,42, Red);
	drawCircle(190,260,42, Red);
	GUI_Text(30,255,"MANUAL",White,Red);
	GUI_Text(150,255,"AUTOMATICO",White,Red);
	}
	else if(statechart==MANUAL){
	squareButton(&zone_1, "Angulo del Motor: ", White, Red);
	squareButton(&zone_2, "Temperatura: ", White, Red);
	squareButton(&zone_3, "Distancia: ", White, Red);
	squareButton(&zone_4, "Brillo TFT: ", White, Red);
	GUI_Text(95,275,"Alarma",White,Black);
  if(AlarmaActivada==OFF){
		drawCircle(50,280,20, Red);
		GUI_Text(40,275,"OFF",White,Red);
	}else{
		drawCircle(50,280,20, White);
		GUI_Text(40,275,"OFF",Black,White);
	} 
	if(AlarmaActivada==ON){
		drawCircle(190,280,20, Red);
		GUI_Text(182,275,"ON",White,Red);
	}else{
		drawCircle(190,280,20, White);
		GUI_Text(182,275,"ON",Black,White);
	}
	}
	else if(statechart==AUTOMATICO){
	squareButton(&zone_1, "Temperatura: ", White, Red);
	squareButton(&zone_2, "Distancia: ", White, Red);
	squareButton(&zone_3, "Brillo: ", White, Red);
	GUI_Text(95,275,"Alarma",White,Black);
  if(AlarmaActivada==OFF){
		drawCircle(50,280,20, Red);
		GUI_Text(40,275,"OFF",White,Red);
	}else{
		drawCircle(50,280,20, White);
		GUI_Text(40,275,"OFF",Black,White);
	} 
	if(AlarmaActivada==ON){
		drawCircle(190,280,20, Red);
		GUI_Text(182,275,"ON",White,Red);
	}else{
		drawCircle(190,280,20, White);
		GUI_Text(182,275,"ON",Black,White);
	}
	}
	else if(statechart==CONFIGURACION){
	squareButton(&zone_1, "Direccion IP: ", White, White);
	if(seleccion==ZONA2){
	squareButton(&zone_2, "Umbral Temperatura: ", White, Red);
	}else squareButton(&zone_2, "Umbral Temperatura: ", White, White);
	if(seleccion==ZONA3){
	squareButton(&zone_3, "Umbral Distancia: ", White, Red);
	}else squareButton(&zone_3, "Umbral Distancia: ", White, White);
	squareButton(&zone_4, "Brillo TFT: ", White, White);
	drawMinus(&zone_5, White);
  drawAdd(&zone_6, Red);
	}
}

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
void checkTouchPanel(void){
	Coordinate* coord;
	
	coord = Read_Ads7846();
	
	if (coord > 0) {
	  getDisplayPoint(&display, coord, &matrix );
     pressedTouchPanel = 1;
   }   
   else
   {   
     pressedTouchPanel = 0;
      
     // Esto es necesario hacerlo si hay dos zonas diferentes en 
     // dos pantallas secuenciales que se solapen      
     zone_1.pressed = 1;
     zone_2.pressed = 1;
     zone_3.pressed = 1;
     zone_4.pressed = 1;
     zone_5.pressed = 1;
     zone_6.pressed = 1;
   }  
}
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
int8_t zonePressed(struct t_screenZone* zone){
	if (pressedTouchPanel == 1) {

		if ((display.x > zone->x) && (display.x < zone->x + zone->size_x) && 
			  (display.y > zone->y) && (display.y < zone->y + zone->size_y))
      {
         zone->pressed = 1;
		   return 1;
      }   
	}
   
	zone->pressed = 0;
	return 0;
}
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
int8_t zoneNewPressed(struct t_screenZone* zone){
	if (pressedTouchPanel == 1) {
		if ((display.x > zone->x) && (display.x < zone->x + zone->size_x) && 
			  (display.y > zone->y) && (display.y < zone->y + zone->size_y))
      {
         if (zone->pressed == 0){   
            zone->pressed = 1;
            return 1;
         }
		   return 0;
      }
	}
  zone->pressed = 0;
	return 0;
}

/*******************************************************************************
* Function Name  : ADC_Handler
* Description    : Llamada a la rutina de interrupcion por fin de conversion del ADC
* Input          : Potenciometro y sensor de temp
* Output         : Modifica las variables BrilloTFT y UmbralTemp
* Return         : Valores actualizados
* Attention		  : None
*******************************************************************************/
void ADC_IRQHandler(void){
	LPC_ADC->ADCR&=~(1<<16); //MODO BURST DESACTIVADO
	if(MicrofonoActivado==OFF){
	LPC_ADC->ADCR&=~(1<<16); //MODO BURST DESACTIVADO
//Almacenamos las muestras en las variables
	brilloTFT=((LPC_ADC->ADDR4>>4)&0xFFF)*100/4095;	// Potenciometro
	setBrillo(brilloTFT);
	Temperatura=((LPC_ADC->ADDR5>>4)&0xFFF)*330/4095; //LM35
	}else
	muestras_voz[incrementos++]=((LPC_ADC->ADDR0>>8)&0xFF);	// se borra automat. el flag DONE al leer ADCGDR
	if(incrementos>=15995){
	incrementos=0;
	LPC_TIM0->TCR=0; //Stop y reset timer0
	LPC_ADC->ADCR&=~(1<<0);
	LPC_ADC->ADINTEN&=~(1<<0);
	NVIC_DisableIRQ(ADC_IRQn);
	init_ADC();
	MicrofonoActivado=0;
	}
}
void TIMER1_IRQHandler(void){
	LPC_TIM1->IR|=(1<<0); 						// ?
	if(modo==1){ //Pitido
		LPC_DAC->DACR=(muestras[indice_muestra++]<<6); // ? 
		if(indice_muestra>=50){
			indice_muestra=0;
		}
	}
	else{ //Voz grabada
		LPC_DAC->DACR=(muestras_voz[indice_muestra++]<<8); // ? 
		if(indice_muestra>=15995){
			indice_muestra=0;
			if(statechart==CONFIGURACION){
				LPC_TIM1->TCR=1<<2;	//Stop Timer and reset, DAC= 0V.
				LPC_DAC->DACR=0;	 // 0 V
			}
		}
	}
}	
/*******************************************************************************
* Function Name  : TIMER2_IRQHandler
* Description    : Llamada a la rutina de interrupcion del timer 2 por captura de CR0
* Input          : Flancos ascendentes del Encoder
* Output         : Modifica el valor Angulo Motor
* Return         : Valores actualizados
* Attention		  : None
*******************************************************************************/
void TIMER2_IRQHandler(void){			// CAP2.0(A) and CAP 2.1(B) Rising 
	LPC_TIM2->IR |= 1<<4;						// borrar flag isr event CR0
	Encoder=1;
	if((LPC_GPIO0->FIOPIN & (1<<5))){ //Si P0.5 (Canal B) esta recibiendo flancos de salida
		Dir=0;
		if(AnguloMotor==OFF){
			AnguloMotor=360-18;
		}else{
		AnguloMotor=AnguloMotor-18;
		}
	}else{
		Dir=1;
		AnguloMotor=AnguloMotor+18;
		if(AnguloMotor==360){
			AnguloMotor=OFF;
		}
	}
	NumPulsos=Desplazamiento;
}

/*******************************************************************************
* Function Name  : SysTick_Handler
* Description    : Cada vez que se alcanza un 1ms se verfica, actualiza los datos de angulo del motor y se mueve el motor
* Input          : zone: Estructura con la información de la zona
* Output         : Modifica los valores Angulo Motor y lo mueve
* Return         : Valores actualizados
* Attention		  : None
*******************************************************************************/
void SysTick_Handler(void){
	if(flagPulsacion==OFF){
		ContadorDeFlagInactivo++;
		if(ContadorDeFlagInactivo>=500){
			flagPulsacion=ON;
			ContadorDeFlagInactivo=0;
			flagISP=OFF;
			flagKey1=OFF;
		}
	}
	if(statechart==AUTOMATICO){
		contadorAutomatico++;
		if(contadorAutomatico>=50){
				contadorAutomatico=0;
				NumPulsosAutomatico--;
				if(NumPulsosAutomatico==0){
					NumPulsosAutomatico=Desplazamiento*20;
					if(Dir==1){
						Dir=0;
					}else{
						Dir=1;
					}
				}
		}
		i&=7; // Contador circular (8 pasos)
		if(NumPulsosAutomatico!=0){
			if(Dir==1){
				LPC_GPIO0->FIOPIN=(secuencia[i++]<<27);
				if(AnguloMotor==0){
					AnguloMotor=360-1;
				}else{
				AnguloMotor=AnguloMotor-1;
				}
			}
			//Actualiza la secuencia P2.2 ... P2.5
			if(Dir==0){
				LPC_GPIO0->FIOPIN=(secuencia[i--]<<27);
				AnguloMotor=AnguloMotor+1;
				if(AnguloMotor==360){
					AnguloMotor=0;
				}
			}
		}		
	}
	if (MicrofonoActivado==OFF){
	ADC_int++;
		if(ADC_int>=100){ //Cada 0,1 se activa la conversion del ADC
			LPC_ADC->ADCR|=(1<<16); //Modo BURST
			ADC_int=0;
		}
	}
	if(statechart==MANUAL){
		if((LPC_GPIO0->FIOPIN & (1<<10))==OFF){ //Quiero que chequee si el pulsador del Encoder esta pulsado
			if(AnguloMotor!=0){ //Si el angulo no es 0
				if(AnguloMotor<=180){//Si el angulo es menor de 180 que vuela a la posicion 0 en el sentido antihorario
					Dir=0;
					NumPulsos=(4096*AnguloMotor)/360;
					AnguloMotor=0;
				}else{ //Si el angulo es mayor de 180 que vuela a la posicion 0 en el sentido horario
					Dir=1;
					NumPulsos=(4096*(360-AnguloMotor))/360;
					AnguloMotor=0;
				}
			}
		}
		i&=7; // Contador circular (8 pasos)
		if(NumPulsos!=0){
			if(Dir==1){
				LPC_GPIO0->FIOPIN=(secuencia[i++]<<27);
				NumPulsos--;
			}
		//Actualiza la secuencia P2.2 ... P2.5
			if(Dir==0){
				LPC_GPIO0->FIOPIN=(secuencia[i--]<<27);
				NumPulsos--;
			}
		}
	}
} 
//Interrupciones
void EINT0_IRQHandler(void){ //Interrupción para la selección de los DISPLAYS
	LPC_SC->EXTINT |= (0<<1);   //Borrar el flag de la EINT1 --> EXTINT.0
	flagISP=ON;
	flagPulsacion=OFF;
	init_TIMER0();
	init_ADC_microfono();
	MicrofonoActivado=1;
	NVIC_EnableIRQ(ADC_IRQn); // Ojo! ADC  interrumpe!!
}
void EINT1_IRQHandler(void){
	LPC_SC->EXTINT|=(0<<2);
	flagKey1=ON;
	flagPulsacion=OFF;
	ContadorFlag=0;
	LPC_TIM1->TCR=0x01; //Arrancamos el Timer1
}
void StateChart(void){
	switch(statechart){
		case IDLE:
			screenMain();
			LPC_PINCON->PINSEL4&=~(1<<20);
			LPC_PINCON->PINSEL4&=~(1<<22);  // Configuramos P2.10, P2.11 como GPIO
			if(zonePressed(&zone_6)){
			statechart=AUTOMATICO;
			estadouart=MODO_SELECCIONADO;
			buffer[0]='A';
			LCD_Clear(Black);	
			}
			if(zonePressed(&zone_5)){
			statechart=MANUAL;
			estadouart=MODO_SELECCIONADO;
			buffer[0]='M';
			LCD_Clear(Black);	
			}
			if((LPC_GPIO2->FIOPIN & (1<<11))==OFF){
			statechart=CONFIGURACION;
			LCD_Clear(Black);
			estadouart=MODO_SELECCIONADO;
			buffer[0]='C';
			}
		break;
		case MANUAL:
			screenMain();
			LPC_PINCON->PINSEL4&=~(1<<20);
			LPC_PINCON->PINSEL4&=~(1<<22);  // Configuramos P2.10, P2.11 como GPIO
			if (((LPC_GPIO2->FIOPIN & (1<<10))==OFF) && ((LPC_GPIO2->FIOPIN & (1<<11))==OFF)&&(flagPulsacion)){ //Si pulsas ISP y KEY1
				flagPulsacion=OFF;
				statechart=AUTOMATICO;
				estadouart=MODO_SELECCIONADO;
				buffer[0]='A';
				LCD_Clear(Black);
			}else{
				if(zonePressed(&zone_6)){
				estadouart=MODO_SELECCIONADO;
				buffer[0]='M';
				AlarmaActivada=ON;
				}
				if(zonePressed(&zone_5)){
				estadouart=MODO_SELECCIONADO;
				buffer[0]='M';
				AlarmaActivada=OFF;
				}
				if(((LPC_GPIO2->FIOPIN & (1<<11))==0)&&(flagPulsacion)){//Si KEY1 esta pulsado
					flagPulsacion=OFF;
					estadouart=MODO_SELECCIONADO;
					buffer[0]='M';
					if(AlarmaActivada==ON){
						AlarmaActivada=OFF;
					}else AlarmaActivada=ON;
				}
				if(((LPC_GPIO2->FIOPIN & (1<<10))==0)&&(flagPulsacion)){//Si ISP esta pulsado
					flagPulsacion=OFF;
					estadouart=MODO_SELECCIONADO;
					buffer[0]='M';
					if(MedidasPeoriodicas==ON){
					MedidasPeoriodicas=OFF;
					}else MedidasPeoriodicas=ON;
				}
				if(MedidasPeoriodicas==ON){
					if(LPC_TIM3->PC%19 == 0){ //Cambia el valor de Laser cada 50 ms (1/25e6 * 2^16 *19)
						DistanciaMedida=(float)distancia/10;	
					}
					if((DistanciaMedida<UmbralDistancia)&&(AlarmaActivada==ON)){
						modo=ON;
					}
					if((Temperatura>UmbralTemp)&&(AlarmaActivada==ON)){
						modo=OFF;
					}
					if(((Temperatura>UmbralTemp)||(DistanciaMedida<UmbralDistancia))&&(AlarmaActivada==ON)){
						LPC_TIM1->TCR=0x01; //Arrancamos el Timer1
					}else{
						LPC_TIM1->TCR=1<<2;	//Stop Timer and reset, DAC= 0V.
						LPC_DAC->DACR=0;	 // 0 V
					}
				}
				//A partir de aqui, mostrar los valores de cada modo
				sprintf(texto,"%10d", AnguloMotor);
				GUI_Text(zone_1.x + zone_1.size_x/2 - (strlen(texto)/2)*8-35, zone_1.y+ zone_1.size_y/2, 
				(uint8_t*) texto, White, Black);	//Restamos 35 para que el numero se situe en el medio del LCD
				sprintf(texto,"%10.1f", Temperatura);
				GUI_Text(zone_2.x + zone_2.size_x/2 - (strlen(texto)/2)*8-35, zone_2.y + zone_2.size_y/2,
				(uint8_t*) texto, White, Black); //Restamos 35 para que el numero se situe en el medio del LCD
				sprintf(texto,"%10.1f cm", DistanciaMedida);
				GUI_Text(zone_3.x + zone_3.size_x/2 - (strlen(texto)/2)*8-35, zone_3.y + zone_3.size_y/2,
				(uint8_t*) texto, White, Black); //Restamos 35 para que el numero se situe en el medio del LCD
				sprintf(texto,"%10d", brilloTFT);
				GUI_Text(zone_4.x + zone_4.size_x/2 - (strlen(texto)/2)*8-35, zone_4.y + zone_4.size_y/2,
				(uint8_t*) texto, White, Black); //Restamos 35 para que el numero se situe en el medio del LCD
				}
		break;
		case AUTOMATICO:
			screenMain();
			LPC_PINCON->PINSEL4&=~(1<<20);
			LPC_PINCON->PINSEL4&=~(1<<22);  // Configuramos P2.10, P2.11 como GPIO
			if (((LPC_GPIO2->FIOPIN & (1<<10))==0) && ((LPC_GPIO2->FIOPIN & (1<<11))==0)&&(flagPulsacion)){ //Si pulsas ISP y KEY1
			flagPulsacion=OFF;
			statechart=MANUAL;
			estadouart=MODO_SELECCIONADO;
			buffer[0]='M';
			LCD_Clear(Black);
			}else{
			if(((LPC_GPIO2->FIOPIN & (1<<11))==0)&&(flagPulsacion)){//Si KEY1 esta pulsado
			flagPulsacion=OFF;
			estadouart=MODO_SELECCIONADO;
			buffer[0]='A';
				if(AlarmaActivada==ON){
					AlarmaActivada=OFF;
				}else AlarmaActivada=ON;
			}
			if(zonePressed(&zone_6)){
			estadouart=MODO_SELECCIONADO;
			buffer[0]='A';
			AlarmaActivada=ON;
			}
			if(zonePressed(&zone_5)){
			estadouart=MODO_SELECCIONADO;
			buffer[0]='A';
			AlarmaActivada=OFF;
			}
			if(LPC_TIM3->PC%19 == 0){ //Cambia el valor de Laser cada 50 ms (1/25e6 * 2^16 *19)
					DistanciaMedida=(float)distancia/10;	
			}
			if((DistanciaMedida<UmbralDistancia)&&(AlarmaActivada==ON)){
				modo=ON;
			}
			if((Temperatura>UmbralTemp)&&(AlarmaActivada==ON)){
				modo=OFF;
			}
			if(((Temperatura>UmbralTemp)||(DistanciaMedida<UmbralDistancia))&&(AlarmaActivada==ON)){
					LPC_TIM1->TCR=0x01; //Arrancamos el Timer1
			}else{
					LPC_TIM1->TCR=1<<2;	//Stop Timer and reset, DAC= 0V.
					LPC_DAC->DACR=0;	 // 0 V
			}
			//A partir de aqui, mostrar los valores de cada modo
			sprintf(texto,"%10.1f", Temperatura);
			GUI_Text(zone_1.x + zone_1.size_x/2 - (strlen(texto)/2)*8-35, zone_1.y+ zone_1.size_y/2, 
			(uint8_t*) texto, White, Black);	//Restamos 35 para que el numero se situe en el medio del LCD
			sprintf(texto,"%10.1f cm", DistanciaMedida);
			GUI_Text(zone_2.x + zone_2.size_x/2 - (strlen(texto)/2)*8-35, zone_2.y + zone_2.size_y/2,
			(uint8_t*) texto, White, Black); //Restamos 35 para que el numero se situe en el medio del LCD
			sprintf(texto,"%10d", brilloTFT);
			GUI_Text(zone_3.x + zone_3.size_x/2 - (strlen(texto)/2)*8-35, zone_3.y + zone_3.size_y/2,
			(uint8_t*) texto, White, Black); //Restamos 35 para que el numero se situe en el medio del LCD
			}
		break;
		case CONFIGURACION:
			screenMain();
			LPC_PINCON->PINSEL4|=(0x01<<20); //Configuro P2.10 como entrada de interrupcion EXT0, boton ISP
			LPC_PINCON->PINSEL4|=(0x01<<22); //Configuro P2.11 como entrada de interrupcion EXT1, boton Key 1
			NVIC_ClearPendingIRQ(EINT0_IRQn);
			NVIC_ClearPendingIRQ(EINT1_IRQn);
			//Habilitación de las interrupciones
			NVIC_EnableIRQ(EINT0_IRQn);	// Habilitar interrupción EINT0
			NVIC_EnableIRQ(EINT1_IRQn);	// Habilitar interrupción EINT1
			if((flagKey1==ON)&&(flagISP==ON)){
				NVIC_DisableIRQ(EINT0_IRQn);	// Habilitar interrupción EINT0
				NVIC_DisableIRQ(EINT1_IRQn);	// Habilitar interrupción EINT1
				LCD_Clear(Black); //Hacemos Doble CLear porque con uno se poducen bugs
				statechart=MANUAL;
				estadouart=MODO_SELECCIONADO;
				buffer[0]='M';
				LCD_Clear(Black);
				LPC_TIM1->TCR=1<<2;	//Stop Timer and reset, DAC= 0V.
				LPC_DAC->DACR=0;	 // 0 V
			}
			if (zoneNewPressed(&zone_2)) //Si se presiona la zona media alta
        seleccion=ZONA2;
			if (zoneNewPressed(&zone_3)) //Si se presiona la zona media
        seleccion=ZONA3;   
			if (zonePressed(&zone_6)){  //Si se presiona la parte inferior izquierda 
				estadouart=MODO_SELECCIONADO;
				buffer[0]='C';
				if(seleccion==ZONA2){
					UmbralTemp=UmbralTemp+0.5; 
				} 
				if(seleccion==ZONA3){
					UmbralDistancia=UmbralDistancia+0.1; 
				} 
			} 
			if (zonePressed(&zone_5)){ //Si se presiona la parte inferior izquierda
				estadouart=MODO_SELECCIONADO;
				buffer[0]='C';
				if(seleccion==ZONA2){
					UmbralTemp=UmbralTemp-0.5; 
				} 
				if(seleccion==ZONA3){
					if(UmbralDistancia>0){
					UmbralDistancia=UmbralDistancia-0.1; 
					}
				}  
			}
			//A partir de aqui, mostrar los valores de cada modo
			sprintf(texto,"%10.1f", UmbralTemp);
			GUI_Text(zone_2.x + zone_2.size_x/2 - (strlen(texto)/2)*8-35, zone_2.y+ zone_2.size_y/2, 
			(uint8_t*) texto, White, Black);	//Restamos 35 para que el numero se situe en el medio del LCD
			sprintf(texto,"%10.1f cm", UmbralDistancia);
			GUI_Text(zone_3.x + zone_3.size_x/2 - (strlen(texto)/2)*8-35, zone_3.y + zone_3.size_y/2,
			(uint8_t*) texto, White, Black); //Restamos 35 para que el numero se situe en el medio del LCD
			sprintf(texto,"%10d", brilloTFT);
			GUI_Text(zone_4.x + zone_4.size_x/2 - (strlen(texto)/2)*8-35, zone_4.y + zone_4.size_y/2,
			(uint8_t*) texto, White, Black); //Restamos 35 para que el numero se situe en el medio del LCD	
		break;
	}
}
void FSM_uart(void){
		switch(estadouart){
			case MENU_INICIAL:
				tx_cadena_UART0("Practica Libre de Ruben Cavero y Andres Garcia\n"
												"\nPor favor escribre tu nombre\n\r");
				estadouart=TX_ENVIADO;
			break;
			case TX_ENVIADO:
			if(tx_completa==1){
					tx_completa=0;
				estadouart=RX_RECIBIDO;
				if(estado_anterior==ON){
					estado_anterior=OFF;
					estadouart=NOMBRE;
				}
			}
			break;
			case RX_RECIBIDO:
				if(rx_completa){
					rx_completa=0;
					for (i=0; i<=30; i++){
						NombreUsuario[i]=buffer[i];
					}
					sprintf(buffer_tx,"\nHola %s, Que modo quieres utilizar? Manual(M), Automatico(A), Configuracion(C) o Capturar Medidas(F)\r\n", buffer);
					estadouart=NOMBRE;
				}
			break;
			case NOMBRE:
				tx_cadena_UART0(buffer_tx);
				estadouart=MODOFUNC;
			break;
			case MODOFUNC:
			if(tx_completa==1){
				tx_completa=0;
				estadouart=MODO;
			}
			break;
			case MODO:
				if(rx_completa){
					rx_completa=0;
					estadouart=MODO_SELECCIONADO;
				}
			break;
			case MODO_SELECCIONADO:
				switch(buffer[0]){
					case 'A':
						tx_cadena_UART0("\nHa seleccionado modo automatico\n"
														"Introduce un comando:\n" 
														"\n -Alarma: Activar (A) o Desactivar (D)\n"
														"\n -Volver al menu inicial(E)\n\r");
						estadouart=ESCRITURA_MODO_SELECCIONADO;
						siguiente_estado=ESTADO_AUTOMATICO;
						if(statechart==IDLE){
							LCD_Clear(Black);
						}
						statechart=AUTOMATICO;
					break;
					case 'M':
						tx_cadena_UART0("\nHa seleccionado modo manual\n"
														"Introduce un comando:\n" 
														"\n -Motor: R (Right), L (Left), Z (Position Zero)\n"
														"\n -Tomar medidas Laser: T (Tomar)\n"
														"\n -Alarma: Activar (A) o Desactivar (D)\n"
														"\n -Volver al menu inicial(E)\n\r");
						estadouart=ESCRITURA_MODO_SELECCIONADO;
						siguiente_estado=ESTADO_MANUAL;
						if(statechart==IDLE){
							LCD_Clear(Black);
						}
						statechart=MANUAL;
					break;
					case 'C':
						tx_cadena_UART0("\nHa seleccionado modo Configuracion\n"
														"Introduce un comando:\n" 
														"\n -Umbral Temperatura: '+' (Incrementar), '-' (Decrementar) o 'Z' (Valor Default)\n"
														"\n -Umbral Distancia: 'M' (Incrementar), 'L' (Decrementar) o 'I' (Valor Default)\n"
														"\n -Volver al menu inicial(E)\n\r");
						estadouart=ESCRITURA_MODO_SELECCIONADO;
						siguiente_estado=ESTADO_CONFIGURACION;
						if(statechart==IDLE){
							LCD_Clear(Black);
						}
						statechart=CONFIGURACION;
					break;
					case 'F':
						if (MostrarDatos==ON){
							tx_cadena_UART0("\nCapturar Medidas Desactivado\n\r");
							MostrarDatos=OFF;
						}else{
							tx_cadena_UART0("\nCapturar Medidas Activado\n\r");
							MostrarDatos=ON;
						}
						for (i=0; i<=30; i++){
							buffer[i]=NombreUsuario[i];
						}
						estadouart=TX_ENVIADO;
						estado_anterior=ON;
					break;
				}
			case ESCRITURA_MODO_SELECCIONADO:
				if(tx_completa==1){
					tx_completa=0;
					estadouart=siguiente_estado;
				}
			break;
			case ESTADO_MANUAL:
				if(rx_completa==1){
					rx_completa=0;
					switch(buffer[0]){
						case 'R':
						Dir=1;
						AnguloMotor=AnguloMotor+18;
						if(AnguloMotor==360){
							AnguloMotor=0;
						}
						NumPulsos=Desplazamiento;
						if(MostrarDatos==ON){
							estadouart=DATOS;
						}else estadouart=MODO_SELECCIONADO;
						buffer[0]='M';
						break;
						case 'L':
						Dir=0;
						if(AnguloMotor==0){
							AnguloMotor=360-18;
						}else{
							AnguloMotor=AnguloMotor-18;
						}
						NumPulsos=Desplazamiento;
						if(MostrarDatos==ON){
							estadouart=DATOS;
						}else estadouart=MODO_SELECCIONADO;
						buffer[0]='M';
						break;
						case 'Z':
						if(AnguloMotor!=0){ //Si el angulo no es 0
							if(AnguloMotor<=180){//Si el angulo es menor de 180 que vuela a la posicion 0 en el sentido antihorario
								Dir=0;
								NumPulsos=(4096*AnguloMotor)/360;
								AnguloMotor=0;
							}else{ //Si el angulo es mayor de 180 que vuela a la posicion 0 en el sentido horario
							Dir=1;
							NumPulsos=(4096*(360-AnguloMotor))/360;
							AnguloMotor=0;
							}
						}
						if(MostrarDatos==ON){
							estadouart=DATOS;
						}else estadouart=MODO_SELECCIONADO;
						buffer[0]='M';
						break;
						case 'T':
						if(MedidasPeoriodicas==ON){
							MedidasPeoriodicas=OFF;
						}else{
							MedidasPeoriodicas=ON;
						}
						if(MostrarDatos==ON){
							estadouart=DATOS;
						}else estadouart=MODO_SELECCIONADO;
						buffer[0]='M';
						break;
						case 'A':
						AlarmaActivada=ON;
						if(MostrarDatos==ON){
							estadouart=DATOS;
						}else estadouart=MODO_SELECCIONADO;
						buffer[0]='M';
						break;
						case 'D':
						AlarmaActivada=OFF;
						if(MostrarDatos==ON){
							estadouart=DATOS;
						}else estadouart=MODO_SELECCIONADO;
						buffer[0]='M';
						break;
						case 'E':
						tx_cadena_UART0("\nVolviendo al menu principal\n\r");
						for (i=0; i<=30; i++){
							buffer[i]=NombreUsuario[i];
						}
						estadouart=TX_ENVIADO;
						estado_anterior=ON;
						if(statechart==MANUAL){
							LCD_Clear(Black);
						}
						statechart=IDLE;
						sprintf(buffer_tx,"\nHola %s, Que modo quieres utilizar? Manual(M), Automatico(A), Configuracion(C) o Capturar Medidas(F)\r\n", buffer);
						break;
					}
				}
			break;
			case ESTADO_AUTOMATICO:
				if(rx_completa){
					rx_completa=0;
					switch(buffer[0]){
					case 'A':
					AlarmaActivada=ON;
					if(MostrarDatos==ON){
						estadouart=DATOS;
					}else estadouart=MODO_SELECCIONADO;
					buffer[0]='A';
					break;
					case 'D':
					AlarmaActivada=OFF;
					if(MostrarDatos==ON){
						estadouart=DATOS;
					}else estadouart=MODO_SELECCIONADO;
					buffer[0]='A';
					break;
					case 'E':
					tx_cadena_UART0("\nVolviendo al menu principal\n\r");
					for (i=0; i<=30; i++){
					buffer[i]=NombreUsuario[i];
					}
					estadouart=TX_ENVIADO;
					estado_anterior=ON;
					if(statechart==AUTOMATICO){
						LCD_Clear(Black);
					}
					statechart=IDLE;
					sprintf(buffer_tx,"\nHola %s, Que modo quieres utilizar? Manual(M), Automatico(A), Configuracion(C) o Capturar Medidas(F)\r\n", buffer);
					break;
					}
				}
			break;
			case ESTADO_CONFIGURACION:
				if(rx_completa){
					rx_completa=0;
					switch(buffer[0]){
					case '+':
					seleccion=ZONA2;
					UmbralTemp=UmbralTemp+0.5;
					if(MostrarDatos==ON){
						estadouart=DATOS;
					}else estadouart=MODO_SELECCIONADO;
					buffer[0]='C';
					break;
					case '-':
					seleccion=ZONA2;
					if(UmbralTemp>0){
					UmbralTemp=UmbralTemp-0.5;
					}
					if(MostrarDatos==ON){
						estadouart=DATOS;
					}else estadouart=MODO_SELECCIONADO;
					buffer[0]='C';
					break;
					case 'Z':
					seleccion=ZONA2;
					UmbralTemp=VALORTEMP;
					if(MostrarDatos==ON){
						estadouart=DATOS;
					}else estadouart=MODO_SELECCIONADO;
					buffer[0]='C';
					break;
					case 'M':
					seleccion=ZONA3;
					UmbralDistancia=UmbralDistancia+0.1;
					if(MostrarDatos==ON){
						estadouart=DATOS;
					}else estadouart=MODO_SELECCIONADO;
					buffer[0]='C';
					break;
					case 'L':
					seleccion=ZONA3;
					if(UmbralDistancia>0){
					UmbralDistancia=UmbralDistancia-0.1;
					}
					if(MostrarDatos==ON){
						estadouart=DATOS;
					}else estadouart=MODO_SELECCIONADO;
					buffer[0]='C';
					break;
					case 'I':
					seleccion=ZONA3;
					UmbralDistancia=VALORDIST;
					if(MostrarDatos==ON){
						estadouart=DATOS;
					}else estadouart=MODO_SELECCIONADO;
					buffer[0]='C';
					break;
					case 'E':
					tx_cadena_UART0("\nVolviendo al menu principal\n\r");
					for (i=0; i<=30; i++){
					buffer[i]=NombreUsuario[i];
					}
					estadouart=TX_ENVIADO;
					estado_anterior=ON;
					if(statechart==CONFIGURACION){
						LCD_Clear(Black);
					}
					statechart=IDLE;
					sprintf(buffer_tx,"\nHola %s, Que modo quieres utilizar? Manual(M), Automatico(A), Configuracion(C) o Capturar Medidas(F)\r\n", buffer);
					break;
					}
				}
			break;
			case DATOS:
				sprintf(buffer_tx,"\nValores actualizados:\n"
													"\nTemperatura:%0.1f, Angulo Motor:%d, Distancia:%0.1f\r\n", Temperatura, AnguloMotor, DistanciaMedida);
				estadouart=MOSTRAR_DATOS;	
			break;
			case MOSTRAR_DATOS:
				tx_cadena_UART0(buffer_tx);
				estadouart=MODO_SELECCIONADO;
			break;
		}
}
int main (void){
	ConfigGPIO();
	genera_muestras();
  LCD_Initializtion();
  LCD_Clear(Black);	
  TP_Init(); 
  //TouchPanel_Calibrate(); /* Configurar "matrix" y comentar para eliminar la calibración */ 
  screenMain();
	//WDTinit();
	Configura_SysTick(); //Configuración del Systick
  configPWM(); //Configuracion de la PWM (Timer1)
	init_TIMER1();
	ConfigTimer2(); //Timer2
	init_ADC(); //ConfigADC
	init_DAC();
	ConfigTimer3();
	VL53L1X_setTimeout(500);
	VL53L1X_init();
	VL53L1X_setDistanceMode(Long);
	VL53L1X_startContinuous(50);
	ptr_rx=buffer;	                // inicializa el puntero de recepción al comienzo del buffer
	uart0_init(9600);							 // configura la UART0 a 9600 baudios, 8 bits, 1 bit stop
	uart3_init(9600);
	init_TcpNet ();
  while(1){
	timer_poll ();
  main_TcpNet ();
	checkTouchPanel();
	StateChart();
	FSM_uart();		
	//Comprobación Laser
	if((LPC_TIM3->TC - last_TC) > 500){
		distancia = VL53L1X_read(false);		
		last_TC = LPC_TIM3->TC;
	}
	}     
}

