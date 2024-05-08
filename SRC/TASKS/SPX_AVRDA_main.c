
/*
 * File:   spxR2_main.c
 * Author: pablo
 *
 * Created on 25 de octubre de 2021, 11:55 AM
 * 
 * Debido a que es un micro nuevo, conviene ir a https://start.atmel.com/ y
 * crear un projecto con todos los perifericos que usemos y bajar el codigo
 * para ver como se inicializan y se manejan.
 * 
 * 
 * -----------------------------------------------------------------------------
 * Version 1.2.0 @ 20240401:
 * 1- Cambio el nombre de SPXR2 a SPX_AVRDA
 * 2- Hay problemas de reseteo cuando se configuran los canales analogicos.
 *    Aumento el stack de tkWan a 512 y creo una funcion que monitorea los stacks.
 *    La copio de la que hice en SPQ_AVRDA.
 *    Agrego la funcion u_check_stacks_usage() y el comando 'test stacks'
 * 3- Al configurar los canales analogicos se resetea. El problema esta en el 
 *    tama�o del buffer del hash. Se aumenta a 64 bytes.
 * 
 * -----------------------------------------------------------------------------
 * Version 1.1.0 @ 2023-11-15:
 * Modbus: El caudalimentro Shinco tiene s/n 77 entonces el id es 119 !!!
 * -----------------------------------------------------------------------------
 * Version 1.1.0 @ 2023-07-21
 * Veo que hay veces que se cuelga o que tareas ocupan mas tiempo del esperable.
 * Esto se refleja en el flasheo del led e indica problemas con la asignacion de
 * tareas.
 * https://www.freertos.org/FreeRTOS_Support_Forum_Archive/December_2013/freertos_configUSE_TIME_SLICING_278999daj.html
 * https://www.freertos.org/FreeRTOS_Support_Forum_Archive/February_2018/freertos_Preemption_by_an_equal_priority_task_c8cb93d0j.html
 * 
 * #define configUSE_TIME_SLICING 1
 * 
 * El problema esta en la funcion FLASH_0_read_eeprom_block.
 * La soluci�n fue poner vTaskDelay( ( TickType_t)( 10 ) ) ente c/escritura de bytes.
 * -----------------------------------------------------------------------------
 * Version 1.0.0 @ 2022-09-12
 * 
 * Para que imprima floats hay que poner en el linker options la opcion
 * -Wl,-u,vfprintf -lprintf_flt -lm
 * https://github.com/microchip-pic-avr-examples/avr128da48-cnano-printf-float-mplab-mcc
 * 
 * Log en el servidor:
 * hset 'SPCOMMS' 'DEBUG_DLGID' 'PPOTKIYU'
 * 
 * 
 * PENDIENTE:
 * 1- Transmitir en modo bloque al hacer un dump.
 * 2- Consumo: entrar en modo tickless
 *
 * -----------------------------------------------------------------------------
 * V1.1.1 @ 20240206
 * -No se estaba respentando el timerpoll.
 *  El problema estaba en las operaciones de 32 bits en tkSystem.
 * 
 * 
 * -----------------------------------------------------------------------------
 * V1.1.0 @ 20230626
 * Hay muchos equipos que no pasan de la configuracion.
 * El problema es que se resetea x wdt. No queda claro porque pero con un
 * wdg_reset en  wan_state_online_data se arregla.
 * HAY QUE REVISAR TODO EL TEMA DE LOS WDGs. !!!!
 * -----------------------------------------------------------------------------
 * V1.1.0 @ 20230620
 * Si el canal analogico 2 esta configurado, entonces la bateria es 0.
 * El protocolo esta modificado para hablar con APICOMMSV3
 * -----------------------------------------------------------------------------
 * V1.0.8 @ 20230505
 * En los contadores incorporo el concepto de rb_size que sirve para determinar
 * cuanto quiero promediar los contadores.
 * El problema surge en perforaciones Colonia que no es util un promediado largo.
 * 
 * -----------------------------------------------------------------------------
 * V1.0.7 @ 20230306
 * Incorporamos el modulo de modbus.
 * Manejamos 5 canales (float) por lo que aumentamos el FS_BLOCKSIZE a 64.
 * Falta:
 *  - A veces da el pc timeout ? Es como si el frame se fragmentara
 *  - WAN
 * 
 * - BUG: No podia escribir el RTC
 * - BUG: Los canales analogicos, contadores, modbus por default deben ser 'X'
 * - BUG: Los contadores al medir caudal nunca llegaban a 0. Agrego en counters_clear()
 *        que converjan a 0.
 * - BUG: No calculaba bien el hash de conf_base y por eso se reconfigura siempre
 * 
 *  
 * -----------------------------------------------------------------------------
 * V1.0.6 @ 20230202
 * Solo manejo 2 canales y el 3o es la bateria.
 * 
 * V1.0.5 @ 20230202
 * Configuro 'samples_count' y 'alarm_level' por la wan.
 *  
 * V1.0.4 @ 20230118
 * Incorporo los conceptos de 'samples_count' y 'alarm_level'
 *  
 * 
 * V1.0.3 @ 20230109
 * Agrego logica de manejo de reles K1,K2. El modem esta en K1.
 * Revisar el reset memory hard que bloquea al dlg.
 * 
 * 
 * V1.0.2 @ 2023-01-02
 * Agrego en la funcion ainputs_test_read_channel() que prenda y apague los sensores.
 * 
 *  
 */

#include "SPX_AVRDA.h"


FUSES = {
	.WDTCFG = 0x00, // WDTCFG {PERIOD=OFF, WINDOW=OFF}
	.BODCFG = 0x00, // BODCFG {SLEEP=DISABLE, ACTIVE=DISABLE, SAMPFREQ=128Hz, LVL=BODLEVEL0}
	.OSCCFG = 0xF8, // OSCCFG {CLKSEL=OSCHF}
	.SYSCFG0 = 0xD2, // SYSCFG0 {EESAVE=CLEAR, RSTPINCFG=GPIO, CRCSEL=CRC16, CRCSRC=NOCRC}
	.SYSCFG1 = 0xF8, // SYSCFG1 {SUT=0MS}
	.CODESIZE = 0x00, // CODESIZE {CODESIZE=User range:  0x0 - 0xFF}
	.BOOTSIZE = 0x00, // BOOTSIZE {BOOTSIZE=User range:  0x0 - 0xFF}
};

LOCKBITS = 0x5CC5C55C; // {KEY=NOLOCK}

/*
FUSES = {
	.WDTCFG = 0x0B, // WDTCFG {PERIOD=8KCLK, WINDOW=OFF}
	.BODCFG = 0x00, // BODCFG {SLEEP=DISABLE, ACTIVE=DISABLE, SAMPFREQ=128Hz, LVL=BODLEVEL0}
	.OSCCFG = 0xF8, // OSCCFG {CLKSEL=OSCHF}
	.SYSCFG0 = 0xD2, // SYSCFG0 {EESAVE=CLEAR, RSTPINCFG=GPIO, CRCSEL=CRC16, CRCSRC=NOCRC}
	.SYSCFG1 = 0xF8, // SYSCFG1 {SUT=0MS}
	.CODESIZE = 0x00, // CODESIZE {CODESIZE=User range:  0x0 - 0xFF}
	.BOOTSIZE = 0x00, // BOOTSIZE {BOOTSIZE=User range:  0x0 - 0xFF}
};

LOCKBITS = 0x5CC5C55C; // {KEY=NOLOCK}
*/
//------------------------------------------------------------------------------
int main(void) {

 
    system_init();
    
    frtos_open(fdTERM, 9600 );
    frtos_open(fdRS485A, 9600 );
    frtos_open(fdRS485B, 9600 );
    frtos_open(fdI2C0, 100 );
    frtos_open(fdI2C1, 100 );
    frtos_open(fdNVM, 0 );
    
    sem_SYSVars = xSemaphoreCreateMutexStatic( &SYSVARS_xMutexBuffer );
    FS_init();
    
    ainputs_init_outofrtos(sem_SYSVars);
    counters_init_outofrtos(sem_SYSVars);
    modbus_init_outofrtos(sem_SYSVars);
    piloto_init_outofrtos(sem_SYSVars);
    piloto_init();
    
    starting_flag = false;
    
    xHandle_tkCtl = xTaskCreateStatic( tkCtl, "CTL", tkCtl_STACK_SIZE, (void *)1, tkCtl_TASK_PRIORITY, tkCtl_Buffer, &tkCtl_Buffer_Ptr );
    xHandle_tkCmd = xTaskCreateStatic( tkCmd, "CMD", tkCmd_STACK_SIZE, (void *)1, tkCmd_TASK_PRIORITY, tkCmd_Buffer, &tkCmd_Buffer_Ptr );
    xHandle_tkSys = xTaskCreateStatic( tkSystem, "SYS", tkSys_STACK_SIZE, (void *)1, tkSys_TASK_PRIORITY, tkSys_Buffer, &tkSys_Buffer_Ptr );
    xHandle_tkRS485A = xTaskCreateStatic( tkRS485A, "COMMSA", tkRS485A_STACK_SIZE, (void *)1, tkRS485A_TASK_PRIORITY, tkRS485A_Buffer, &tkRS485A_Buffer_Ptr );
    xHandle_tkRS485B = xTaskCreateStatic( tkRS485B, "COMMSB", tkRS485B_STACK_SIZE, (void *)1, tkRS485B_TASK_PRIORITY, tkRS485B_Buffer, &tkRS485B_Buffer_Ptr );
    xHandle_tkWAN = xTaskCreateStatic( tkWAN, "WAN", tkWAN_STACK_SIZE, (void *)1, tkWAN_TASK_PRIORITY, tkWAN_Buffer, &tkWAN_Buffer_Ptr );

    
#ifdef PILOTO
  //  xHandle_tkPILOTO = xTaskCreateStatic( tkPiloto, "PLT", tkPILOTO_STACK_SIZE, (void *)1, tkPILOTO_TASK_PRIORITY, tkPILOTO_Buffer, &tkPILOTO_Buffer_Ptr );
#endif
    
    /* Arranco el RTOS. */
	vTaskStartScheduler();
  
	// En caso de panico, aqui terminamos.
	exit (1);
   
            
}
//------------------------------------------------------------------------------
/* configSUPPORT_STATIC_ALLOCATION is set to 1, so the application must provide an
implementation of vApplicationGetIdleTaskMemory() to provide the memory that is
used by the Idle task. */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{
/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
//------------------------------------------------------------------------------------
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize )
{
/* If the buffers to be provided to the Timer task are declared inside this
function then they must be declared static - otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH ];

	/* Pass out a pointer to the StaticTask_t structure in which the Timer
	task's state will be stored. */
	*ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

	/* Pass out the array that will be used as the Timer task's stack. */
	*ppxTimerTaskStackBuffer = uxTimerTaskStack;

	/* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
	Note that, as the array is necessarily of type StackType_t,
	configMINIMAL_STACK_SIZE is specified in words, not bytes. */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
//-----------------------------------------------------------------------------


