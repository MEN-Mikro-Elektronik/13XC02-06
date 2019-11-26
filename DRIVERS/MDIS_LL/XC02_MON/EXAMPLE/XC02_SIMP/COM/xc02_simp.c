/****************************************************************************/
/*!
 *         \file xc02_simp.c
 *       \author thomas.schnuerer@men.de
 *
 *       \brief  Simple example program for the XC02 monitoring driver
 *
 *     Required: libraries: mdis_api
 *     \switches (none)
 */
 /*
 *---------------------------------------------------------------------------
 * Copyright 2005-2019, MEN Mikro Elektronik GmbH
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

#include <stdio.h>
#include <string.h>
#include <MEN/men_typs.h>
#include <MEN/mdis_api.h>
#include <MEN/usr_oss.h>
#include <MEN/xc02_drv.h>

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void PrintError(char *info);



/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/

/* more compact error handling */
#define CHK(expression,errst)	\
    if((expression)<0) {		\
		PrintError(errst);		\
		goto abort;				\
	}



/********************************* PrintMdisError **************************/
/** Print MDIS error message
 *
 *  \param info       \IN  info string
*/
static void PrintError(char *info)
{
	printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}




/********************************* main ************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return	          success (0) or error (1)
 */
int main(int argc, char *argv[])
{
	MDIS_PATH path; /* value, arg_bright, */
	int32 i;
	char  *device;

	if (argc < 2 ) {
		printf("Syntax: xc02_simp <device> \n");
		printf("Function: XC02_MON example program for the XC02 driver\n");
		printf("        example: xc02_simp xc02_1\n");
		printf(" Switch display off and on 4x and ramp brightness from \n");
		printf(" darkest to brightest setting\n");
		return(1);
	}

	device = argv[1];

	/*--------------------+
    |  open path          |
    +--------------------*/
	if ((path = M_open(device)) < 0) {
		PrintError("open");
		return(1);
	}


	/*
	 * 1. switch display off/on 2 times
	 */
	for (i=0; i < 2; i++ ) {
		CHK((M_setstat( path, XC02_SW_DISP, 0x00)), "setstat XC02_SW_DISP");
		UOS_Delay( 2000 );

		CHK((M_setstat( path, XC02_SW_DISP, 0x01)), "setstat XC02_SW_DISP");
		UOS_Delay( 2000 );
	}

	/*
	 * 2. run a brightness ramp from dark (0xc8) to bright (0x0)
	 */
	for (i = 200; i > 0 ; i-=5 ) {
 		CHK((M_setstat( path, XC02_BRIGHTNESS, i)),	"setstat XC02_BRIGHTNESS");
		UOS_Delay( 100 );
	}


	/*--------------------+
    |  cleanup            |
    +--------------------*/
	abort:
	if (M_close(path) < 0)
		PrintError("close");

	return(0);
}

