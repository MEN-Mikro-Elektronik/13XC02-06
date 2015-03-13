/***********************  I n c l u d e  -  F i l e  ***********************/
/*!
 *        \file  wdog.h
 *
 *      \author  dp
 *        $Date: 2006/05/26 15:08:34 $
 *    $Revision: 2.6 $
 *
 *       \brief  Header file for Watchdog drivers
 *               - Watchdog specific status codes
 *
 *    \switches  ---
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: wdog.h,v $
 * Revision 2.6  2006/05/26 15:08:34  DPfeuffer
 * cosmetics
 *
 * Revision 2.5  2005/12/22 14:42:35  dpfeuffer
 * comments now in doxygen style
 *
 * Revision 2.4  2004/09/02 13:41:42  AJendrusch
 * cosmetics
 *
 * Revision 2.3  2000/05/11 13:13:23  Schmidt
 * Set-/GetStat codes mapped to low-level driver profile range
 *
 * Revision 2.2  1999/08/03 16:42:02  Schmidt
 * WDOG_SHOT code added
 *
 * Revision 2.1  1999/06/23 12:03:25  Schmidt
 * Initial Revision
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 1999 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#ifndef _WDOG_H
#define _WDOG_H

#ifdef __cplusplus
      extern "C" {
#endif

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
#define M_CH_PROFILE_WDOG	200		/**< profile type (returned in M_LL_CH_TYP */

/** \name Watchdog specific Getstat/Setstat standard codes 
 *  \anchor wdog_getstat_setstat_codes
 */
/**@{*/
#define WDOG_START		M_DEV_OF+0xe0		/**<  S: Start Watchdog					*/
#define WDOG_TRIG		M_DEV_OF+0xe1		/**<  S: Trigger Watchdog				*/
#define WDOG_STOP		M_DEV_OF+0xe2		/**<  S: Stop Watchdog					*/
#define WDOG_TIME		M_DEV_OF+0xe3		/**<G,S: Get-/Set Watchdog Time (ms)	    */
#define WDOG_STATUS		M_DEV_OF+0xe4		/**<G  : Get Status (0:deaktiv, 1:aktiv) */
#define WDOG_SHOT		M_DEV_OF+0xe5		/**<G  : 0: no wdog shot, 1: wdog shot, 
                                                     255 = not identifiable */ 
/**@}*/

#ifdef __cplusplus
      }
#endif

#endif /* WDOG_H */




