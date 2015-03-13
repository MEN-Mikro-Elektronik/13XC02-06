/*****************************************************************************/
/*!
 *         \file xc02_test.c
 *       \author thomas.schnuerer@men.de
 *        $Date: 2011/08/22 18:14:33 $
 *    $Revision: 1.4 $
 *
 *        \brief regression Test tool for 13xc02-06 driver Get/Setstats
 *
 *     Required: libraries: mdis_api, usr_oss, usr_utl
 *     \switches (none)
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: xc02_test.c,v $
 * Revision 1.4  2011/08/22 18:14:33  ts
 * R: MDVE warning about difference (signed/unsigned) in parameter wdogErr
 * M: changed parameter to int32
 *
 * Revision 1.3  2009/09/04 12:13:56  MRoth
 * R: Porting to MDIS5
 * M: a) added support for 64bit (MDIS_PATH)
 *    b) added casts to avoid compiler warnings
 *
 * Revision 1.2  2008/12/08 18:59:53  ts
 * R: regression test for every Get/SetStat was missing
 * M: added test -n which writes/reads every Get/SetStat in the driver
 *
 * Revision 1.1  2008/09/04 11:48:13  ts
 * Initial Revision
 *
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2006 by MEN mikro elektronik GmbH, Nuernberg, Germany
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <MEN/men_typs.h>
#include <MEN/usr_oss.h>
#include <MEN/usr_utl.h>
#include <MEN/mdis_api.h>
#include <MEN/xc02_drv.h>
#include <MEN/xc02.h>
#include <MEN/wdog.h>

/*--------------------------------------+
|   GLOBALS                             |
+--------------------------------------*/
static MDIS_PATH G_Path;
static u_int32	 G_TimeStart;

/*--------------------------------------+
|   DEFINES                             |
+--------------------------------------*/
#define NONE		-1
#define	PWR_ON		1
#define	PWR_OFF		0

/* more compact error handling */
#define CHK(expression,errst) \
    if((expression)<0) { \
        PrintMdisError(errst); \
        goto abort; \
    }

#define MSEC_2_MIN_SEC( msec, min, sec )	\
	min = (msec) / (60*1000);	\
	sec = (double)((msec) % (60*1000))/1000.0;

#define TIMER_GET_MIN_SEC( min, sec )				\
	MSEC_2_MIN_SEC( UOS_MsecTimerGet() - G_TimeStart, min, sec )

#define TIMER_GET_MSEC \
	(UOS_MsecTimerGet() - G_TimeStart )

#define TIMER_GET_SEC \
	((UOS_MsecTimerGet() - G_TimeStart) / 1000 )

#define FWARE_DELAY		10				/* ms */
#define POLL_TIME		100				/* ms */
#define DELAYTIME		500				/* ms */
#define USERTIME		5000			/* ms */

#define _TOUT_OFFSET(t)	(((t)*3)/100)	/* 3% */
#define TOUT_OFFSET(t)	(_TOUT_OFFSET(t) < DELAYTIME ? \
						DELAYTIME : _TOUT_OFFSET(t))	/* 3% or DELAYTIME */
#define TOUT_MIN(t)		((t) - TOUT_OFFSET(t))
#define TOUT_MAX(t)		((t) + TOUT_OFFSET(t))

/*--------------------------------------+
|   PROTOTYPES                          |
+--------------------------------------*/
static void usage(void);
static void PrintMdisError(char *info);
static int TestSwOffOn( u_int32 downDelay, u_int32 offDelay, u_int32 wot );
static int Wait4DownEvent( u_int32 timeout );
static int Wait4PwrOnOff( u_int32 state, u_int32 timeout );
static int WotDuringDowndelay( u_int32 timeout );
static int TestKeyOffOn( u_int32 downDelay, u_int32 offDelay );
static int TestOffAck( void );
static int TestRegularGetSetStats(void);
static int TestWdog( u_int32 wdog );
static int CheckRstErroffErrcount( u_int32 *rst,
								   u_int32 *errOff,
								   u_int32 *ackErr,
								   int32 *wdogErr );

static int WdogOnOff( u_int32 enable );

/********************************* usage ***********************************/
/**  Print program usage
 */
static void usage(void)
{
	printf("Usage: xc02_test [<opts>] <device> [<opts>]\n");
	printf("Function: XC02 PIC Test, derived from 08AD78 uC test\n");
	printf("Options:\n");
	printf("    device       device name                                \n");
	printf("    -n           test write/read access to each GetSetstat  \n");
	printf("                                                            \n");
	printf("    -s=<level>   test SWOFF, DOWN_DELAY, OFF_DELAY, WOT     \n");
	printf("                   level=1/2/3 : short/medium/complete test \n");
	printf("    -k           test key off/on                            \n");
	printf("                   Note: requires user interaction          \n");
	printf("    -a=<level>   test off acknowledge                       \n");
	printf("                   level=1/2 : short/complete test          \n");
	printf("    -w=<level>   test watchdog                              \n");
	printf("                   level=1/2 : short/complete test          \n");
	printf("    -f=<time>    show firmware internal state               \n");
	printf("                   poll all <time> in ms in a loop          \n");
	printf("\n");

}

/********************************* main ************************************/
/** Program main function
 *
 *  \param minutes    \IN  minutes to convert, must be power of 2
 *
 *  \return	  mode number, minutes=1<<(mode-1) as in log(x-1)/log(2)
 */
static int Minutes2Mode( int minutes)
{
	int mode=0xff;

	/* poor mans log(x)/log(2).. */
	switch( minutes ){
	case 0:
		mode=0;
		break;
	case 1:
		mode=1;
		break;
	case 2:
		mode=2;
		break;
	case 4:
		mode=3;
		break;
	case 8:
		mode=4;
		break;
	case 16:
		mode=5;
		break;
	case 32:
		mode=6;
		break;
	case 64:
		mode=7;
		break;
	default:
		printf("*** Warning: invalid delay time (no power of 2)\n");
	}

	return mode;
}

/********************************* main ************************************/
/** Program main function
 *
 *  \param argc       \IN  argument counter
 *  \param argv       \IN  argument vector
 *
 *  \return	          success (0) or error (1)
 */
int main( int argc, char *argv[])
{
	char	*device,*str,*errstr,buf[40];
	int32	n, val, ret=1;
	int32	swOff, keyOff, onOffAck, fState, wdog, 	getSetStat;

	/*--------------------+
    |  check arguments    |
    +--------------------*/
	if( (errstr = UTL_ILLIOPT("s=kna=f=w=?", buf)) ){	/* check args */
		printf("*** %s\n", errstr);
		return(1);
	}

	if( UTL_TSTOPT("?") ){						/* help requested ? */
		usage();
		return(1);
	}

	/*--------------------+
    |  get arguments      |
    +--------------------*/
	for (device=NULL, n=1; n<argc; n++)
		if( *argv[n] != '-' ){
			device = argv[n];
			break;
		}

	if( !device || argc<3 ) {
		usage();
		return(1);
	}

	getSetStat  = (UTL_TSTOPT("n") ? 1 : NONE);
	swOff  		= ((str = UTL_TSTOPT("s=")) ? atoi(str) : NONE);
	keyOff 		= (UTL_TSTOPT("k") ? 1 : NONE);
	onOffAck  	= ((str = UTL_TSTOPT("a=")) ? atoi(str) : NONE);
	wdog 		= ((str = UTL_TSTOPT("w=")) ? atoi(str) : NONE);
	fState  	= ((str = UTL_TSTOPT("f=")) ? atoi(str) : NONE);

	/*--------------------+
    |  open path          |
    +--------------------*/
	if( (G_Path = M_open(device)) < 0) {
		PrintMdisError("open");
		return(1);
	}

	/*--------------------+
    |  enable test mode   |
    +--------------------*/
	printf("=== enable test mode ===\n");
	if( (M_setstat(G_Path, XC02_TEST1, XC02C_TEST1_TMODE1)) < 0) {
			PrintMdisError("setstat XC02_TEST1");
		goto abort;
	}

	if (getSetStat) {
		ret = TestRegularGetSetStats();
		printf("\n============= %s ==============\n",(ret<0) ? "FAILED" : "OK");
		if (ret<0)
			goto abort;
	}

	if( swOff != NONE ){
		/*------------------------------------+
		|  SWOFF, DOWN_DELAY, OFF_DELAY, WOT  |
		+------------------------------------*/

		struct _DELAY_TABLE
		{
			int32 downDelay		/* 0..64min */;
			int32 offDelay;		/* 0..16min */
			int32 wot;			/* 0..65535min (should !=downDelay for test) */
			int32 testLevel;	/* 1,2,3 */
		} delay[] =
		{ /* down   off      wot      level */
			/* short test */
			{ 0,	 0,		  1,		1},
			{ 0,	 1,		  1,		1},
			{ 2,	 0,		  1,		1},	/* wot during downDelay */
			{ 1,	 1,		  2,		1},
			/* medium test */
			{ 1,	 2,		  2,		2},
			{ 2,	 1,		  3,		2},
			{ 2,	 2,		  3,		2},
			/* complete test */
			{ 0,	 4,		 60,		3},
			{ 0,	 8,		255,		3},
			{ 0,	16,		720,		3},
			{ 4,	 0,		  3,		3}, /* wot during downDelay */
			{ 8,	 0,		  9,		3},
			{16,	 0,		 17,		3},
			{32,	 0,		 33,		3},
			{64,	 0,		 65,		3},
		};

		printf("=== test SWOFF, DOWN_DELAY, OFF_DELAY, WOT ===\n");

		for( n=0; n<sizeof(delay)/sizeof(struct _DELAY_TABLE); n++ ){

			if( delay[n].testLevel > swOff )
				continue;

			if((TestSwOffOn(delay[n].downDelay,
							delay[n].offDelay,
							delay[n].wot)))
				goto abort;
		}
	}

	if( keyOff != NONE ){
		/*-------------------------------+
		|  KEY OFF/ON                    |
		+-------------------------------*/
		printf("=== test key off/on ===\n");

		if( (TestKeyOffOn( 0, 1 )) )
			goto abort;
	}

	if( onOffAck != NONE ){

		/*-------------------------------+
		|  OFF ACKNOWLEDGE               |
		+-------------------------------*/
		if( (TestOffAck()) )
			goto abort;

		/*-------------------------------+
		|  ON ACKNOWLEDGE                |
		+-------------------------------*/
	}

	if( wdog != NONE ){

		/*-------------------------------+
		|  WATCHDOG                      |
		+-------------------------------*/
		if( (TestWdog( wdog )) )
			goto abort;
	}

	if( fState != NONE ){
		/*-------------------------------+
		|  Firmware State                |
		+-------------------------------*/
		char *str;
		printf("=== poll fw state all %dms (until keypress) ===\n", fState);
		/* repeat until keypress */
		do {

			if ((M_getstat(G_Path, XC02_TEST4, &val)) < 0) {
				PrintMdisError("getstat XC02_TEST4");
				goto abort;
			}

			switch( val ){
			case SM_STATE_OFF:
				str = "SM_STATE_OFF";
				break;
			case SM_STATE_WAIT_SOA:
				str = "SM_STATE_WAIT_SMB_ON_ACK";
				break;
			case SM_STATE_ON:
				str = "SM_STATE_ON";
				break;
			case SM_STATE_WAIT_SD:
				str = "SM_STATE_WAIT_SHUTDOWN_DELAY";
				break;
			case SM_STATE_WAIT_OD:
				str = "SM_STATE_WAIT_OFF_DELAY";
				break;
			case SM_STATE_WAIT_OFF:
				str = "SM_STATE_WAIT_OFF";
				break;
			case SM_STATE_ERROR_OFF:
				str = "SM_STATE_ERROR_OFF";
				break;
			default:
				str = "UNKNOWN";
			}

			printf("- firmware state: %s (%d)\r", str, val);
			UOS_Delay(fState);

		} while( (UOS_KeyPressed() == -1) );

		printf("\n");
	}

	ret = 0;

	/*--------------------+
    |  cleanup            |
    +--------------------*/
	abort:

	if( ret )
		printf("*** SOME TESTS FAILED ***\n");
	else
		printf("=== ALL TESTS SUCCEEDED ===\n");

	/*--------------------+
    |  disable test mode  |
    +--------------------*/
	printf("=> press any key to disable the test mode\n");
	UOS_KeyWait();

	printf("=== disable test mode ===\n");
	if( (M_setstat(G_Path, XC02_TEST1, XC02C_TEST1_NOTEST)) < 0) {
			PrintMdisError("setstat XC02_TEST1");
		goto abort;
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

/********************************* TestSwOffOn *****************************/
/** Test SWOFF, DOWN_DELAY, OFF_DELAY, WOT
 *
 *  \param downDelay	\IN  down delay [min]
 *  \param offDelay		\IN  off delay [min]
 *  \param wot			\IN  wot [min]
 *
 *  \return				0 success or error code
*/
static int TestSwOffOn( u_int32 downDelay, u_int32 offDelay, u_int32 wot )
{
	u_int8	mode;
	u_int32	min;
	double	sec;

	printf("\n- set downDelay=%dmin, offDelay=%dmin, wot=%dmin\n",
		downDelay, offDelay, wot);

	/* set shutdown delay */
	mode = (u_int8)(Minutes2Mode( downDelay ));
	if( (M_setstat(G_Path, XC02_DOWN_DELAY, mode)) < 0) {
		PrintMdisError("setstat XC02_DOWN_DELAY");
		return 1;
	}

	/* set off delay */
	mode = (u_int8)(Minutes2Mode( offDelay ));
	if( (M_setstat(G_Path, XC02_OFF_DELAY, mode)) < 0) {
		PrintMdisError("setstat XC02_OFF_DELAY");
		return 1;
	}

	/* set wake on time */
	if( (M_setstat(G_Path, XC02_WOT, wot)) < 0) {
		PrintMdisError("setstat XC02_WOT");
		return 1;
	}

	/* initiate a software shutdown */
	printf("- send SWOFF\n");
	if( (M_setstat(G_Path, XC02_SWOFF, 0)) < 0) {
		PrintMdisError("setstat XC02_SWOFF");
		return 1;
	}

	G_TimeStart = UOS_MsecTimerGet();

	/* wot during downDelay? */
	if( wot < downDelay ){
		if( (WotDuringDowndelay( TOUT_MAX(downDelay*60*1000) )) )
			return 1;

		TIMER_GET_MIN_SEC( min, sec );

		printf("- WOT %dmin%.3fs after SWOFF\n", min, sec);
	}
	/* wot after downDelay */
	else{
		if( (Wait4DownEvent( TOUT_MAX(downDelay*60*1000) )) )
			return 1;

		TIMER_GET_MIN_SEC( min, sec );
		printf("- DOWN_EVENT %dmin%.3fs after SWOFF\n", min, sec);

		G_TimeStart = UOS_MsecTimerGet();
		if( (Wait4PwrOnOff( PWR_OFF, TOUT_MAX(offDelay*60*1000) )) )
			return 1;

		TIMER_GET_MIN_SEC( min, sec );
		printf("- PWR_OFF %dmin%.3fs after DOWN_EVENT\n", min, sec);

		G_TimeStart = UOS_MsecTimerGet();
		if( (Wait4PwrOnOff( PWR_ON, TOUT_MAX(
				XC02C_WAIT_OFF + (wot-(downDelay+offDelay))*60*1000) )) )
			return 1;

		TIMER_GET_MIN_SEC( min, sec );
		printf("- WOT/PWR_ON %dmin%.3fs after PWR_OFF\n", min, sec);
	}


	return 0;
}

/********************************* TestKeyOffOn ****************************/
/** Test key off/on
 *
 *  \param downDelay	\IN  down delay [min]
 *  \param offDelay		\IN  off delay [min]
 *
 *  \return				0 success or error code
*/
static int TestKeyOffOn( u_int32 downDelay, u_int32 offDelay )
{
	u_int8	mode;
	u_int32	min;
	double	sec;

	printf("- downDelay=%dmin, offDelay=%dmin\n",
		downDelay, offDelay);

	/* set shutdown delay */
	mode = (u_int8)(Minutes2Mode( downDelay ));
	if( (M_setstat(G_Path, XC02_DOWN_DELAY, mode)) < 0) {
		PrintMdisError("setstat XC02_DOWN_DELAY");
		return 1;
	}

	/* set off delay */
	mode = (u_int8)(Minutes2Mode( offDelay ));
	if( (M_setstat(G_Path, XC02_OFF_DELAY, mode)) < 0) {
		PrintMdisError("setstat XC02_OFF_DELAY");
		return 1;
	}

	printf("=> Please, switch the key off within %ds\n", USERTIME/1000);

	G_TimeStart = UOS_MsecTimerGet();
	if( (Wait4DownEvent( downDelay*60*1000 + USERTIME )) )
		goto ERROR_CLEANUP;

	TIMER_GET_MIN_SEC( min, sec );
	printf("- DOWN_EVENT after %dmin%.3fs\n", min, sec);

	G_TimeStart = UOS_MsecTimerGet();
	if( (Wait4PwrOnOff( PWR_OFF, TOUT_MAX(offDelay*60*1000) )) )
		goto ERROR_CLEANUP;

	TIMER_GET_MIN_SEC( min, sec );
	printf("- PWR_OFF %dmin%.3fs after DOWN_EVENT\n", min, sec);

	printf("=> Please, switch the key on within %ds\n", USERTIME/1000);

	G_TimeStart = UOS_MsecTimerGet();
	if( (Wait4PwrOnOff( PWR_ON, offDelay*60*1000 + USERTIME )) )
		goto ERROR_CLEANUP;

	TIMER_GET_MIN_SEC( min, sec );
	printf("- PWR_ON %dmin%.3fs after DOWN_EVENT\n", min, sec);

	return 0;

ERROR_CLEANUP:
	printf("=> *** TEST ABORTED: please switch the key on\n");
	return 1;
}


/********************************* TestKeyOffOn ****************************/
/** Test read and write access to every Get- and SetStat code
 *
 *  \return			0 success or error code
*/
static int TestRegularGetSetStats(void)
{
	int32 val = 0;
	int32 i;

	printf("Checking Set/GetStats. Write and read back all Set/GetStats\n");

	printf(" ---------- Checking XC02_WOT ----------\n");
	/* write and read back an 8 and 16bit value */
	CHK( M_getstat(G_Path, XC02_WOT, &val )," XC02_WOT" );
	printf(" get WOT: WOT = %d min:\n", val);
	val = 100;
	printf(" set WOT = %d\n", val);
	CHK( M_setstat(G_Path, XC02_WOT, val )," XC02_WOT" );
	printf(" get WOT. ");
	CHK( M_getstat(G_Path, XC02_WOT, &val )," XC02_WOT" );
	printf("WOT = %d min:\n", val);
	val = 65000;
	printf(" set WOT = %d\n", val);
	CHK( M_setstat(G_Path, XC02_WOT, val )," XC02_WOT" );
	printf(" get WOT. ");
	CHK( M_getstat(G_Path, XC02_WOT, &val )," XC02_WOT" );
	printf(" WOT = %d min:\n", val);

	UOS_Delay(500);
	printf(" ---------- Checking XC02_DOWN_DELAY ----------\n");
	for (i=0; i <=7; i++) { 	/* write and read back 0..7 */
		CHK( M_getstat(G_Path, XC02_DOWN_DELAY, &val )," XC02_DOWN_DELAY" );
		printf(" get down delay: down delay = %d\n", val);
		printf(" set down delay = %d\n", i);
		CHK( M_setstat(G_Path, XC02_DOWN_DELAY, i )," XC02_DOWN_DELAY" );
		printf(" get down delay. ");
		CHK( M_getstat(G_Path, XC02_DOWN_DELAY, &val )," XC02_DOWN_DELAY" );
		printf("down delay = %d min:\n", val);
	}
	/* reset to default */
	CHK( M_setstat(G_Path, XC02_DOWN_DELAY, 0)," XC02_DOWN_DELAY" );

	UOS_Delay(500);
	printf(" ---------- Checking XC02_OFF_DELAY ----------\n");
	for (i=0; i <=5; i++) { 	/* write and read back 0..7 */
		CHK( M_getstat(G_Path, XC02_OFF_DELAY, &val )," XC02_OFF_DELAY" );
		printf(" get down delay: down delay = %d\n", val);
		printf(" set down delay = %d\n", i);
		CHK( M_setstat(G_Path, XC02_OFF_DELAY, i )," XC02_OFF_DELAY" );
		printf(" get down delay. ");
		CHK( M_getstat(G_Path, XC02_OFF_DELAY, &val )," XC02_OFF_DELAY" );
		printf("down delay = %d min:\n", val);
	}
	/* reset to default */
	CHK( M_setstat(G_Path, XC02_OFF_DELAY, 0)," XC02_OFF_DELAY" );


	UOS_Delay(500);
	printf(" ---------- Checking XC02_TIMESTAMP ----------\n");
	val = 0;
	printf(" date/time string: '");
	while (val != 0xff) {
		CHK( M_getstat(G_Path, XC02_TIMESTAMP, &val )," XC02_TIMESTAMP" );
		printf("%c", val);
	}
	printf("'\n");

	UOS_Delay(500);
	printf(" ---------- Checking XC02_IN, XC02_TEMP, XC02_VOLT ----------\n");
	val = 0;
	CHK( M_getstat(G_Path, XC02_IN, &val )," XC02_IN" );
	printf(" XC02_IN: 0x%x \n", val);
    /* temp */
    CHK( (M_getstat(G_Path, XC02_TEMP, &val)),   " XC02_TEMP" );
    printf(" XC02_TEMP: %d °C (raw 0x%x)\n", XC02_ADC2TEMP(val), val);
	val = 0;
    CHK( (M_getstat(G_Path, XC02_VOLTAGE, &val))," XC02_VOLTAGE" );
    printf(" XC02_VOLTAGE: %d mV (raw 0x%x)\n", XC02_ADC2VOLT(val), val);

#define MAXTEMP 77
#define MINTEMP -22
	UOS_Delay(500);
	printf(" ---------- Checking XC02_TEMP_HIGH/LOW ----------\n");
	val = 0;
	CHK( M_getstat(G_Path, XC02_TEMP_HIGH, 	&val )," XC02_TEMP_HIGH" );
	printf(" XC02_TEMP_HIGH: %d\n", XC02_ADC2TEMP(val));
	val = 0;
	CHK( M_getstat(G_Path, XC02_TEMP_LOW, 	&val )," XC02_TEMP_LOW" );
	printf(" XC02_TEMP_LOW: %d\n", XC02_ADC2TEMP(val));
	printf(" Now setting temp range %d - %d °C \n", MINTEMP, MAXTEMP);
	CHK( M_setstat(G_Path, XC02_TEMP_HIGH, 	MAXTEMP )," XC02_TEMP_HIGH" );
	CHK( M_setstat(G_Path, XC02_TEMP_LOW, 	MINTEMP )," XC02_TEMP_LOW" );
	val = 0;
	CHK( M_getstat(G_Path, XC02_TEMP_HIGH, 	&val )," XC02_TEMP_HIGH" );
	printf(" XC02_TEMP_HIGH: %d\n", XC02_ADC2TEMP(val));
	val = 0;
	CHK( M_getstat(G_Path, XC02_TEMP_LOW, 	&val )," XC02_TEMP_LOW" );
	printf(" XC02_TEMP_LOW: %d\n", XC02_ADC2TEMP(val));

	UOS_Delay(500);
	printf(" ---------- Checking XC02_BRIGHTNESS, XC02_SW_DISP ----------\n");
	val = 0;
	CHK( M_getstat(G_Path, XC02_BRIGHTNESS, &val )," XC02_BRIGHTNESS" );
	printf(" XC02_BRIGHTNESS: %d\n", val);
	val = 25;
	printf(" Now set brightness = 25\n" );
	CHK( M_setstat(G_Path, XC02_BRIGHTNESS, 25 )," XC02_BRIGHTNESS" );
	UOS_Delay(500);

	CHK( M_getstat(G_Path, XC02_SW_DISP, &val )," XC02_SW_DISP" );
	printf(" XC02_SW_DISP: %d\n", val);
	UOS_Delay(2000);
	printf(" Now switch display off\n");
	CHK( M_setstat(G_Path, XC02_SW_DISP, 0 ), " XC02_SW_DISP" );
	UOS_Delay(2000);
	CHK( M_getstat(G_Path, XC02_SW_DISP, &val )," XC02_SW_DISP" );
	printf(" XC02_SW_DISP: %d\n", val);
	printf(" Now switch display on\n");
	CHK( M_setstat(G_Path, XC02_SW_DISP, 1 )," XC02_SW_DISP" );
	UOS_Delay(2000);

	printf(" ---------- Checking XC02_BRIGHTNESS, XC02_SW_DISP ----------\n");
	val = 0;
	CHK( M_getstat(G_Path, XC02_BRIGHTNESS, &val )," XC02_BRIGHTNESS" );
	printf(" XC02_BRIGHTNESS: %d\n", val);
	val = 25;
	printf(" Now set brightness = 25\n" );
	CHK( M_setstat(G_Path, XC02_BRIGHTNESS, 25 )," XC02_BRIGHTNESS" );
	UOS_Delay(500);


	return 0;
abort:
	return -1;

}

/********************************* Wait4DownEvent **************************/
/** Wait for shutdown event
 *
 *  \param timeout	\IN  timeout [msec]
 *
 *  \return			0 success or error code
*/
static int Wait4DownEvent( u_int32 timeout )
{
	int32	val;
	u_int32	min;
	double	sec;

	while( TIMER_GET_MSEC < timeout ){

		TIMER_GET_MIN_SEC( min, sec )
			printf("- waiting for down event: %dmin%.3fs\r",
				   min, sec );

		if ((M_getstat(G_Path, XC02_DOWN_EVT, &val)) < 0) {
			printf("                                                       \r");
			PrintMdisError("getstat XC02_DOWN_EVT");
			return 1;
		}

		if( val ){
			printf("                                                       \r");
			return 0;
		}

		UOS_Delay( POLL_TIME );
	}

	TIMER_GET_MIN_SEC( min, sec )
	printf("*** timeout - down event missed after %dmin%.3fs         \n",
		min, sec);

	return 1;
}

/********************************* Wait4PwrOnOff ***************************/
/** Wait for power on/off
*
*  \param state		\IN  state to wait for (PWR_ON or PWR_OFF)
*  \param timeout	\IN  timeout [msec]
*
*  \return		0 success or error code
*/
static int Wait4PwrOnOff( u_int32 state, u_int32 timeout )
{
	int32	val;
	u_int32	min;
	double	sec;

	while( TIMER_GET_MSEC < timeout ){

		TIMER_GET_MIN_SEC( min, sec )
		printf("- waiting for PWR %s: %dmin%.3fs\r",
			((state==PWR_ON) ? "on" : "off"), min, sec );

		if ((M_getstat(G_Path, XC02_TEST3, &val)) < 0) {
			printf("                                                       \r");
			PrintMdisError("getstat XC02_TEST3");
			return 1;
		}

		if( (state==PWR_ON) == (val & XC02C_TEST3_PWR) ){
			printf("                                                       \r");
			return 0;
		}

		UOS_Delay( POLL_TIME );
	}

	TIMER_GET_MIN_SEC( min, sec )
	printf("*** timeout - PWR %s missed after %dmin%.3fs         \n",
		((state==PWR_ON) ? "on" : "off"), min, sec );

	return 1;
}

/********************************* WotDuringDowndelay **********************/
/** WOT during down delay
 *
 *  \param timeout	\IN  timeout [msec]
 *
 *  \return			0 success or error code
*/
static int WotDuringDowndelay( u_int32 timeout )
{

	int32	val;
	u_int32	min;
	double	sec;

	printf("- Note: wot during downDelay\n");

	UOS_Delay( FWARE_DELAY );

	/* firmware must be in SM_STATE_WAIT_SHUTDOWN_DELAY state */
	if ((M_getstat(G_Path, XC02_TEST4, &val)) < 0) {
		PrintMdisError("getstat XC02_TEST4");
		return 1;
	}

	if( val != SM_STATE_WAIT_SD ){
		printf("*** firmware not in SM_STATE_WAIT_SHUTDOWN_DELAY state\n");
		return 1;
	}

	while( TIMER_GET_MSEC < timeout ){

		TIMER_GET_MIN_SEC( min, sec )
		printf("- waiting for wot during down delay: %dmin%.3fs\r",
			min, sec );

		if ((M_getstat(G_Path, XC02_TEST4, &val)) < 0) {
			printf("                                                       \r");
			PrintMdisError("getstat XC02_TEST4");
			return 1;
		}

		if( val==SM_STATE_ON ){
			printf("                                                       \r");
			return 0;
		}

		UOS_Delay( POLL_TIME );
	}

	TIMER_GET_MIN_SEC( min, sec )
	printf("*** timeout - wot missed after %dmin%.3fs         \n",
		min, sec);

	return 1;
}

/********************************* TestOffAck ******************************/
/** Test off acknowledge
*
*  \return			0 success or error code
*/
static int TestOffAck( void )
{
	u_int32	min;
	double	sec;

	printf("=== test OFFACK ===\n");

	/* set shutdown delay */
	printf("- set shutdown delay: none\n");
	if( (M_setstat(G_Path, XC02_DOWN_DELAY, XC02C_DOWN_DELAY_MINMODE)) < 0) {
		PrintMdisError("setstat XC02_DOWN_DELAY");
		return 1;
	}

	/* set off delay */
	printf("- set off delay: maximum\n");
	if( (M_setstat(G_Path, XC02_OFF_DELAY, XC02C_OD_MAXMODE)) < 0) {
		PrintMdisError("setstat XC02_OFF_DELAY");
		return 1;
	}

	/* initiate a software shutdown */
	printf("- send SWOFF\n");
	if( (M_setstat(G_Path, XC02_SWOFF, 0)) < 0) {
		PrintMdisError("setstat XC02_SWOFF");
		return 1;
	}

	G_TimeStart = UOS_MsecTimerGet();
	if( (Wait4DownEvent( DELAYTIME )) )
		return 1;

	TIMER_GET_MIN_SEC( min, sec );
	printf("- DOWN_EVENT %dmin%.3fs after SWOFF\n", min, sec);

	/* send OFFACK */
	printf("- send OFFACK\n");
	if( (M_setstat(G_Path, XC02_OFFACK, 0)) < 0) {
		PrintMdisError("setstat XC02_OFFACK");
		return 1;
	}

	G_TimeStart = UOS_MsecTimerGet();
	if( (Wait4PwrOnOff( PWR_OFF, DELAYTIME )) )
		return 1;

	TIMER_GET_MIN_SEC( min, sec );
	printf("- PWR_OFF %dmin%.3fs after OFFACK\n", min, sec);

	/* immediate SWON (for test only!) */
	printf("- immediate SWON (for test only!)\n");
	if( (M_setstat(G_Path, XC02_TEST2, XC02C_TEST2_SWON)) < 0) {
			PrintMdisError("setstat XC02_TEST2");
		return 1;
	}

	G_TimeStart = UOS_MsecTimerGet();
	if( (Wait4PwrOnOff( PWR_ON, TOUT_MAX( XC02C_WAIT_OFF ) )) )
		return 1;

	TIMER_GET_MIN_SEC( min, sec );
	printf("- PWR_ON %dmin%.3fs after SWON\n", min, sec);

	return 0;
}


/********************************* TestWdog ********************************/
/** Test watchdog
*
*  \param testLevel		\IN  test level
*
*  \return			0 success or error code
*/
static int TestWdog( u_int32 testLevel )
{

	u_int32	min;
	double	sec;
	u_int32	rst, errOff, ackErr, wdogTime, n;
	u_int32	expectedWdogResets=0;
	int32 wdogErr;

	printf("=== test watchdog ===\n");

	/*-------------------------------+
	|  trigger before timeout        |
	+-------------------------------*/
	printf("\n--- trigger before wdog timeout ---\n");

	wdogTime = 2000;	/* 2s */

	/* set wdog time */
	printf("- set wdog time=%dms\n", wdogTime);
	if( (M_setstat(G_Path, WDOG_TIME, wdogTime)) < 0) {
		PrintMdisError("setstat WDOG_TIME");
		return 1;
	}

	/* enable watchdog */
	if( WdogOnOff( 1 ) )
		return 1;

	G_TimeStart = UOS_MsecTimerGet();

	for( n=0; n<5; n++ ){

		/* trigger watchdog */
		TIMER_GET_MIN_SEC( min, sec )
		printf("- trigger watchdog %dmin%.3fs after start/trigger\n",
			min, sec );
		if( (M_setstat(G_Path, WDOG_TRIG, 0)) < 0) {
			PrintMdisError("setstat WDOG_TRIG");
			WdogOnOff( 0 );
			return 1;
		}

		/* check state */
		if( (CheckRstErroffErrcount( &rst, &errOff, &ackErr, &wdogErr )) ){
			WdogOnOff( 0 );
			return 1;
		}
		if( rst || errOff || wdogErr ){
			printf("*** unexpected reset=%d/err-off=%d/wdogErr=%d\n",
				rst, errOff, wdogErr);
			WdogOnOff( 0 );
			return 1;
		}

		printf("- waiting %dms before trigger\n", wdogTime/2 );
		G_TimeStart = UOS_MsecTimerGet();
		UOS_Delay( wdogTime/2 );
	}

	/* disable watchdog */
	if( WdogOnOff( 0 ) )
		return 1;

	printf("- waiting %dms before state check\n", wdogTime*2 );
	UOS_Delay( wdogTime*2 );

	/* check state */
	if( (CheckRstErroffErrcount( &rst, &errOff, &ackErr, &wdogErr )) )
		return 1;
	if( rst || errOff || wdogErr ){
		printf("*** unexpected reset=%d/err-off=%d/wdogErr=%d\n",
			rst, errOff, wdogErr);
		return 1;
	}

	/*-------------------------------+
	|  check wdog timeout            |
	+-------------------------------*/
	printf("\n--- check wdog timeout ---\n");

	for( wdogTime=500; (int32)wdogTime <= ((testLevel>1) ? 25500 : 10000);
		 wdogTime+= ((testLevel>1) ?  3500 :  2500)){

		/* set wdog time */
		printf("\n- set wdog time=%dms\n", wdogTime);
		if( (M_setstat(G_Path, WDOG_TIME, wdogTime)) < 0) {
			PrintMdisError("setstat WDOG_TIME");
			return 1;
		}

		/* enable watchdog */
		if( WdogOnOff( 1 ) )
			return 1;

		G_TimeStart = UOS_MsecTimerGet();

		/* state: after PWR_ON or RESET */
		while( 1 ){
			TIMER_GET_MIN_SEC( min, sec )
			printf("- waiting for wdog error: %dmin%.3fs\r",
				min, sec );

			/* check state */
			if( (CheckRstErroffErrcount( &rst, &errOff, &ackErr, &wdogErr )) ){
				WdogOnOff( 0 );
				return 1;
			}

			/* time-to-short? */
			if( (rst || errOff) && ( TIMER_GET_MSEC < TOUT_MIN(wdogTime) )){
				TIMER_GET_MIN_SEC( min, sec )
					printf("*** WARNING: time-to-short - reset/error-off after %dmin%.3fs\n",
					min, sec);
			}

			/* reset? */
			if( rst ){
				expectedWdogResets++;

				TIMER_GET_MIN_SEC( min, sec )
				printf("- reset #%d %dmin%.3fs after PWR_ON/RESET           \n",
					expectedWdogResets, min, sec );

				/* check error count */
				if( expectedWdogResets != wdogErr ){
					printf("*** wdogErr=%d but %d expected                 \n",
						wdogErr, expectedWdogResets);
					WdogOnOff( 0 );
					return 1;
				}
				if( wdogErr > XC02C_WDOG_MAX_ERR ){
					printf("*** wdogErr=%d but max-err=%d                  \n",
						wdogErr, XC02C_WDOG_MAX_ERR);
					WdogOnOff( 0 );
					return 1;
				}

				G_TimeStart = UOS_MsecTimerGet();
			}

			/* err-off? */
			if( errOff ){
				expectedWdogResets=0;

				TIMER_GET_MIN_SEC( min, sec )
				printf("- error-off %dmin%.3fs after PWR_ON/RESET           \n",
					min, sec );

				/* check error count */
				if( expectedWdogResets != wdogErr ){
					printf("*** wdogErr=%d but %d expected\n", wdogErr, expectedWdogResets);
					WdogOnOff( 0 );
					return 1;
				}
				break;
			}

			/* time-to-long? */
			if( TIMER_GET_MSEC > TOUT_MAX(wdogTime) ){
				TIMER_GET_MIN_SEC( min, sec )
				printf("*** time-to-long - reset/error-off missed after %dmin%.3fs\n",
					min, sec);
				WdogOnOff( 0 );
				return 1;
			}

			UOS_Delay( POLL_TIME );
		} /* while */

		/* disable watchdog */
		if( WdogOnOff( 0 ) )
			return 1;
	}

	/*-------------------------------+
	|  trigger after reset           |
	+-------------------------------*/
	printf("\n--- test trigger after reset ---\n");

	wdogTime = 6000;	/* 6s */

	/* set wdog time */
	printf("- set wdog time=%dms\n", wdogTime);
	if( (M_setstat(G_Path, WDOG_TIME, wdogTime)) < 0) {
		PrintMdisError("setstat WDOG_TIME");
		return 1;
	}

	/* enable watchdog */
	if( WdogOnOff( 1 ) )
		return 1;

	G_TimeStart = UOS_MsecTimerGet();

	while( 1 ){
		TIMER_GET_MIN_SEC( min, sec )
		printf("- waiting for wdog error: %dmin%.3fs\r",
			min, sec );

		/* check state */
		if( (CheckRstErroffErrcount( &rst, &errOff, &ackErr, &wdogErr )) ){
			WdogOnOff( 0 );
			return 1;
		}

		/* reset? */
		if( rst ){
			expectedWdogResets++;

			TIMER_GET_MIN_SEC( min, sec )
			printf("- reset #%d %dmin%.3fs after PWR_ON/RESET \n",
				expectedWdogResets, min, sec );

			/* check error count */
			if( expectedWdogResets != wdogErr ){
				printf("*** wdogErr=%d but %d expected \n",
					wdogErr, expectedWdogResets);
				WdogOnOff( 0 );
				return 1;
			}
			if( wdogErr > XC02C_WDOG_MAX_ERR ){
				printf("*** wdogErr=%d but max-err=%d\n",
					wdogErr, XC02C_WDOG_MAX_ERR);
				WdogOnOff( 0 );
				return 1;
			}

			G_TimeStart = UOS_MsecTimerGet();
		}

		if( wdogErr == XC02C_WDOG_MAX_ERR-1 ){

			printf("- waiting %dms before trigger\n", wdogTime*2/3 );
			UOS_Delay( wdogTime*2/3 );

			/* trigger watchdog */
			printf("- trigger watchdog\n");
			if( (M_setstat(G_Path, WDOG_TRIG, 0)) < 0) {
				PrintMdisError("setstat WDOG_TRIG");
				WdogOnOff( 0 );
				return 1;
			}

			printf("- waiting %dms before wdog disable\n", wdogTime*2/3 );
			UOS_Delay( wdogTime*2/3 );

			/* disable watchdog */
			if( WdogOnOff( 0 ) )
				return 1;

			printf("- waiting %dms before state check\n", wdogTime*2/3 );
			UOS_Delay( wdogTime*2/3 );

			/* check state */
			if( (CheckRstErroffErrcount( &rst, &errOff, &ackErr, &wdogErr )) )
				return 1;

			if( rst || errOff || (wdogErr != XC02C_WDOG_MAX_ERR-1) ){
				printf("*** unexpected reset=%d/err-off=%d/wdogErr=%d\n",
					rst, errOff, wdogErr);
				return 1;
			}

			return 0;
		}

		/* time-to-long? */
		if( TIMER_GET_MSEC > TOUT_MAX(wdogTime) ){
			TIMER_GET_MIN_SEC( min, sec )
			printf("*** time-to-long - reset/error-off missed after %dmin%.3fs\n",
				min, sec);
			return 1;
		}

		UOS_Delay( POLL_TIME );
	} /* while */

	return 0;
}

/********************************* CheckRstErroffErrcount ******************/
/** Check reset, error-off, ONACK error count, WDOG error count
*
*  \param rst		\OUT  <>1: reset was set
*  \param errOff	\OUT  <>1: error-off was set
*  \param ackErr	\OUT  ONACK error count
*  \param wdogErr	\OUT  WDOG error count
*
*  \return		0 success or error code
*/
static int CheckRstErroffErrcount(
	u_int32		*rst,
	u_int32		*errOff,
	u_int32		*ackErr,
	int32		*wdogErr )
{
	int32	val;


	/* number of missing watchdog triggers - ckeck before reset flag */
	if ((M_getstat(G_Path, XC02_WDOG_ERR, wdogErr)) < 0) {
		PrintMdisError("getstat XC02_WDOG_ERR");
		return 1;
	}

	/* check error OFF state */
	if ((M_getstat(G_Path, XC02_TEST4, &val)) < 0) {
		PrintMdisError("getstat XC02_TEST4");
		return 1;
	}
	*errOff = (val == SM_STATE_ERROR_OFF) ? 1 : 0;
	if( *errOff ){
		u_int32	oldTimeStart;
		/* check power */
		if ((M_getstat(G_Path, XC02_TEST3, &val)) < 0) {
			PrintMdisError("getstat XC02_TEST3");
			return 1;
		}
		if( val & XC02C_TEST3_PWR ){
			printf("*** error OFF state <==> power is on\n");
			return 1;
		}

		/* toggle test mode to restart */
		if( (M_setstat(G_Path, XC02_TEST1, XC02C_TEST1_NOTEST)) < 0) {
				PrintMdisError("setstat XC02_TEST1");
			return 1;
		}
		if( (M_setstat(G_Path, XC02_TEST1, XC02C_TEST1_TMODE1)) < 0) {
				PrintMdisError("setstat XC02_TEST1");
			return 1;
		}
		/* save and restore old TimeStart for caller */
		oldTimeStart = G_TimeStart;
		G_TimeStart = UOS_MsecTimerGet();
		if( (Wait4PwrOnOff( PWR_ON, DELAYTIME )) )
			return 1;
		G_TimeStart = oldTimeStart;

		/* number of missing watchdog triggers - update after restart */
		if ((M_getstat(G_Path, XC02_WDOG_ERR, wdogErr)) < 0) {
			PrintMdisError("getstat XC02_WDOG_ERR");
			return 1;
		}
	}

	/* check reset */
	if ((M_getstat(G_Path, XC02_TEST2, &val)) < 0) {
		PrintMdisError("getstat XC02_TEST2");
		return 1;
	}

	*rst = (val & XC02C_TEST2_RST) ? 1 : 0;
	if( *rst ){
		/* clear reset flag */
		if( (M_setstat(G_Path, XC02_TEST2, 0x00)) < 0) {
				PrintMdisError("setstat XC02_TEST2");
			return 1;
		}

		/* number of missing watchdog triggers - update after reset */
		if ((M_getstat(G_Path, XC02_WDOG_ERR, wdogErr)) < 0) {
			PrintMdisError("getstat XC02_WDOG_ERR");
			return 1;
		}
	}

	return 0;
}

#if 0
/********************************* Restart *********************************/
/** Restart (SWOFF, SWON)
*
*  \return			0 success or error code
*/
static int Restart( void )
{
	u_int32	min;
	double	sec;

	/* initiate a software shutdown */
	printf("- send SWOFF\n");
	if( (M_setstat(G_Path, XC02_SWOFF, 0)) < 0) {
		PrintMdisError("setstat XC02_SWOFF");
		return 1;
	}

	G_TimeStart = UOS_MsecTimerGet();
	if( (Wait4DownEvent( DELAYTIME )) )
		return 1;

	TIMER_GET_MIN_SEC( min, sec );
	printf("- PWR_OFF %dmin%.3fs after SWOFF\n", min, sec);

	/* immediate SWON (for test only!) */
	printf("- immediate SWON (for test only!)\n");
	if( (M_setstat(G_Path, XC02_TEST2, XC02C_TEST2_SWON)) < 0) {
			PrintMdisError("setstat XC02_TEST2");
		return 1;
	}

	G_TimeStart = UOS_MsecTimerGet();
	if( (Wait4PwrOnOff( PWR_ON, TOUT_MAX( XC02C_WAIT_OFF ) )) )
		return 1;

	TIMER_GET_MIN_SEC( min, sec );
	printf("- PWR_ON %dmin%.3fs after SWON\n", min, sec);

	return 0;
}
#endif

/********************************* WdogOnOff *******************************/
/** Enable/disable watchdog
*
*  \param enabel	\IN  1=enable, 0=disable
*
*  \return			0 success or error code
*/
static int WdogOnOff( u_int32 enable )
{
	if( enable ){
		/* enable watchdog */
		printf("- enable watchdog\n");
		if( (M_setstat(G_Path, WDOG_START, 0)) < 0) {
			PrintMdisError("setstat WDOG_START");
			return 1;
		}
	}
	else {
		/* disable watchdog */
		printf("- disable watchdog\n");
		if( (M_setstat(G_Path, WDOG_STOP, 0)) < 0) {
			PrintMdisError("setstat WDOG_STOP");
			return 1;
		}
	}

	return 0;
}
