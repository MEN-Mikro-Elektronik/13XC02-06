/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  xc02_doc.c
 *
 *      \author  ts
 *
 *      \brief   User documentation for XC02BC driver
 *
 *     Required: -
 *
 *     \switches -
 */
 /*
 *---------------------------------------------------------------------------
 * Copyright (c) 2005-2019, MEN Mikro Elektronik GmbH
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

/*! \mainpage
    This is the documentation of the MDIS5 XC2 board controller driver that
    supports the features that are specific to the XC2 carrier board inside
    the DC1	display computer. It allows controlling the integrated TFT panel
    with various SetStat codes.

    The driver supports the following HW features:\n
	- onboard CPU independent system watchdog for additional security
    - panel protection and brightness control settings
	- reading the 4 GPIO pins which allow encoding e.g. a geographical address
    - programmable shutdown delay and startup times
    \n

    Notes:\n
	- The driver supports the standard watchdog functionality as provided
      by other MEN boards, e.g. 08AD78.
    - MDIS5 32bit drivers are compatible to the MDIS4 drivers but must not
      be mixed with MDIS4 drivers at one target system.


    \n \section FuncDesc Functional Description

    \n \subsection General General
	The driver supports the basic watchdog functions (start, stop, trigger
	and state), reading the carrier board's temperature and monitored Voltage
	of the DC/DC converters output.
	The limits for low/high temperature and low/high voltage are set through
	descriptors, since these need to be set just once usually after deploy of
	DC1 in the field.

	When the first path is opened to the XC02BC device, the HW and the driver
	are being initialized with default values. Under linux additional delay
	may occur if its the first open of the SMBus channel. Mind the different
	SMBus addresses used for linux (0x09) and windows (0x12). This results from
	the different treatment of SMB addresses under linux and windows: while
	linux treats the value as raw 7 bit value, the SMB address under windows
    contains the LSB that encodes if its a read or write access. So the SMB
	address is shifted left by one bit.

    \n \subsection channels Logical channels
    The driver provides 4 logical channels from which currently only 2
	are used, they represent the 2 possible displays that can be controlled
	from the XC2. For example, when initial brightness is set, the display
	for which the brightness is to be set has to be selected first by setting
	the MDIS channel to 0 or 1.\n

    \n \section api_functions Supported API Functions

    <table border="0">
    <tr>
        <td><b>API function</b></td>
        <td><b>Functionality</b></td>
        <td><b>Corresponding low level function</b></td></tr>

    <tr><td>M_open()</td><td>Open device</td>
	<td>XC02_Init()</td></tr>
    <tr><td>M_close()     </td><td>Close device             </td>
    <td>XC02_Exit())</td></tr>
    <tr><td>M_setstat()   </td><td>Set device parameter     </td>
    <td>XC02_SetStat()</td></tr>
    <tr><td>M_getstat()   </td><td>Get device parameter     </td>
    <td>XC02_GetStat()</td></tr>
    <tr><td>M_errstringTs() </td><td>Generate error message </td>
    <td>-</td></tr>
    </table>

    \n \section descriptor_entries Descriptor Entries

    The low-level driver initialization routine decodes the following entries
    ("keys") in addition to the general descriptor keys:

    <table border="0">
    <tr><td><b>Descriptor entry</b></td>
        <td><b>Description</b></td>
        <td><b>Values</b></td>
        <td><b>required/optional</b></td>

    </tr>
    <tr><td>SMB_BUSNBR</td>
        <td>SMBus bus number</td>
        <td>0, 1, 2, ...\n
			Default: 0</td>
		<td>required</td>
    </tr>

    <tr><td>SMB_DEVADDR</td>
        <td>SMBus address of the XC02 board controller</td>
        <td>0x00, 0xff\n
			Default linux: 0x09 windows: 0x12</td>
		<td>required</td>
    </tr>

    <tr><td>WDOG_TOUT</td>
        <td>Watchdog timeout in 100ms units</td>
        <td>1..255 [100ms]\n
			Default: 255</td>
		<td>optional</td>
    </tr>
    <tr><td>DOWN_DELAY</td>
        <td>Shutdown delay mode</td>
        <td>see \ref _XC02_DOWN_DELAY\n
			Default: 0 (no delay)</td>
		<td>optional</td>
    </tr>
    <tr><td>OFF_DELAY</td>
        <td>Off delay mode</td>
        <td>see \ref _XC02_OFF_DELAY\n
			Default: 0 (no delay)</td>
		<td>optional</td>
    </tr>
    <tr><td>TEMP_HIGH</td>
        <td>Temperature high limit for display</td>
        <td>Default: 60°C</td>
		<td>optional</td>
    </tr>
    <tr><td>TEMP_LOW</td>
        <td>Temperature low limit for display</td>
        <td>Default: -10°C</td>
		<td>optional</td>
    </tr>
    <tr><td>VOLT_HIGH</td>
        <td>display supply high voltage monitoring limit\n
		    before display switch off</td>
        <td>Default: 14 V</td>
		<td>optional</td>
    </tr>
    <tr><td>VOLT_LOW</td>
        <td>display supply low voltage monitoring limit\n
		    before display switch off</td>
        <td>Default: 10V</td>
		<td>optional</td>
    </tr>
    <tr><td>BRIGHT_SOURCE</td>
        <td>display brightness control source \n
		    before display switch off</td>
        <td>0: use value passed by SetStat XC02_BRIGHTNESS\n
		    1: use automatic setting through photoelement (Option)\n
			Default: 0</td>
		<td>optional</td>
    </tr>
    <tr><td>INIT_DISPSTAT</td>
        <td>initial display status after powerup \n
		    </td>
        <td>0: display 1 off,   display 2 (optional) off\n
		    1: display 1 on,    display 2 (optional) off\n
		    2: display 1 off,   display 2 (optional) on\n
		    3: display 1 on,    display 2 (optional) on\n
			Default: 1</td>
		<td>optional</td>
    </tr>
    <tr><td>INIT_BRIGHT_1</td>
        <td>initial brightness of display 1 on powerup</td>
		<td>Default: 100 (50%) Attention: 0=most bright, 200=most dark</td>
		<td>optional</td>
    </tr>
    <tr><td>INIT_BRIGHT_2</td>
        <td>initial brightness of optional display 2 on powerup\n
		    </td>
		<td>Default: 100 (50%) Attention: 0=most bright, 200=most dark</td>
		<td>optional</td>
    </tr>
    </table>

	\attention changes made in the descriptors occur after next power cycle.
	The brightness values given above are in the range of 0 to 200, where
	0 represents the most bright setting and 200 the most dark, dimmed setting.


    \n \section codes XC02 board controller specific GetStat/SetStat codes
	For the watchdog functionality, the driver supports a subset of the WDOG
	GetStat/SetStat codes (defined in wdog.h):
	- #WDOG_START
	- #WDOG_STOP
	- #WDOG_TRIG
	- #WDOG_STATUS\n

	The other driver functionality is supported by the XC02 GetStat/SetStat
	codes.
	See \ref getstat_setstat_codes "section about XC02 GetStat/SetStat codes".

    \n \section programs Overview of provided programs

    \subsection xc02_ctrl example for setting various parameters of the driver
    xc02_ctrl.c (see example section)

    \subsection xc02_simp simple example for driver usage
	switches the display off and on	and displays a brightness ramp from minimum to maximum brightness.
	\n
    xc02_simp.c (see example section),

*/

/** \example xc02_simp.c */
/** \example xc02_ctrl.c */

/*!
  \page xc02dummy MEN logo
  \menimages
*/

