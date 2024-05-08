/* 
 * File:   frtos20_utils.c
 * Author: pablo
 *
 * Created on 22 de diciembre de 2021, 07:34 AM
 */

#include "SPX_AVRDA.h"
#include "pines.h"

//------------------------------------------------------------------------------
int8_t WDT_init(void);
int8_t CLKCTRL_init(void);
uint8_t checksum( uint8_t *s, uint16_t size );

//-----------------------------------------------------------------------------
void system_init()
{

	CLKCTRL_init();
    //WDT_init();
    LED_init();
    XPRINTF_init();
    //COUNTERS_init();
    OCOUT_init();
    RELE_K1_init();
    //RELE_K2_init();
    VSENSORS420_init();
    CONFIG_RTS_485A();
    CONFIG_RTS_485B();
    DRV8814_init();
    FCx_init();
    
    
}
//-----------------------------------------------------------------------------
int8_t WDT_init(void)
{
	/* 8K cycles (8.2s) */
	/* Off */
	ccp_write_io((void *)&(WDT.CTRLA), WDT_PERIOD_8KCLK_gc | WDT_WINDOW_OFF_gc );  
	return 0;
}
//-----------------------------------------------------------------------------
int8_t CLKCTRL_init(void)
{
	// Configuro el clock para 24Mhz
	
	ccp_write_io((void *)&(CLKCTRL.OSCHFCTRLA), CLKCTRL_FRQSEL_24M_gc         /* 24 */
	| 0 << CLKCTRL_AUTOTUNE_bp /* Auto-Tune enable: disabled */
	| 0 << CLKCTRL_RUNSTDBY_bp /* Run standby: disabled */);

	// ccp_write_io((void*)&(CLKCTRL.MCLKCTRLA),CLKCTRL_CLKSEL_OSCHF_gc /* Internal high-frequency oscillator */
	//		 | 0 << CLKCTRL_CLKOUT_bp /* System clock out: disabled */);

	// ccp_write_io((void*)&(CLKCTRL.MCLKLOCK),0 << CLKCTRL_LOCKEN_bp /* lock enable: disabled */);

	return 0;
}
//-----------------------------------------------------------------------------
void reset(void)
{
    xprintf_P(PSTR("ALERT !!!. Going to reset...\r\n"));
    vTaskDelay( ( TickType_t)( 100 / portTICK_PERIOD_MS ) );
	/* Issue a Software Reset to initilize the CPU */
	ccp_write_io( (void *)&(RSTCTRL.SWRR), RSTCTRL_SWRST_bm ); 
                                           
}
//------------------------------------------------------------------------------
void reset_memory_remote(void)
{
    /*
     * Desde el servidor podemos mandar resetear la memoria cuando detectamos
     * problemas como fecha/hora en 0 o valores incorrectos.
     * Se debe mandar 'RESMEM'
     */
          
    vTaskSuspend( xHandle_tkSys );
    vTaskSuspend( xHandle_tkRS485A );
    vTaskSuspend( xHandle_tkRS485B );
    //vTaskSuspend( xHandle_tkWAN );
        

    FS_format(true);
    
    xprintf("Reset..\r\n");
    reset();
    
}
//------------------------------------------------------------------------------
void kick_wdt( uint8_t bit_pos)
{
    // Pone el bit correspondiente en 0.
    sys_watchdog &= ~ (1 << bit_pos);
    
}
//------------------------------------------------------------------------------
void config_default(void)
{

    // Configuro a default todas las configuraciones locales
    // y luego actualizo el systemConf
    
    systemConf.wan_port = WAN_RS485B;
    memcpy(systemConf.dlgid, "DEFAULT\0", sizeof(systemConf.dlgid));
    
    systemConf.timerpoll = 60;
    systemConf.timerdial = 0;
    
    systemConf.alarm_level = 0;
    systemConf.samples_count = 1;
    
    systemConf.pwr_modo = PWR_CONTINUO;
    systemConf.pwr_hhmm_on = 2330;
    systemConf.pwr_hhmm_off = 630;
 
    // Actualizo las configuraciones locales a default
    ainputs_config_defaults(&systemConf.ainputs_conf);
    counters_config_defaults(&systemConf.counters_conf);
    modbus_config_defaults(&systemConf.modbus_conf);
    
#ifdef PILOTO
    piloto_config_defaults();
#endif
    
    // Actualizo las configuraciones locales en el systemConf
//    modbus_read_local_config(&systemConf.modbus_conf);
    
#ifdef PILOTO
    piloto_read_local_config(&systemConf.piloto_conf);
#endif
    
}
//------------------------------------------------------------------------------
bool config_debug( char *tipo, char *valor)
{
    /*
     * Configura las flags de debug para ayudar a visualizar los problemas
     */
    
    if (!strcmp_P( strupr(tipo), PSTR("NONE")) ) {
        ainputs_config_debug(false);
        counters_config_debug(false);
        modbus_config_debug(false);
        WAN_config_debug(false);
        piloto_config_debug(false);
        return(true); 
    }

    if (!strcmp_P( strupr(tipo), PSTR("PILOTO")) ) {
        if (!strcmp_P( strupr(valor), PSTR("TRUE")) ) {
            piloto_config_debug(true);
            return(true);
        }
        if (!strcmp_P( strupr(valor), PSTR("FALSE")) ) {
            piloto_config_debug(false);
            return(true);
        }
    }
    
    if (!strcmp_P( strupr(tipo), PSTR("MODBUS")) ) {
        if (!strcmp_P( strupr(valor), PSTR("TRUE")) ) {
            modbus_config_debug(true);
            return(true);
        }
        if (!strcmp_P( strupr(valor), PSTR("FALSE")) ) {
            modbus_config_debug(false);
            return(true);
        }
    }
    
    if (!strcmp_P( strupr(tipo), PSTR("ANALOG")) ) {
        if (!strcmp_P( strupr(valor), PSTR("TRUE")) ) {
            ainputs_config_debug(true);
            return(true);
        }
        if (!strcmp_P( strupr(valor), PSTR("FALSE")) ) {
            ainputs_config_debug(false);
            return(true);
        }
    }

    if (!strcmp_P( strupr(tipo), PSTR("COUNTERS")) ) {
        if (!strcmp_P( strupr(valor), PSTR("TRUE")) ) {
            counters_config_debug(true);
            return(true);
        }
        if (!strcmp_P( strupr(valor), PSTR("FALSE")) ) {
            counters_config_debug(false);
            return(true);
        }
    }
    
    if (!strcmp_P( strupr(tipo), PSTR("COMMS")) ) {
        if (!strcmp_P( strupr(valor), PSTR("TRUE")) ) {
            WAN_config_debug(true);
            return(true);
        }
        if (!strcmp_P( strupr(valor), PSTR("FALSE")) ) {
            WAN_config_debug(false);
            return(true);
        }
    }
    
    return(false);
    
}
//------------------------------------------------------------------------------
bool save_config_in_NVM(void)
{
   
int8_t retVal;
uint8_t cks;

    cks = checksum ( (uint8_t *)&systemConf, ( sizeof(systemConf) - 1));
    systemConf.checksum = cks;
    
    retVal = NVMEE_write( 0x00, (char *)&systemConf, sizeof(systemConf) );
    
    //xprintf_P(PSTR("DEBUG: Save in NVM OK\r\n"));
    
    if (retVal == -1 )
        return(false);
    
    return(true);
   
}
//------------------------------------------------------------------------------
bool load_config_from_NVM(void)
{

uint8_t rd_cks, calc_cks;
    
    NVMEE_read( 0x00, (char *)&systemConf, sizeof(systemConf) );
    rd_cks = systemConf.checksum;
    
    calc_cks = checksum ( (uint8_t *)&systemConf, ( sizeof(systemConf) - 1));
    
    if ( calc_cks != rd_cks ) {
		xprintf_P( PSTR("ERROR: Checksum systemVars failed: calc[0x%0x], read[0x%0x]\r\n"), calc_cks, rd_cks );
        
		return(false);
	}
    
    return(true);
}
//------------------------------------------------------------------------------
uint8_t checksum( uint8_t *s, uint16_t size )
{
	/*
	 * Recibe un puntero a una estructura y un tama�o.
	 * Recorre la estructura en forma lineal y calcula el checksum
	 */

uint8_t *p = NULL;
uint8_t cks = 0;
uint16_t i = 0;

	cks = 0;
	p = s;
	for ( i = 0; i < size ; i++) {
		 cks = (cks + (int)(p[i])) % 256;
	}

	return(cks);
}
//------------------------------------------------------------------------------
bool config_wan_port(char *comms_type)
{
    if ((strcmp_P( strupr(comms_type), PSTR("RS485")) == 0) ) {
        systemConf.wan_port = WAN_RS485B;
        return(true);
    }
    
    if ((strcmp_P( strupr(comms_type), PSTR("NBIOT")) == 0) ) {
        systemConf.wan_port = WAN_NBIOT;
        return(true);
    }
    return(false);
    
}
//------------------------------------------------------------------------------
void data_resync_clock( char *str_time, bool force_adjust)
{
	/*
	 * Ajusta el clock interno de acuerdo al valor de rtc_s
	 * Bug 01: 2021-12-14:
	 * El ajuste no considera los segundos entonces si el timerpoll es c/15s, cada 15s
	 * se reajusta y cambia la hora del datalogger.
	 * Modifico para que el reajuste se haga si hay una diferencia de mas de 90s entre
	 * el reloj local y el del server
	 */


float diff_seconds;
RtcTimeType_t rtc_l, rtc_wan;
int8_t xBytes = 0;
   
    // Convierto el string YYMMDDHHMM a RTC.
    //xprintf_P(PSTR("DATA: DEBUG CLOCK2\r\n") );
    memset( &rtc_wan, '\0', sizeof(rtc_wan) );        
    RTC_str2rtc( str_time, &rtc_wan);
    //xprintf_P(PSTR("DATA: DEBUG CLOCK3\r\n") );
            
            
	if ( force_adjust ) {
		// Fuerzo el ajuste.( al comienzo )
		xBytes = RTC_write_dtime(&rtc_wan);		// Grabo el RTC
		if ( xBytes == -1 ) {
			xprintf_P(PSTR("ERROR: CLOCK: I2C:RTC:pv_process_server_clock\r\n"));
		} else {
			xprintf_P( PSTR("CLOCK: Update rtc.\r\n") );
		}
		return;
	}

	// Solo ajusto si la diferencia es mayor de 90s
	// Veo la diferencia de segundos entre ambos.
	// Asumo yy,mm,dd iguales
	// Leo la hora actual del datalogger
	RTC_read_dtime( &rtc_l);
	diff_seconds = abs( rtc_l.hour * 3600 + rtc_l.min * 60 + rtc_l.sec - ( rtc_wan.hour * 3600 + rtc_wan.min * 60 + rtc_wan.sec));
	//xprintf_P( PSTR("COMMS: rtc diff=%.01f\r\n"), diff_seconds );

	if ( diff_seconds > 90 ) {
		// Ajusto
		xBytes = RTC_write_dtime(&rtc_wan);		// Grabo el RTC
		if ( xBytes == -1 ) {
			xprintf_P(PSTR("ERROR: CLOCK: I2C:RTC:pv_process_server_clock\r\n"));
		} else {
			xprintf_P( PSTR("CLOCK: Update rtc\r\n") );
		}
		return;
	}
}
//------------------------------------------------------------------------------
bool config_timerdial ( char *s_timerdial )
{
	// El timer dial puede ser 0 si vamos a trabajar en modo continuo o mayor a
	// 15 minutos.
	// Es una variable de 32 bits para almacenar los segundos de 24hs.

uint16_t l_timerdial;
    
    l_timerdial = atoi(s_timerdial);
    if ( (l_timerdial > 0) && (l_timerdial < TDIAL_MIN_DISCRETO ) ) {
        xprintf_P( PSTR("TDIAL warn: continuo TDIAL=0, discreto TDIAL >= 900)\r\n"));
        l_timerdial = TDIAL_MIN_DISCRETO;
    }
    
	systemConf.timerdial = atoi(s_timerdial);
	return(true);
}
//------------------------------------------------------------------------------
bool config_timerpoll ( char *s_timerpoll )
{
	// Configura el tiempo de poleo.
	// Se utiliza desde el modo comando como desde el modo online
	// El tiempo de poleo debe estar entre 15s y 3600s


	systemConf.timerpoll = atoi(s_timerpoll);

	if ( systemConf.timerpoll < 15 )
		systemConf.timerpoll = 15;

	if ( systemConf.timerpoll > 3600 )
		systemConf.timerpoll = 300;

	return(true);
}
//------------------------------------------------------------------------------
bool config_pwrmodo ( char *s_pwrmodo )
{
    if ((strcmp_P( strupr(s_pwrmodo), PSTR("CONTINUO")) == 0) ) {
        systemConf.pwr_modo = PWR_CONTINUO;
        return(true);
    }
    
    if ((strcmp_P( strupr(s_pwrmodo), PSTR("DISCRETO")) == 0) ) {
        systemConf.pwr_modo = PWR_DISCRETO;
        return(true);
    }
    
    if ((strcmp_P( strupr(s_pwrmodo), PSTR("MIXTO")) == 0) ) {
        systemConf.pwr_modo = PWR_MIXTO;
        return(true);
    }
    
    return(false);
}
//------------------------------------------------------------------------------
bool config_pwron ( char *s_pwron )
{
    systemConf.pwr_hhmm_on = atoi(s_pwron);
    return(true);
}
//------------------------------------------------------------------------------
bool config_pwroff ( char *s_pwroff )
{
    systemConf.pwr_hhmm_off = atoi(s_pwroff);
    return(true);
}
//------------------------------------------------------------------------------
void print_pwr_configuration(void)
{
    /*
     * Muestra en pantalla el modo de energia configurado
     */
    
uint16_t hh, mm;
    
    switch( systemConf.pwr_modo ) {
        case PWR_CONTINUO:
            xprintf_P(PSTR(" pwr_modo: continuo\r\n"));
            break;
        case PWR_DISCRETO:
            xprintf_P(PSTR(" pwr_modo: discreto (%d s)\r\n"), systemConf.timerdial);
            break;
        case PWR_MIXTO:
            xprintf_P(PSTR(" pwr_modo: mixto\r\n"));
            hh = (uint8_t)(systemConf.pwr_hhmm_on / 100);
            mm = (uint8_t)(systemConf.pwr_hhmm_on % 100);
            xprintf_P(PSTR("    inicio continuo -> %02d:%02d\r\n"), hh,mm);
            
            hh = (uint8_t)(systemConf.pwr_hhmm_off / 100);
            mm = (uint8_t)(systemConf.pwr_hhmm_off % 100);
            xprintf_P(PSTR("    inicio discreto -> %02d:%02d\r\n"), hh,mm);
            break;
    }
    xprintf_P(PSTR(" pwr_on:%d, pwr_off:%d\r\n"),systemConf.pwr_hhmm_on, systemConf.pwr_hhmm_off );
}
//------------------------------------------------------------------------------
void xprint_dr(dataRcd_s *dr)
{
    /*
     * Imprime en pantalla el dataRcd pasado
     */
    
uint8_t i, channel;


    xprintf_P( PSTR("ID:%s;TYPE:%s;VER:%s;"), systemConf.dlgid, FW_TYPE, FW_REV);
 
    // Clock
    xprintf_P( PSTR("DATE:%02d%02d%02d;"), dr->rtc.year, dr->rtc.month, dr->rtc.day );
    xprintf_P( PSTR("TIME:%02d%02d%02d;"), dr->rtc.hour, dr->rtc.min, dr->rtc.sec);
    
    // Analog Channels:
    for ( i=0; i < NRO_ANALOG_CHANNELS; i++) {
        //if ( strcmp ( systemConf.ainputs_conf[channel].name, "X" ) != 0 ) {
        if ( systemConf.ainputs_conf.channel[i].enabled ) {
            xprintf_P( PSTR("%s:%0.2f;"), systemConf.ainputs_conf.channel[i].name, dr->l_ainputs[i]);
        }
    }
        
    // Counter Channels:
    for ( channel=0; channel < NRO_COUNTER_CHANNELS; channel++) {
        if ( strcmp ( systemConf.counters_conf.channel[channel].name, "X" ) != 0 ) {
            xprintf_P( PSTR("%s:%0.3f;"), systemConf.counters_conf.channel[channel].name, dr->l_counters[channel]);
        }
    }

    // Modbus Channels:
    if (systemConf.modbus_conf.enabled) {
        for ( channel=0; channel < NRO_MODBUS_CHANNELS; channel++) {
            if ( systemConf.modbus_conf.mbch[channel].enabled ) {
                xprintf_P( PSTR("%s:%0.3f;"), systemConf.modbus_conf.mbch[channel].name, dr->l_modbus[channel]);
            }
        }
    }
       
    // Battery
    xprintf_P( PSTR("bt:%0.2f;"), dr->battery);
    
    xprintf_P( PSTR("\r\n"));
}
//------------------------------------------------------------------------------
bool xprint_from_dump(char *buff, bool f_dummy)
{
    /*
     * Funcion pasada a FS_dump() para que formatee el buffer y lo imprima
     * como un dr.
     */
dataRcd_s dr;


/*
uint8_t j;

    for (j=0; j<FF_RECD_SIZE; j++) {
        if ( (j%8) == 0 ) {
            xprintf_P(PSTR("\r\n%02d: "),j);
        }
        xprintf_P(PSTR("[0x%02x] "), buff[j]);
    }
    xprintf_P(PSTR("\r\n"));
*/

    memcpy ( &dr, buff, sizeof(dataRcd_s) );
    xprint_dr(&dr);
    return(true);
}
//------------------------------------------------------------------------------
bool config_samples ( char *s_samples )
{
	// Configura el numero de muestras de c/poleo.
	// Debe ser mayor a 1 y menor a 20.


	systemConf.samples_count= atoi(s_samples);

	if ( systemConf.samples_count < 1 ) {
		systemConf.samples_count = 1;
        xprintf_P(PSTR("ALERT: samples default to 1 !!!\r\n"));
    }
    

	if ( systemConf.samples_count > 10 ) {
		systemConf.samples_count = 10;
        xprintf_P(PSTR("ALERT: samples default to 10 !!!\r\n"));
    }

	return(true);
}
//------------------------------------------------------------------------------
bool config_almlevel ( char *s_almlevel )
{
	// Configura el nivel de disparo de alarmas.
	// Se utiliza desde el modo discreto e indica el porcentaje del valor
    // de la medida.
    // Varia entre 0 y 100


	systemConf.alarm_level = atoi(s_almlevel);

	if ( systemConf.alarm_level < 0 )
		systemConf.alarm_level = 0;

	if ( systemConf.alarm_level > 100 )
		systemConf.alarm_level = 100;

	return(true);
}
//------------------------------------------------------------------------------
uint8_t confbase_hash(void)
{
   
uint8_t hash_buffer[32];
uint8_t hash = 0;
char *p;

    // Calculo el hash de la configuracion base
    memset(hash_buffer, '\0', sizeof(hash_buffer));
    sprintf_P( (char *)&hash_buffer, PSTR("[TIMERPOLL:%03d]"), systemConf.timerpoll );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    //
    memset(hash_buffer, '\0', sizeof(hash_buffer));
    sprintf_P( (char *)&hash_buffer, PSTR("[TIMERDIAL:%03d]"), systemConf.timerdial );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );    
    //
    memset(hash_buffer, '\0', sizeof(hash_buffer));
    sprintf_P( (char *)&hash_buffer, PSTR("[PWRMODO:%d]"), systemConf.pwr_modo );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    //
    memset(hash_buffer, '\0', sizeof(hash_buffer));
    sprintf_P( (char *)&hash_buffer, PSTR("[PWRON:%04d]"), systemConf.pwr_hhmm_on );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    //
    memset(hash_buffer, '\0', sizeof(hash_buffer));
    sprintf_P( (char *)&hash_buffer, PSTR("[PWROFF:%04d]"), systemConf.pwr_hhmm_off );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    //
    memset(hash_buffer, '\0', sizeof(hash_buffer));
    sprintf_P( (char *)&hash_buffer, PSTR("[SAMPLES:%02d]"), systemConf.samples_count );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    //
    memset(hash_buffer, '\0', sizeof(hash_buffer));
    sprintf_P( (char *)&hash_buffer, PSTR("[ALMLEVEL:%02d]"), systemConf.alarm_level );
    p = (char *)hash_buffer;
    while (*p != '\0') {
		hash = u_hash(hash, *p++);
	}  
    //xprintf_P(PSTR("HASH_BASE:<%s>, hash=%d\r\n"),hash_buffer, hash );
    
    return(hash);
}
//------------------------------------------------------------------------------

void u_check_stacks_usage(void)
{
    /*
     * Mide el stack de todas las tareas y lo informa
     */
    
uint16_t uxHighWaterMark;

    uxHighWaterMark = SPYuxTaskGetStackHighWaterMark( xHandle_tkCmd );
    xprintf_P(PSTR("tkCMD stack = %d\r\n"), uxHighWaterMark );

    uxHighWaterMark = SPYuxTaskGetStackHighWaterMark( xHandle_tkSys );
    xprintf_P(PSTR("tkSYS stack = %d\r\n"), uxHighWaterMark );
    
    uxHighWaterMark = SPYuxTaskGetStackHighWaterMark( xHandle_tkRS485A );
    xprintf_P(PSTR("tkRS485A stack = %d\r\n"), uxHighWaterMark );

    uxHighWaterMark = SPYuxTaskGetStackHighWaterMark( xHandle_tkRS485B );
    xprintf_P(PSTR("tkRS485B stack = %d\r\n"), uxHighWaterMark );

    uxHighWaterMark = SPYuxTaskGetStackHighWaterMark( xHandle_tkWAN );
    xprintf_P(PSTR("tkWAN stack = %d\r\n"), uxHighWaterMark );
    
    uxHighWaterMark = SPYuxTaskGetStackHighWaterMark( xHandle_tkPILOTO );
    xprintf_P(PSTR("tkPILOTO stack = %d\r\n"), uxHighWaterMark );
    
}
//------------------------------------------------------------------------------
