/****************************************************************************/
/*!
 *         \file xc02_ctrl.c
 *       \author ts
 *        $Date: 2011/08/19 19:48:37 $
 *    $Revision: 1.10 $
 *
 *        \brief Tool to control XC02 PIC
 *
 *     Required: libraries: mdis_api, usr_oss, usr_utl
 *     \switches (none)
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: xc02_ctrl.c,v $
 * Revision 1.10  2011/08/19 19:48:37  ts
 * R: 1. PIC FW power up behavior can be configured (KEY_IN or always on)
 *    2. PIC FW provides raw ADC photo Value from photo sensor
 *    3. auto brightness control behavior can be configure
 * M: 1. added command to access Set/GetStat XC02_KEY_IN_CTRL
 *    2. added command to access GetStat XC02_RAW_BRIGHTNESS
 *    3. added command to access Set/GetStat XC02_AUTO_BRIGHT_CTRL
 *
 * Revision 1.9  2009/09/04 12:13:52  MRoth
 * R: Porting to MDIS5
 * M: added support for 64bit (MDIS_PATH)
 *
 * Revision 1.8  2009/06/20 13:31:05  ts
 * R: auto brightness behaviour made adjustable in firmware
 * M: added options -m,-q and -t to read auto brightness offset,
 *    multiplier and temperature
 *
 * Revision 1.7  2009/03/24 19:48:35  ts
 * R: 1) user could not set an initial brightness for the displays
 *    2) bugfix: display protect status wasnt displayed correct
 * M: 1) new option -a to set initial brightness for both displays
 *    2) corrected bitmask in driver and display status in own line
 *
 * Revision 1.6  2009/02/26 10:56:48  ts
 * R: 1) new Revision XC02-01 with 2nd display has to be supported
 * M: 1) added options -o to select 1st or 2nd display and -m for
 *    minicard power
 *
 * Revision 1.5  2008/12/12 16:27:08  ts
 * R: non ASCII chars in -i info dump cause Windows to display wrong characters
 * M: replaced temperature info to "degree Celsius"
 *
 * Revision 1.4  2008/12/09 11:37:20  ts
 * R: final tests and cosmetics
 *
 * Revision 1.3  2008/12/08 18:58:30  ts
 * R: Info about more internal data was missing
 * M: added GetStat calls to retrieve several settings, like
 *    min/max voltage, brightness and initial display status
 *
 * Revision 1.2  2008/11/25 10:55:44  ts
 * R: 1. UOS_SigInst call causes error on windows build
 *    2: Cosmetics, usage text was not up to date
 *    3: SW shutdown wasnt initiated
 * M: 1. added specifier _MAPILIB
 *    2: corrected usage text, added how to specify numbers
 *    3: changed option s to accept a value (magic 0xa8 is expected)
 *
 * Revision 1.1  2008/09/04 11:47:04  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2009 by MEN mikro elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/xc02_drv.h>
#include <MEN/xc02.h>

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static MDIS_PATH G_Path;
static u_int32   G_SigCountSdEvt;    /* count of shutdown event signals */
static u_int32   G_SigCountOthers;   /* count of other signals */


/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void usage(void);
static void PrintMdisError(char *info);
static void PrintUosError(char *info);
static int ShowIo(void);
static void __MAPILIB SigHandler(u_int32 sigCode);



/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define NONE        -1
#define IO_MAX_NBR  8

#define TEMP_INF    99999   /* if no temperature was passed */

/* more compact error handling */
#define CHK(expression,errst)       \
    if((expression)<0) {            \
        PrintMdisError(errst);      \
        goto abort;                 \
    }


/********************************* usage ***********************************/  
/**  Print program usage
 */
static void usage(void)
{
    printf( "xc02_ctrl [<opts>] <device> [<opts>]    Built: %s %s\n",
            __DATE__, __TIME__ );
    printf(
  "Function: access MEN DCx display controller functions\n"
  "Options:\n"
  " device                  device name, e.g. xc02_1\n"
  " -i                      get current settings and firmware build date\n"
  " -t                      dump only carrier board temperature\n"
  "-------------------------------- power-up settings -----------------------------------------\n"
  " -w=<min>                set wake on time [min] (0=none)\n"
  "-------------------------------- power-off settings ----------------------------------------\n"
  " -d=<mode>               set shutdown delay:\n"
  "                             mode       : 0 | 1 | 2 | 3 |...| 7 \n"
  "                             time [min] : 0 | 1 | 2 | 4 |...| 64\n"
  " -f=<mode>               set off delay:\n"
  "                             mode       : 0 | 1 | 2 | 3 | 4 | 5\n"
  "                             time [min] : 0 | 1 | 2 | 4 | 8 | 16\n"
  " -s=<magic>              do software shutdown (with magic=0xA8, 168 dec.)\n"
  "-------------------------------- binary inputs, temp settings ------------------------------\n"
  " -H=<limit>              set temperature high limit                    [degree celsius]\n"
  " -L=<limit>              set temperature low  limit                    [degree celsius]\n"
  " -I                      continously read bin-IO/temp/volt/brightness\n"
  " -k=<KEY_IN control>     control board startup behavior   -k=0:         start up on KEY_IN=1\n"
  "                                                          -k=1: always start up when powered\n"
  "-------------------------------- Display control -------------------------------------------\n"
  " -o=<0,1>                select primary or secondary screen for brightness commands:\n"
  " -b=<bright>             set brightness in 0,5 percent steps                   [0-200, dec.]\n"
  " -a=<bright>             set initial brightness of display <-o>                [0-200, dec.]\n"
  " -r=<src>                set brightness source                       [0:use <-b>, 1:sensor ]\n"
  " -m=<mult>               set auto brightness multiplier                   [ x0.1,default:10]\n"
  " -q=<offs>               set auto brightness offset                            [0-254, dec.]\n"
  " -p                      SC21 only: power drawn from backlight/panel (12V rail)          [W]\n"
  " -c=<switch>             switch displays off/on:                        -c=0 pri:OFF sec:OFF\n"
  "                         (2nd Display optional)                         -c=1 pri: ON sec:OFF\n"
  "                                                                        -c=2 pri:OFF sec: ON\n"
  "                                                                        -c=3 pri: ON sec: ON\n\n"
  " -z=<init.state>         initial panel state (!!! USE WITH CARE !!!) [values: see option -c]\n"
  " -g=<bright.direction>   auto brightness behavior       [0:XC02/SC21 Std  1:inverse (DC2/6)]\n"
        );
}

static int showInfo(void)
{

    int32   val, chan;
	float mul=0.0;

	printf("current settings:\n");

	CHK((M_getstat(G_Path, XC02_WOT, &val))," XC02_WOT" );
	printf("- wake on time                : %dmin\n", val);

	CHK((M_getstat(G_Path, XC02_WDOG_ERR, &val))," XC02_WDOG_ERR" );
	printf("- missing wdog triggers       : %d\n", val);

	CHK((M_getstat(G_Path, XC02_DOWN_DELAY, &val))," XC02_DOWN_DELAY");
	printf("- shutdown delay              : %d min\n",
		   XC02C_DOWN_DELAY_MIN( val ));

	CHK((M_getstat(G_Path, XC02_OFF_DELAY, &val))," XC02_OFF_DELAY");
	printf("- off delay                   : %dmin\n",
		   XC02C_OFF_DELAY_MIN( val ));

	CHK((M_getstat(G_Path, XC02_DOWN_EVT, &val))," XC02_DOWN_EVT");
	printf("- shutdown event flag         : %d\n", val);

	CHK( (M_getstat(G_Path, XC02_IN, &val)), " XC02_IN");
	printf("- binary inputs               : 0x%x\n", val );

	CHK( (M_getstat(G_Path, XC02_VOLTAGE, &val))," XC02_VOLTAGE");
	printf("- Voltage                     : %d mV (raw 0x%x)\n",
		   XC02_ADC2VOLT(val), val);

	CHK( (M_getstat(G_Path, XC02_BRIGHTNESS, &val))," XC02_BRIGHTNESS");
	printf("- Brightness                  : %d% (raw 0x%x)\n",
		   100 - (val >> 1), val);

	CHK((M_getstat(G_Path, XC02_RAW_BRIGHTNESS, &val))," XC02_RAW_BRIGHTNESS");
	printf("- Brightness Raw ADC          : %d \n", val);

	CHK((M_getstat(G_Path, XC02_BR_SRC, &val )), " XC02_BR_SRC");
	printf("- brightness source           : %s\n", val ? "auto" : "manual");

	CHK( (M_getstat(G_Path, XC02_BR_OFFS, &val))," XC02_BR_OFFS");
	printf("- Brightness offset           : 0x%02x\n", val);

	CHK( (M_getstat(G_Path, XC02_BR_MULT, &val))," XC02_BR_MULT");
	mul=(float)((float)val/10.0);
	printf("- Brightness multiplier       : %1.1f\n", mul/*(float)(val/10)*/);

	CHK((M_getstat(G_Path, XC02_DISP_INITSTAT, &val )), "XC02_DISP_INITSTAT");
	printf("- initial displaystatus       : pri: %s sec: %s\n", (val&0x1) ? "ON" : "OFF",  (val&0x2) ? "ON" : "OFF" );

	chan = 0;
	CHK((M_setstat(G_Path, M_MK_CH_CURRENT, chan)), "M_MK_CH_CURRENT");

	CHK((M_getstat(G_Path, XC02_INIT_BRIGHT, &val )), "XC02_INIT_BRIGHT");
	printf("- initial brightness 1        : %d % (raw 0x%x)\n",
		   100 - (val >> 1), val);
	chan = 1;
	CHK((M_setstat(G_Path, M_MK_CH_CURRENT, chan)), "M_MK_CH_CURRENT");
	CHK((M_getstat(G_Path, XC02_INIT_BRIGHT, &val )), "XC02_INIT_BRIGHT");
	printf("- initial brightness 2(option): %d % (raw 0x%x)\n",
		   100 - (val >> 1), val);

	/* Temp & limits */
	CHK( (M_getstat(G_Path, XC02_TEMP, &val)),   " XC02_TEMP");
	printf("- temperature          	      : %d degree Celsius(raw 0x%x)\n",
		   XC02_ADC2TEMP(val), val);

	CHK((M_getstat(G_Path, XC02_TEMP_HIGH, &val))," TEMP_HIGH");
	printf("- Temp high limit             : %d degree Celsius\n", val);

	CHK((M_getstat(G_Path, XC02_TEMP_LOW, &val))," TEMP_LOW");
	printf("- Temp low limit              : %d degree Celsius\n", val);

	CHK( (M_getstat(G_Path, XC02_VOLTAGE, &val))," XC02_VOLTAGE");
	printf("- Voltage                     : %d mV (raw 0x%x)\n",
		   XC02_ADC2VOLT(val), val);

	CHK((M_getstat(G_Path, XC02_VOLT_HIGH, &val))," VOLT_HIGH");
	printf("- Volt high limit             : %d mV\n",
		   XC02_ADC2VOLT(val));

	CHK((M_getstat(G_Path, XC02_VOLT_LOW, &val))," VOLT_LOW");
	printf("- Volt low limit              : %d mV\n",
		   XC02_ADC2VOLT(val));

    CHK((M_getstat(G_Path, XC02_KEY_IN_CTRL, &val))," XC02_KEY_IN_CTRL");
    printf("- KEY_IN control:             : %d (%s)\n", val, val ? "always on" : "KEY_IN");

	CHK((M_getstat(G_Path, SC21_BL_CURRENT, &val )), " SC21_BL_CURRENT");
	if (val == 0xff) {
		printf("- drawn power (12V rail)      : <unknown>\n", SC21_ADC2POWER(val));
	} else {
		printf("- drawn power (12V rail)      : %d W\n", SC21_ADC2POWER(val));
	}

    CHK((M_getstat(G_Path, XC02_AUTO_BRIGHT_CTRL, &val))," XC02_AUTO_BRIGHT_CTRL");
    printf("- auto brightness direction   : %d (%s)\n", val, val ? "DC2/6" : "default");

	/* PIC firmware built string */
	printf("firmware build string:\n");
	do {
		CHK(( M_getstat( G_Path, XC02_TIMESTAMP, &val))," XC02_TIMESTAMP" );
		if (val == 0xff)
			break;
		printf("%c", val);
		UOS_Delay(8);
	} while (val !=0xff);
	printf("\n");

	return(0);

abort:
    return(1);

}

/********************************* main ************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return           success (0) or error (1)
 */
int main( int argc, char *argv[])
{
    char    *device,*str,*errstr,buf[255];
    int32   info, ioInfo;
    int32   wot, val;
    int32   sdDelay, offDelay, swSd, sdEvtPollTime,	arg_broffs, arg_brmult;
    int32   arg_tempHi, arg_tempLo, tempHigh, arg_gettemp, arg_power;
    int32   arg_source, arg_swDisp, arg_screen, arg_keyctrl;
    int32   arg_brightdir, arg_initstate;
    int32   n, arg_bright, arg_inibright;
	float   mul;

    /*-------------------+
	  | check arguments  |
	  +-----------------*/
    if( (errstr = UTL_ILLIOPT("ptiIw=n=a=d=f=s=e=o=q=m=H=L=b=a=c=r=k=z=g=?", buf)) )
    {   /* check args */
        printf("*** %s\n", errstr);
        return(1);
    }

    if( UTL_TSTOPT("?") ){                      /* help requested ? */
        usage();
        return(1);
    }

    /*--------------------+
	  |  get arguments    |
	  +------------------*/
    for (device=NULL, n=1; n<argc; n++)
        if( *argv[n] != '-' ){
            device = argv[n];
            break;
        }

    if( !device || argc<3 ) {
        usage();
        return(1);
    }

    info          = (UTL_TSTOPT("i") ? 1 : NONE);
    ioInfo        = (UTL_TSTOPT("I") ? 1 : NONE);
	arg_gettemp   = (UTL_TSTOPT("t") ? 1 : 0);
	arg_power	  = (UTL_TSTOPT("p") ? 1 : NONE);
    wot           = ((str = UTL_TSTOPT("w=")) ? atoi(str) : NONE);
    sdDelay       = ((str = UTL_TSTOPT("d=")) ? atoi(str) : NONE);
    offDelay      = ((str = UTL_TSTOPT("f=")) ? atoi(str) : NONE);
    swSd          = ((str = UTL_TSTOPT("s=")) ? atoi(str) : NONE);
    sdEvtPollTime = ((str = UTL_TSTOPT("e=")) ? atoi(str) : NONE);
    arg_tempHi    = ((str = UTL_TSTOPT("H=")) ? atoi(str) : TEMP_INF);
    arg_tempLo    = ((str = UTL_TSTOPT("L=")) ? atoi(str) : TEMP_INF);
    arg_bright    = ((str = UTL_TSTOPT("b=")) ? atoi(str) : NONE);
    arg_source    = ((str = UTL_TSTOPT("r=")) ? atoi(str) : NONE);
    arg_swDisp    = ((str = UTL_TSTOPT("c=")) ? atoi(str) : NONE);
	arg_screen	  = ((str = UTL_TSTOPT("o=")) ? atoi(str) : 0);
    arg_inibright = ((str = UTL_TSTOPT("a=")) ? atoi(str) : NONE);
	arg_broffs    = ((str = UTL_TSTOPT("q=")) ? atoi(str) : -1);
	arg_brmult    = ((str = UTL_TSTOPT("m=")) ? atoi(str) : -1);

	arg_keyctrl   = ((str = UTL_TSTOPT("k=")) ? atoi(str) : -1);
	arg_brightdir = ((str = UTL_TSTOPT("g=")) ? atoi(str) : -1);
	arg_initstate = ((str = UTL_TSTOPT("z=")) ? atoi(str) : -1);
	
    tempHigh      = ((str = UTL_TSTOPT("T=")) ? atoi(str) : NONE);

    /* sanity check of passed args */
    if ( (arg_tempLo < TEMP_INF) && (arg_tempHi < TEMP_INF)) {
        CHK( (arg_tempLo < arg_tempHi), "low temp above high temp");
    }

    /*--------------------+
	  |  open path          |
	  +--------------------*/

    CHK( (G_Path = M_open(device)),"open");

    /* current settings summary */
    if( info != NONE )
		showInfo();

    /* set wake on time ? */
    if( wot != NONE ) {
        printf("- set wake on time to: %dmin\n", wot);
        CHK((M_setstat(G_Path, XC02_WOT, wot))," XC02_WOT");
    }

    /* set brightness ? */
    if ( arg_bright >=0) {
		if (arg_screen == 0) {
			printf("- set brightness of Screen 0 to %d\n", arg_bright );
			CHK((M_setstat(G_Path,XC02_BRIGHTNESS, arg_bright)),
				"setstat XC02_BRIGHTNESS");
		}
		if (arg_screen == 1){
			printf("- set brightness of Screen 1 to %d\n", arg_bright );
			CHK((M_setstat(G_Path,XC02_BRIGHTNESS2, arg_bright)),
				"setstat XC02_BRIGHTNESS2");
		}
    }

    /* set initial brightness ? */
    if ( arg_inibright >=0 ) {
		/* select the screen */
		printf("- set initial brightness of Screen %d to %d\n",
			   arg_screen, arg_inibright );

		CHK((M_setstat(G_Path, M_MK_CH_CURRENT, arg_screen)),
			"setstat M_MK_CH_CURRENT");

		CHK((M_setstat(G_Path, XC02_INIT_BRIGHT, arg_inibright)),
			"setstat XC02_INIT_BRIGHT");
    }

    /* set shutdown delay ? */
    if( sdDelay != NONE ){
        printf("- set shutdown delay to: mode=%d (%dmin)\n",
			   sdDelay, XC02C_DOWN_DELAY_MIN(sdDelay));
        CHK((M_setstat(G_Path, XC02_DOWN_DELAY, sdDelay)),
            "setstat XC02_DOWN_DELAY");
    }

    /* set off delay ? */
    if( offDelay != NONE ){
        printf("- set off delay to: mode=%d (%dmin)\n",
			   offDelay, XC02C_OFF_DELAY_MIN(offDelay));
        CHK((M_setstat(G_Path, XC02_OFF_DELAY, offDelay)),"XC02_OFF_DELAY");
    }

    /* initiate a software shutdown ? */
    if( swSd != NONE ){
        printf("- initiate a software shutdown\n");
        CHK((M_setstat(G_Path, XC02_SWOFF, swSd)),"XC02_SWOFF");
    }

	/* get temperature ? */
	if (arg_gettemp) { /* temp */
		CHK( (M_getstat(G_Path, XC02_TEMP, &val)), " XC02_TEMP");
		printf("XC2 Temp: %d degree celsius (raw 0x%x)\n",
			   XC02_ADC2TEMP(val), val);
	}

    /* set new temperature high limit */
    if( arg_tempHi != TEMP_INF ){
        printf("- set temperature high limit: %d\n", arg_tempHi);
        CHK((M_setstat(G_Path, XC02_TEMP_HIGH, arg_tempHi ))," XC02_TEMP_HIGH");
    }

    /* set new temperature low limit */
    if( arg_tempLo != TEMP_INF ){
        printf("- set temperature low limit: %d\n", arg_tempLo );
        CHK((M_setstat(G_Path, XC02_TEMP_LOW, arg_tempLo )), "XC02_TEMP_LOW");
    }

    /* select the brightness control source */
    if (arg_source>=0) {
        printf("- set brightness source to %s\n",
               arg_source ? "auto" : "SMB cmd" );
        CHK((M_setstat(G_Path, XC02_BR_SRC, arg_source )), "XC02_BR_SRC");
    }

    /* switch display(s) with bit0 and bit1 */
    if ( (arg_swDisp >= 0) && (arg_swDisp <= 3 ) ) {
		printf("- switch display1 %s, display2 %s (display2 only DC1R01)\n",
			   (arg_swDisp & 0x1) ? "ON" : "OFF" ,
			   (arg_swDisp & 0x2) ? "ON" : "OFF" );
		CHK((M_setstat(G_Path, XC02_SW_DISP, arg_swDisp )),"XC02_SW_DISP");
	}

	arg_broffs    = ((str = UTL_TSTOPT("q=")) ? atoi(str) : -1);
	arg_brmult    = ((str = UTL_TSTOPT("m=")) ? atoi(str) : -1);

	/* brightness offset passed */
	if ( arg_broffs >= 0 ) {
		printf("- set auto brightness offset = %d\n", arg_broffs );
		CHK((M_setstat(G_Path, XC02_BR_OFFS, arg_broffs)), "XC02_BR_OFFS");
	}

	/* multiplier passed */
	if ( arg_brmult >= 0 ) {
		mul=(float)((float)arg_brmult/10.0);
		printf("- set auto brightness multiplier = %1.1f\n", mul );
		CHK((M_setstat( G_Path, XC02_BR_MULT, arg_brmult)), "XC02_BR_MULT");
	}

	/* get drawn SC21 12V power */
	if (arg_power == 1) {
		CHK( (M_getstat(G_Path, SC21_BL_CURRENT, &val )), "SC21_BL_CURRENT");
		if (val == 0xff) {
			printf(" *** SC21_BL_CURRENT returned 0xff (board no SC21?)\n");
		} else {
			printf(" drawn power (12V rail) = %d W\n", SC21_ADC2POWER(val));
		}
	}

	/* KEY_IN behavior */
	if ( (arg_keyctrl == 0) || (arg_keyctrl == 1) ) {
		printf("- set KEY_IN behavior = %d\n", arg_keyctrl );
		CHK((M_setstat(G_Path, XC02_KEY_IN_CTRL, arg_keyctrl)), "XC02_KEY_IN_CTRL");
	}

	/* initial display state */
	if ( (arg_initstate >=0) && (arg_initstate <= 3 )) {
		printf("- set initial display state = %d ( pri: %s sec: %s)\n", 
			   arg_initstate,
			   ( arg_initstate & 0x1 ) ? "ON" : "OFF",
			   ( arg_initstate & 0x2 ) ? "ON" : "OFF" );
		CHK((M_setstat(G_Path, XC02_DISP_INITSTAT, arg_initstate)), "XC02_DISP_INITSTAT");
	}

	/* auto brightness direction control */
	if ( (arg_brightdir == 0) || (arg_brightdir == 1 )) {
		printf("- set auto brightness direction = %d\n", arg_brightdir );
		CHK((M_setstat(G_Path, XC02_AUTO_BRIGHT_CTRL, arg_brightdir)), "XC02_AUTO_BRIGHT_CTRL");
	}

	
    /*-------------------------------+
	 | handle shutdown event signal  |
	 +-------------------------------*/
    if( sdEvtPollTime != NONE ){

        M_SG_BLOCK          blk;
        XC02_BLK_DOWN_SIG   downSig;

        printf("- handle shutdown event signal:\n");
        printf("  The driver polls all %dms for an shutdown event\n",
               sdEvtPollTime);
        printf("  and sends an signal to the App if the event-flag is set.\n");
        printf("  The App sends OFF Acknowledge if the signal was received\n");

        /* clear signal counters */
        G_SigCountSdEvt = 0;
        G_SigCountOthers = 0;

        blk.size        = sizeof(XC02_BLK_DOWN_SIG);
        blk.data        = (void*)&downSig;

        downSig.msec    = sdEvtPollTime;
        downSig.signal  = UOS_SIG_USR1;

        /* install signal handler */
        if( UOS_SigInit(SigHandler) ){
            PrintUosError("SigInit");
            return(1);
        }

        /* install signal */
        if( UOS_SigInstall(UOS_SIG_USR1) ){
            PrintUosError("SigInstall");
            goto abort;
        }

        /* install signal for shutdown event */
        CHK((M_setstat(G_Path, XC02_BLK_DOWN_SIG_SET,(INT32_OR_64)&blk)),
            "XC02_BLK_DOWN_SIG_SET");
        printf("--- press any key to abort ---\n");
    }

    /*--------------------------+
	 | input/temp/volt states   |
	 |      AND / OR            |
	 | wait event signal        |
	 +--------------------------*/
    if( ( ioInfo != NONE ) || (sdEvtPollTime != NONE) ){
        printf("--- To stop poll, press any key to abort ---\n");
        /* repeat until keypress */
        do {
            if( ioInfo != NONE ){
                if( ShowIo() )
                    goto abort;
            }
            if( sdEvtPollTime != NONE ){
                if( G_SigCountSdEvt ){
                    printf("--- OFF Acknowledge sent ---\n");
                    printf("--- system poweroff after off delay time ---\n");
                    goto abort;
                }
            }
            UOS_Delay(1000);
        } while( (UOS_KeyPressed() == -1) );
    }

    /*--------------------+
	  |  cleanup            |
	  +--------------------*/
abort:

    if( sdEvtPollTime != NONE ){
        /* terminate signal handling */
        M_setstat(G_Path, XC02_DOWN_SIG_CLR, 0);
        UOS_SigExit();
        printf("\n");
        printf("Count of shutdown event signals : %d \n", G_SigCountSdEvt);
        printf("Count of other signals          : %d \n", G_SigCountOthers);
    }

    if( M_close(G_Path) < 0 )
        PrintMdisError("close");

    return(0);
}

/********************************* PrintMdisError **************************/
/** Print MDIS error message
 *
 *  \param info       \IN  info string
*/
static void PrintMdisError(char *info)
{
    printf("*** can't %s: %s\n", info, M_errstring(UOS_ErrnoGet()));
}


/********************************* PrintUosError ***************************/
/** Print User OSS error message
 *
 *  \param info       \IN  info string
*/
static void PrintUosError(char *info)
{
    printf("*** can't %s: %s\n", info, UOS_ErrString(UOS_ErrnoGet()));
}


/********************************* ShowIo **********************************/
/** show i/o, temp and volt
 *
 *  \return           \c 0 success or error code
*/
static int ShowIo( void )
{
    int32 val;

    printf(" -------- binary i/o, temp and volt states: -------\n");

    /* binary inputs state , val contains raw value */
    CHK( (M_getstat(G_Path, XC02_IN, &val))," XC02_IN");
    printf("- binary inputs: KEY_IN = %d  GA[3:0] = 0x%x\n",
           val & 0x1, (val>>1) & 0xf);
    /* temp */
    CHK( (M_getstat(G_Path, XC02_TEMP, &val)),   " XC02_TEMP");
    printf("- temperature :                        %d degree celsius (raw 0x%x)\n",
           XC02_ADC2TEMP(val), val);

    /* voltage */
    CHK( (M_getstat(G_Path, XC02_VOLTAGE, &val))," XC02_VOLTAGE");
    printf("- Voltage:                             %d mV  (raw 0x%x)\n",
           XC02_ADC2VOLT(val), val);

    CHK((M_getstat(G_Path, XC02_DOWN_EVT, &val)), "XC02_DOWN_EVT");
    printf("- shutdown event flag:                 %d\n", val);

    CHK((M_getstat(G_Path, XC02_SW_DISP,  &val )), "XC02_SW_DISP");
    printf("- display switch status:               %d\n", val);

    /* brightness */
    CHK( (M_getstat(G_Path, XC02_BRIGHTNESS, &val))," XC02_BRIGHTNESS");
    printf("- Brightness screen1: %d % (raw 0x%02x) ", 100-(val>>1), val);
    /* brightness2 */
    CHK( (M_getstat(G_Path, XC02_BRIGHTNESS2, &val)), " XC02_BRIGHTNESS2");
    printf(                                      "screen2: %d % (raw 0x%x)\n", 100-(val>>1), val);

    CHK((M_getstat(G_Path, XC02_RAW_BRIGHTNESS, &val))," XC02_RAW_BRIGHTNESS");
    printf("- Brightness Raw ADC value:            0x%02x \n", val);

    CHK((M_getstat(G_Path, XC02_KEY_IN_CTRL, &val))," XC02_KEY_IN_CTRL");
    printf("- KEY_IN control:                      0x%02x (%s)\n", val, val ? "always on" : "KEY_IN");


	CHK((M_getstat(G_Path, SC21_BL_CURRENT, &val )), " SC21_BL_CURRENT");
	if (val == 0xff) {
		/* 0xff returned, no register implemented (XC02) */
		printf("- drawn power (12V rail):              <unknown>\n"); 
	} else {
		printf("- drawn power (12V rail):              %d W\n", SC21_ADC2POWER(val));
	}

    /* Temp Limits */
    CHK((M_getstat(G_Path, XC02_TEMP_HIGH, &val))," TEMP_HIGH");
    printf("- Temp limits: high %d ,", val);
    CHK((M_getstat(G_Path, XC02_TEMP_LOW, &val))," TEMP_LOW");
    printf(                         " low          %d degree C  \n", val);

    return(0);

abort:
    return(1);
}

/********************************* SigHandler *******************************/
/** Signal handler
 *
 *  \param sigCode       \IN signal code received
*/
static void __MAPILIB SigHandler(u_int32 sigCode)
{
    if(  sigCode == UOS_SIG_USR1){
        /* send OFF Acknowledge */
        M_setstat(G_Path, XC02_OFFACK, 0);
        G_SigCountSdEvt++;
    }
    else
        G_SigCountOthers++;
}


