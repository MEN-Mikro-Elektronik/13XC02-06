/***********************  I n c l u d e  -  F i l e  ***********************/
/*!
 *        \file  xc02_drv.h
 *
 *      \author  thomas.schnuerer@men.de
 *
 *       \brief  Header file for XC02 driver containing
 *               MDIS side Get/SetStats. PIC-side commands are in xc02.h
 *
 *
 *    \switches  _ONE_NAMESPACE_PER_DRIVER_
 *               _LL_DRV_
 */
 /*
 *---------------------------------------------------------------------------
 * Copyright 2019, MEN Mikro Elektronik GmbH
 ****************************************************************************/
/*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _XC02_DRV_H
#define _XC02_DRV_H

#ifdef __cplusplus
      extern "C" {
#endif

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/** structure for #XC02_BLK_DOWN_SIG_SET setstat */
typedef struct {
	u_int32 msec;			/* poll period [msec] */
	u_int32 signal;			/* signal to send */
} XC02_BLK_DOWN_SIG;

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/** \name DC1 specific Getstat/Setstat codes
 *  \anchor getstat_setstat_codes
 */
/**@{*/

#define XC02_WOT	    		 M_DEV_OF+0x01   /**<G,S: Wake ON Time (minutes, 0=no WOT)\n
								 				   Values: 0..65535\n
								 				   Default: 0\n */
#define XC02_WDOG_ERR			 M_DEV_OF+0x02   /**<G  : Missing wdog triggers after the last shutdown\n
								 				   Values: 0..n\n
								 				   Default: 0\n */
#define XC02_SWOFF				 M_DEV_OF+0x03   /**<  S: Initiate a software shutdown\n */
#define XC02_OFFACK				 M_DEV_OF+0x04   /**<  S: Send OFF Acknowledge\n */
#define XC02_DOWN_DELAY			 M_DEV_OF+0x05   /**<G,S: Shutdown delay mode\n
								 				   Values: see \ref _XC02_DOWN_DELAY\n
								 				   Default: 0 (no delay)\n*/
#define XC02_OFF_DELAY			 M_DEV_OF+0x06   /**<G,S: Off delay mode\n
								 				   Values: see \ref _XC02_OFF_DELAY\n
								 				   Default: 0 (no delay)\n*/
#define XC02_DOWN_EVT			 M_DEV_OF+0x07   /**<G  : Shutdown event flag\n
								 			         Values: 0,1\n
								 			         Default: 0\n */
#define XC02_DOWN_SIG_CLR 		 M_DEV_OF+0x08 	 /**<  S: Clear signal for Shutdown
								 				     Event\n */
#define XC02_IN					 M_DEV_OF+0x09   /**<G  : Binary inputs state\n
								 			         Values:Bit 4..1: GA[3:0] Bit 0: KEY\n
								 			         1=set, 0=not set\n */
#define XC02_TEMP				 M_DEV_OF+0x0a   /**<G  : Temperature [°C]\n*/
#define XC02_TEMP_HIGH			 M_DEV_OF+0x0b   /**<G,S: Temp high limit [°C]
								 			         Default: 60°C\n */
#define XC02_TEMP_LOW   		 M_DEV_OF+0x0c   /**<G,S: Temp low limit [°C]
                            	                      Default: -10°C\n */
#define XC02_BRIGHTNESS   		 M_DEV_OF+0x0d 	 /**<G,S: primary screen brightness */

#define XC02_BR_SRC		   		 M_DEV_OF+0x0e 	 /**<G,S: Brightness control source */
#define XC02_SW_DISP	   		 M_DEV_OF+0x0f 	 /**<G,S: switch display on/off */
#define XC02_DISP_INITSTAT 		 M_DEV_OF+0x10 	 /**<G,S: set initial display state:\n
								 			          1: ON, BIOS etc. shown\n
								 			          0: OFF, display switch by\n
								 			             User application */
#define XC02_TIMESTAMP   		 M_DEV_OF+0x11   /**<G  : firmware build date/time (returns every char, to\n
								 				      be executed until 0xff is returned) */
#define XC02_VOLTAGE     		 M_DEV_OF+0x12   /**<G  : display supply voltage\n
								 				   ( 12V scaled by voltage divider (2k7/12k7),\n
								 				   default: 12V * (2,7/12,7) = 2550 mV*/
#define XC02_VOLT_LOW  			 M_DEV_OF+0x13   /**<G  : low volt limit for display protect \n
								 				   (measured through voltage divider,
								 				   can be set with system descriptor only)*/
#define XC02_VOLT_HIGH			 M_DEV_OF+0x14   /**<G  : high volt limit for display protect \n
													(measured through voltage divider,
													can be set with system descriptor only) */

#define XC02_INIT_BRIGHT 		 M_DEV_OF+0x19   /**<G,S: Initial brightness at startup */
#define XC02_BRIGHTNESS2    	 M_DEV_OF+0x1a 	 /**<G,S: 2nd screen brightness (DC1 R01 only) */
#define XC02_MINICARD_PWR  		 M_DEV_OF+0x1b 	 /**<G,S: Minicard slot power on/off (DC1 R01 only) */
#define XC02_BR_OFFS 			 M_DEV_OF+0x1c   /**<G,S: offset added to foto sensor value from ADC in auto mode */
#define XC02_BR_MULT 			 M_DEV_OF+0x1d   /**<G,S: multiplier for foto sensor value (x0.1 steps) in auto mode */
#define SC21_BL_CURRENT 		 M_DEV_OF+0x1e   /**<G  : SC21 only: drawn power of backlight */
#define XC02_BRIGHT_DIRECTION    M_DEV_OF+0x1f   /**<G,S: brightness direction control */
#define XC02_RAW_BRIGHTNESS      M_DEV_OF+0x20   /**<G  : photo sensor raw ADC value */
#define XC02_KEY_IN_CTRL  		 M_DEV_OF+0x21   /**<G,S: set KEY_IN control behavior\n*/
#define XC02_AUTO_BRIGHT_CTRL 	 M_DEV_OF+0x22   /**<G,S: set auto brightness behavior\n*/



#ifndef _DOXYGEN_
/* Test registers for PIC firmware test */
#define XC02_TEST1	M_DEV_OF+0x15   /**<G,S: Test register #1 */
#define XC02_TEST2	M_DEV_OF+0x16   /**<G,S: Test register #2 */
#define XC02_TEST3	M_DEV_OF+0x17   /**<G  : Test register #3 */
#define XC02_TEST4	M_DEV_OF+0x18   /**<G  : Test register #3 */
#endif



/* DC1 specific Getstat/Setstat block codes */
#define XC02_BLK_DOWN_SIG_SET	M_DEV_BLK_OF+0x00 /**< S: Install signal for \n
													 shutdown event. Values:\n
													 see #XC02_BLK_DOWN_SIG
													 structure. Default: \n
													 0 (no signal)\n */
/**@}*/

#ifndef  XC02_VARIANT
# define XC02_VARIANT XC02
#endif

/** \name XC02 miscellaneous defines for Temperature and voltage conversion
 *  \anchor miscellaneous_defines
 */
/**@{*/

#define TEMP_RANGE_MIN		-40	   /**<  minimal operating temperature */
#define TEMP_RANGE_MAX		100	   /**<  maximal operating temperature */
#define ADC_REF_3V			3000   /**< 3V (3000mV) is the ADC Reference */

#define  XC02_ADC2TEMP(ADC)    (((ADC_REF_3V*(ADC)/255)-500)/10) /**<  convert raw ADC value to °C , see LM50 datasheet */

#define  XC02_TEMP2ADC(T)    ((((10*(T))+500)*255)/ADC_REF_3V) /**<  convert °C to raw ADC value, see LM50 datasheet */
#define  XC02_ADC2VOLT(ADC)  (ADC_REF_3V*(ADC)/255) /**<  convert ADC value to Voltage */
#define  SC21_ADC2POWER(ADC) ((72*(ADC))/255) /**<  convert ADC value to 12V power */

/**@}*/

/** \name XC02 default defines for Display operating Temp and voltage ranges
 *  \anchor default_defines
 */
/**@{*/

#define TEMP_DEF_MIN XC02_TEMP2ADC(-10) /**< display low temperature */
#define TEMP_DEF_MAX XC02_TEMP2ADC(60)  /**< display high temperature */
#define VOLT_DEF_MIN 180  /**< low operate voltage ( 10V at voltage divider) */
#define VOLT_DEF_MAX 249  /**< high operate voltage ( 14V at voltage divider) */
/**@}*/

# define _XC02_GLOBNAME(var,name) var##_##name

#ifndef _ONE_NAMESPACE_PER_DRIVER_
# define XC02_GLOBNAME(var,name) _XC02_GLOBNAME(var,name)
#else
# define XC02_GLOBNAME(var,name) _XC02_GLOBNAME(XC02,name)
#endif

#define __XC02_GetEntry    XC02_GLOBNAME(XC02_VARIANT,GetEntry)


/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
#ifdef _LL_DRV_
#ifndef _ONE_NAMESPACE_PER_DRIVER_
	extern void __XC02_GetEntry(LL_ENTRY* drvP);
#endif
#endif /* _LL_DRV_ */

/*-----------------------------------------+
|  BACKWARD COMPATIBILITY TO MDIS4         |
+-----------------------------------------*/
#ifndef U_INT32_OR_64
 /* we have an MDIS4 men_types.h and mdis_api.h included */
 /* only 32bit compatibility needed!                     */
 #define INT32_OR_64  int32
 #define U_INT32_OR_64 u_int32
 typedef INT32_OR_64  MDIS_PATH;
#endif /* U_INT32_OR_64 */


#ifdef __cplusplus
      }
#endif

#endif /* _XC02_DRV_H */

