/*********************  P r o g r a m  -  M o d u l e ***********************/
/*!
 *        \file  xc02_drv.c
 *
 *      \author  thomas.schnuerer@men.de
 *
 *      \brief   Low-level driver for PIC on SMBus at XC02 MEN DC carrier
 *
 *     Required: OSS, DESC, DBG, libraries
 *
 *     \switches _ONE_NAMESPACE_PER_DRIVER_
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

#define _NO_LL_HANDLE		/* ll_defs.h: don't define LL_HANDLE struct */

#ifdef XC02_SW				/* swapped variant */
# define MAC_MEM_MAPPED
# define ID_SW
#endif

#include <MEN/men_typs.h>   /* system dependent definitions */
#include <MEN/maccess.h>    /* hw access macros and types */
#include <MEN/dbg.h>        /* debug functions */
#include <MEN/oss.h>        /* oss functions */
#include <MEN/desc.h>       /* descriptor functions */
#include <MEN/mdis_api.h>   /* MDIS global defs */
#include <MEN/mdis_com.h>   /* MDIS common defs */
#include <MEN/mdis_err.h>   /* MDIS error codes */
#include <MEN/ll_defs.h>    /* low-level driver definitions */
#include <MEN/smb2.h>		/* SMB2 definitions */
#include <MEN/xc02.h>		/* XC02 controller definitions - PIC side */
#include <MEN/wdog.h>		/* watchdog specific definitions  */

/*-----------------------------------------+
|  DEFINES                                 |
+-----------------------------------------*/
/* general defines */
#define CH_NUMBER		4		/**< Nr. of channels, prepare for 4 displays */
#define CH_BYTES		1		/**< Number of bytes per channel */
#define USE_IRQ			FALSE	/**< Interrupt required  */
#define ADDRSPACE_COUNT	0		/**< Number of required address spaces */
#define ADDRSPACE_SIZE	0		/**< Size of address space */
#define DEV_ID			0xc2	/**< device ID: "XC2" */

/* debug defines */
#define DBG_MYLEVEL			llHdl->dbgLevel   /**< Debug level */
#define DBH					llHdl->dbgHdl     /**< Debug handle */

/* SMB access macros */
#define SMB_W_BYTE( cmd, val ) \
	error = llHdl->smbH->WriteByteData( llHdl->smbH, 0, \
		llHdl->smbAddr, (u_int8)(cmd), (u_int8)(val) );

#define SMB_R_BYTE( cmd, valP ) \
	*valP = 0; \
	error = llHdl->smbH->ReadByteData(llHdl->smbH, 0, \
		llHdl->smbAddr,(u_int8)(cmd),(u_int8*)(valP));

/* helper */
#define NO_KEY 	ERR_DESC_KEY_NOTFOUND

/*-----------------------------------------+
|  TYPEDEFS                                |
+-----------------------------------------*/
/** low-level handle */
typedef struct {
	/* general */
    int32           		memAlloc;	/**< Size allocated for the handle */
    OSS_HANDLE      		*osHdl;     /**< OSS handle */
    OSS_IRQ_HANDLE  		*irqHdl;    /**< IRQ handle */
    DESC_HANDLE     		*descHdl;   /**< DESC handle */
	MDIS_IDENT_FUNCT_TBL 	idFuncTbl;	/**< ID function table */
    u_int32         		idCheck;	/**< id check enabled */
	/* debug */
    u_int32         		dbgLevel;	/**< Debug level */
	DBG_HANDLE      		*dbgHdl;    /**< Debug handle */
	/* FW(PIC) specific */
	SMB_HANDLE				*smbH;		/**< ptr to SMB_HANDLE struct */
	u_int16					smbAddr;	/**< SMB address of XC02 */
    OSS_SIG_HANDLE  		*sigHdl;    /**< signal handle */
    OSS_ALARM_HANDLE 		*alarmHdl;	/**< alarm handle */
	u_int8					wdState;	/**< Watchdog state */
} LL_HANDLE;

/* include files which need LL_HANDLE */
#include <MEN/ll_entry.h>   /* low-level driver jump table  */
#include <MEN/xc02_drv.h>	/* XC02 driver header file */

static const char IdentString[]=MENT_XSTR(MAK_REVISION);

/*-----------------------------------------+
|  PROTOTYPES                              |
+-----------------------------------------*/
static int32 XC02_Init(DESC_SPEC *descSpec, OSS_HANDLE *osHdl,
					   MACCESS *ma, OSS_SEM_HANDLE *devSemHdl,
					   OSS_IRQ_HANDLE *irqHdl, LL_HANDLE **llHdlP);
static int32 XC02_Exit(LL_HANDLE **llHdlP );
static int32 XC02_Read(LL_HANDLE *llHdl, int32 ch, int32 *value);
static int32 XC02_Write(LL_HANDLE *llHdl, int32 ch, int32 value);
static int32 XC02_SetStat(LL_HANDLE *llHdl,int32 ch, int32 code, INT32_OR_64 value32_or_64);
static int32 XC02_GetStat(LL_HANDLE *llHdl, int32 ch, int32 code, INT32_OR_64 *value32_or_64P);
static int32 XC02_BlockRead(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							int32 *nbrRdBytesP);
static int32 XC02_BlockWrite(LL_HANDLE *llHdl, int32 ch, void *buf, int32 size,
							 int32 *nbrWrBytesP);
static int32 XC02_Irq(LL_HANDLE *llHdl );
static int32 XC02_Info(int32 infoType, ... );

static char* Ident( void );
static int32 Cleanup(LL_HANDLE *llHdl, int32 retCode);

/* XC02 specific helper functions */
static void AlarmHandler(void *arg);

/****************************** XC02_GetEntry ********************************/
/** Initialize driver's jump table
 *
 *  \param drvP     \OUT Pointer to the initialized jump table structure
 */
#ifdef _ONE_NAMESPACE_PER_DRIVER_
    void LL_GetEntry( LL_ENTRY* drvP )
#else
    void __XC02_GetEntry( LL_ENTRY* drvP )
#endif /* _ONE_NAMESPACE_PER_DRIVER_ */
{
    drvP->init        = XC02_Init;
    drvP->exit        = XC02_Exit;
    drvP->read        = XC02_Read;
    drvP->write       = XC02_Write;
    drvP->blockRead   = XC02_BlockRead;
    drvP->blockWrite  = XC02_BlockWrite;
    drvP->setStat     = XC02_SetStat;
    drvP->getStat     = XC02_GetStat;
    drvP->irq         = XC02_Irq;
    drvP->info        = XC02_Info;
}


/******************************** XC02_Init **********************************/
/** Allocate and return low-level handle, initialize hardware
 *
 * The function initializes the XC02 device with the definitions made
 * in the descriptor.
 *
 * The function decodes \ref descriptor_entries "these descriptor entries"
 * in addition to the general descriptor keys.
 *
 *  \param descP      \IN  Pointer to descriptor data
 *  \param osHdl      \IN  OSS handle
 *  \param ma         \IN  HW access handle
 *  \param devSemHdl  \IN  Device semaphore handle
 *  \param irqHdl     \IN  IRQ handle
 *  \param llHdlP     \OUT Pointer to low-level driver handle
 *
 *  \return           \c 0 On success or error code
 */
static int32 XC02_Init(
    DESC_SPEC       *descP,
    OSS_HANDLE      *osHdl,
    MACCESS         *ma,
    OSS_SEM_HANDLE  *devSemHdl,
    OSS_IRQ_HANDLE  *irqHdl,
    LL_HANDLE       **llHdlP
)
{
    LL_HANDLE	*llHdl = NULL;
    u_int32		gotsize, smbBusNbr;
    int32		error;
	u_int8		value8;
	u_int32		value32,i;
	u_int32 	wdogTout, downDelay, offDelay,tempHigh,tempLow;
	u_int32 	voltHigh, voltLow, brightSource, initState;
	u_int32 	initBright1, initBright2;

    /*------------------------------+
    |  prepare the handle           |
    +------------------------------*/
	*llHdlP = NULL;

	/* alloc */
    if((llHdl = (LL_HANDLE*)OSS_MemGet(
			osHdl, sizeof(LL_HANDLE), &gotsize)) == NULL)
       return(ERR_OSS_MEM_ALLOC);

	/* clear */
    OSS_MemFill(osHdl, gotsize, (char*)llHdl, 0x00);

	/* init */
    llHdl->memAlloc   = gotsize;
    llHdl->osHdl      = osHdl;
    llHdl->irqHdl     = irqHdl;

    /*------------------------------+
    |  init id function table       |
    +------------------------------*/
	/* driver's ident function */
	llHdl->idFuncTbl.idCall[0].identCall = Ident;
	/* library's ident functions */
	llHdl->idFuncTbl.idCall[1].identCall = DESC_Ident;
	llHdl->idFuncTbl.idCall[2].identCall = OSS_Ident;
	/* terminator */
	llHdl->idFuncTbl.idCall[3].identCall = NULL;

    /*------------------------------+
    |  prepare debugging            |
    +------------------------------*/
	DBG_MYLEVEL = OSS_DBG_DEFAULT;	/* set OS specific debug level */
	DBGINIT((NULL,&DBH));


    /*------------------------------+
    |  scan descriptor              |
    +------------------------------*/
	/* prepare access */
    if((error = DESC_Init(descP, osHdl, &llHdl->descHdl)))
		return( Cleanup(llHdl,error) );

    /* DEBUG_LEVEL_DESC */
    if((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
								&value32, "DEBUG_LEVEL_DESC")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	DESC_DbgLevelSet(llHdl->descHdl, value32);	/* set level */

	/* generic descriptors */

    /* DEBUG_LEVEL */
    if((error = DESC_GetUInt32(llHdl->descHdl, OSS_DBG_DEFAULT,
							   &llHdl->dbgLevel, "DEBUG_LEVEL")) &&
	   error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

    DBGWRT_1((DBH, "LL - XC02_Init\n"));

    /* ID_CHECK */
    if ((error = DESC_GetUInt32(llHdl->descHdl, TRUE,
								&llHdl->idCheck, "ID_CHECK")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	/**
	 *	required descriptors
	 */

    /* SMB_BUSNBR */
    if((error = DESC_GetUInt32(llHdl->descHdl, 0, &smbBusNbr, "SMB_BUSNBR")))
		return( Cleanup(llHdl,error) );

    /* SMB_DEVADDR */
    if((error = DESC_GetUInt32(llHdl->descHdl, 0, &value32, "SMB_DEVADDR")))
		return( Cleanup(llHdl,error) );
	llHdl->smbAddr = (u_int16)value32;


	/***
	 * additional descriptors, set to defaults if omitted in system.dsc. The
	 * Keys are written to HW only when they were present
	 */

    /* WDOG_TOUT in 100ms units */
    if((error=DESC_GetUInt32(llHdl->descHdl, XC02C_WDOG_TOUT_MAXMODE,
							 &wdogTout, "WDOG_TOUT")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	if((wdogTout<XC02C_WDOG_TOUT_MINMODE) || (wdogTout>XC02C_WDOG_TOUT_MAXMODE))
		return( Cleanup(llHdl,ERR_LL_DESC_PARAM) );


    /* DOWN_DELAY */
    if((error = DESC_GetUInt32(llHdl->descHdl, XC02C_DOWN_DELAY_MINMODE,
							   &downDelay, "DOWN_DELAY")) &&
		error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );
	if( ((int32)downDelay < XC02C_DOWN_DELAY_MINMODE) ||
		(downDelay > XC02C_DOWN_DELAY_MAXMODE) )
		return( Cleanup(llHdl,ERR_LL_DESC_PARAM) );

    /* OFF_DELAY */
    if((error = DESC_GetUInt32(llHdl->descHdl, XC02C_OD_MINMODE,
							   &offDelay, "OFF_DELAY")) &&
	   error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );
	if( ((int32)offDelay < XC02C_OD_MINMODE) || (offDelay > XC02C_OD_MAXMODE) )
		return( Cleanup(llHdl,ERR_LL_DESC_PARAM) );

	/* BRIGHT_SOURCE */
    if((error = DESC_GetUInt32(llHdl->descHdl, XC02_DEFAULT_BCON_SRC,
							   &brightSource, "BRIGHT_SOURCE")) &&
	   error!=ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error));

	/* TEMP_HIGH  */
    if((error =DESC_GetUInt32(llHdl->descHdl, XC02C_TEMP_MAX,
							  &tempHigh, "TEMP_HIGH")) &&
	   error != ERR_DESC_KEY_NOTFOUND)
		return( Cleanup(llHdl,error) );

	/* TEMP_LOW  */
    if((error = DESC_GetUInt32(llHdl->descHdl, XC02C_TEMP_MIN,
							   &tempLow, "TEMP_LOW")) &&
	   error != ERR_DESC_KEY_NOTFOUND  )
		return( Cleanup(llHdl,error) );


	/* VOLT_HIGH  */
    if((error = DESC_GetUInt32(llHdl->descHdl, XC02C_VOLT_MAX,
							   &voltHigh, "VOLT_HIGH")) &&
	   error != ERR_DESC_KEY_NOTFOUND )
		return( Cleanup(llHdl,error) );


	/* VOLT_LOW */
    if((error = DESC_GetUInt32(llHdl->descHdl, XC02C_VOLT_MIN,
							   &voltLow, "VOLT_LOW")) &&
	   error != ERR_DESC_KEY_NOTFOUND )
		return( Cleanup(llHdl,error) );

	/* INIT_DISPSTAT */
    if((error = DESC_GetUInt32(llHdl->descHdl, XC02_DEFAULT_DISP_STAT,
							   &initState, "INIT_DISPSTAT")) &&
	   error != ERR_DESC_KEY_NOTFOUND )
		return( Cleanup(llHdl,error));

	/* INIT_BRIGHT_1 */
    if((error = DESC_GetUInt32(llHdl->descHdl, XC02_DEFAULT_BRIGHTNESS,
							   &initBright1, "INIT_BRIGHT_1")) &&
	   error != ERR_DESC_KEY_NOTFOUND )
		return( Cleanup(llHdl,error));

	/* INIT_BRIGHT_2 */
    if((error = DESC_GetUInt32(llHdl->descHdl, XC02_DEFAULT_BRIGHTNESS,
							   &initBright2, "INIT_BRIGHT_2")) &&
	   error != ERR_DESC_KEY_NOTFOUND )
		return( Cleanup(llHdl,error));

    /*------------------------------+
    |  get SMB handle               |
    +------------------------------*/
	if((error = OSS_GetSmbHdl( llHdl->osHdl, smbBusNbr, (void**)&llHdl->smbH)))
		return( Cleanup(llHdl,error) );


    /*------------------------------+
    |  check module id              |
    +------------------------------*/
	if (llHdl->idCheck) {
		/* we expect 0xc2 */
		SMB_R_BYTE( XC02C_ID, &value8 );
		if( error )
			return( error );

		if (value8 != DEV_ID) {
			DBGWRT_ERR((DBH," *** XC02_Init: illegal id=0x%x\n",value8));
			error = ERR_LL_ILL_ID;
			return( Cleanup(llHdl,error) );
		}
	}

	/* get and print firmware revision */
	SMB_R_BYTE( XC02C_REV, &value8 );
	DBGWRT_2((DBH, "XC02C_REV=0x%02x\n", value8));

    /*------------------------------+
    |  init alarm                   |
    +------------------------------*/
	if((error = OSS_AlarmCreate(llHdl->osHdl, AlarmHandler, llHdl,
								&llHdl->alarmHdl)) )
		return( Cleanup(llHdl,error));


    /*------------------------------+
    |  init hardware                |
    +------------------------------*/
	DBGWRT_3((DBH, "XC02_Init - setting descriptor defaults:\n"));

	/* write values physically to hardware only if present */
  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "WDOG_TOUT") != NO_KEY ) {
		DBGWRT_3((DBH, "    set WDOG_TOUT    = 0x%02x\n", wdogTout ));
		SMB_W_BYTE( XC02C_WDOG_TOUT, wdogTout );
		if( error  )
			return( Cleanup(llHdl,error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "DOWN_DELAY") != NO_KEY ) {
		DBGWRT_3((DBH, "    set DOWN_DELAY   = 0x%02x\n", downDelay ));
		SMB_W_BYTE( XC02C_DOWN_DELAY, downDelay );
		if( error  )
			return( Cleanup(llHdl,error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "OFF_DELAY") != NO_KEY ) {
		DBGWRT_3((DBH, "    set OFF_DELAY    = 0x%02x\n", offDelay ));
		SMB_W_BYTE( XC02C_OFF_DELAY, offDelay );
		if( error  )
			return( Cleanup(llHdl,error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "BRIGHT_SOURCE") != NO_KEY ) {

		DBGWRT_3((DBH, "    set XC02C_BR_SRC = 0x%02x\n", brightSource ));
		SMB_W_BYTE(	XC02C_BR_SRC, brightSource );
		if( error  )
			return( Cleanup(llHdl,error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "TEMP_HIGH") != NO_KEY ) {
		DBGWRT_3((DBH, "    set TEMP_HIGH    = 0x%02x\n", tempHigh ));
		SMB_W_BYTE( XC02C_TEMP_HIGH, tempHigh );
		if( error  )
			return( Cleanup(llHdl,error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "TEMP_LOW") != NO_KEY ) {
		DBGWRT_3((DBH, "    set TEMP_LOW     = 0x%02x\n", tempLow ));
		SMB_W_BYTE( XC02C_TEMP_LOW, tempLow );
		if( error  )
			return( Cleanup(llHdl,error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "VOLT_HIGH") != NO_KEY ) {
		DBGWRT_3((DBH, "    set VOLT_HIGH    = 0x%02x\n", voltHigh ));
		SMB_W_BYTE( XC02C_VOLT_HIGH, voltHigh );
		if( error  )
			return( Cleanup(llHdl,error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "VOLT_LOW") != NO_KEY ) {
		DBGWRT_3((DBH, "    set VOLT_LOW     = 0x%02x\n", voltLow ));
		SMB_W_BYTE( XC02C_VOLT_LOW, voltLow );
		if( error  )
			return( Cleanup(llHdl,error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "INIT_DISPSTAT") != NO_KEY ) {
		DBGWRT_3((DBH, "    set XC02C_INIT_DS = 0x%02x\n", initState ));
		SMB_W_BYTE(XC02C_INIT_DS, initState );
		if( error  )
			return( Cleanup(llHdl, error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "INIT_BRIGHT_1") != NO_KEY ) {
		DBGWRT_3((DBH, "    set INIT_BRIGHT_1 = 0x%02x\n", initBright1 ));
		SMB_W_BYTE( XC02C_INIT_BR1, initBright1 );
		if( error  )
			return( Cleanup(llHdl, error) );
	}

  	if (DESC_GetUInt32(llHdl->descHdl, 0x0, &i, "INIT_BRIGHT_2") != NO_KEY ) {
		DBGWRT_3((DBH, "    set INIT_BRIGHT_2 = 0x%02x\n", initBright2 ));
		SMB_W_BYTE( XC02C_INIT_BR2, initBright1 );
		if( error  )
			return( Cleanup(llHdl,error) );
	}

	*llHdlP = llHdl;	/* set low-level driver handle */

	return(ERR_SUCCESS);
}

/****************************** XC02_Exit ************************************/
/** De-initialize hardware and clean up memory
 *
 *  The function deinitializes the XC02 device.
 *
 *  \param llHdlP      \IN  Pointer to low-level driver handle
 *
 *  \return           \c 0 On success or error code
 */
static int32 XC02_Exit(
   LL_HANDLE    **llHdlP
)
{
    LL_HANDLE *llHdl = *llHdlP;
	int32 error = 0;

    DBGWRT_1((DBH, "LL - XC02_Exit\n"));

    /*------------------------------+
    |  de-init hardware             |
    +------------------------------*/

    /*------------------------------+
    |  clean up memory               |
    +------------------------------*/
	*llHdlP = NULL;		/* set low-level driver handle to NULL */
	error = Cleanup(llHdl,error);

	return(error);
}

/****************************** XC02_Read ************************************/
/** Read a value from the device
 *
 *  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
 *
 *  \param llHdl      \IN  Low-level handle
 *  \param ch         \IN  Current channel
 *  \param valueP     \OUT Read value
 *
 *  \return           \c 0 On success or error code
 */
static int32 XC02_Read(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 *valueP
)
{
    DBGWRT_1((DBH, "LL - XC02_Read: ch=%d\n",ch));

	return(ERR_LL_ILL_FUNC);
}

/****************************** XC02_Write ***********************************/
/** Write a value to the device
 *
 *  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
 *
 *  \param llHdl      \IN  Low-level handle
 *  \param ch         \IN  Current channel
 *  \param value      \IN  Read value
 *
 *  \return           \c ERR_LL_ILL_FUNC
 */
static int32 XC02_Write(
    LL_HANDLE *llHdl,
    int32 ch,
    int32 value
)
{
    DBGWRT_1((DBH, "LL - XC02_Write: ch=%d\n",ch));

	return(ERR_LL_ILL_FUNC);
}

/****************************** XC02_SetStat *********************************/
/** Set the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl  	      \IN  Low-level handle
 *  \param code           \IN  \ref getstat_setstat_codes "status code"
 *  \param ch             \IN  Current channel
 *  \param value32_or_64  \IN  Data or
 *                         pointer to block data structure (M_SG_BLOCK) for
 *                         block status codes
 *  \return           \c 0 On success or error code
 */
static int32 XC02_SetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 value32_or_64
)
{
	int32	error = ERR_SUCCESS;
	u_int32 wdtime=0;

	int32	    value  = (int32)value32_or_64;	/* 32bit value */
	INT32_OR_64	valueP = value32_or_64;	        /* stores 32/64bit pointer */

    DBGWRT_1((DBH, "LL - XC02_SetStat: ch=%d code=0x%04x value=0x%x\n",
			  ch,code,value));

    switch(code) {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            llHdl->dbgLevel = value;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
			if( value != M_CH_INOUT )
				error = ERR_LL_ILL_DIR;
            break;
        /*--------------------------+
        |  WOT                      |
        +--------------------------*/
        case XC02_WOT:
			if( value > 0xffff ){
				error = ERR_LL_ILL_PARAM;
				break;
			}
			DBGWRT_2((DBH, " - XC02_WOT: WOT= %d min\n", value));

			SMB_W_BYTE( XC02C_WOT_L,      value & 0xff );
			SMB_W_BYTE( XC02C_WOT_H, (value>>8) & 0xff );
			break;

        /*--------------------------+
        |  SWOFF                    |
        +--------------------------*/
        case XC02_SWOFF:
			/* here we expect XC02C_SWOFF_MAGIC as value */
			DBGWRT_2((DBH, " - XC02_SWOFF: value = 0x%x min\n", value));
			SMB_W_BYTE( XC02C_SWOFF, (value & 0xff) );
			break;
        /*--------------------------+
        |  OFFACK                   |
        +--------------------------*/
        case XC02_OFFACK:
			DBGWRT_2((DBH, " - XC02_OFFACK\n"));
			SMB_W_BYTE( XC02C_OFFACK, XC02C_FLAG );
			break;
        /*--------------------------+
        |  DOWN_DELAY               |
        +--------------------------*/
        case XC02_DOWN_DELAY:
			if( (value < XC02C_DOWN_DELAY_MINMODE) ||
				(value > XC02C_DOWN_DELAY_MAXMODE) ){
				error = ERR_LL_ILL_PARAM;
				break;
			}
			DBGWRT_2((DBH, " - XC02_DOWN_DELAY: value = %d\n", value));
			SMB_W_BYTE( XC02C_DOWN_DELAY, value );
			break;
        /*--------------------------+
        |  OFF_DELAY                |
        +--------------------------*/
        case XC02_OFF_DELAY:
			if( (value < XC02C_OD_MINMODE) || (value > XC02C_OD_MAXMODE) ){
				error = ERR_LL_ILL_PARAM;
				break;
			}
			DBGWRT_2((DBH, " - XC02_OFF_DELAY: value = %d\n", value));
			SMB_W_BYTE( XC02C_OFF_DELAY, value );
			break;

        /*------------------------------------+
        |  install signal for shutdown event  |
        +------------------------------------*/
        case XC02_BLK_DOWN_SIG_SET:
		{
			M_SG_BLOCK			*blk = (M_SG_BLOCK*)valueP;
			XC02_BLK_DOWN_SIG	*alm = (XC02_BLK_DOWN_SIG*)blk->data;
			u_int32				realMsec;

			/* check buf size */
			if( blk->size < sizeof(XC02_BLK_DOWN_SIG) )
				return(ERR_LL_USERBUF);

			/* illegal signal code ? */
			if( alm->signal == 0 ){
				DBGWRT_ERR((DBH, " *** XC02_SetStat: illegal signal code=0x%x",
					alm->signal));
				return(ERR_LL_ILL_PARAM);
			}

			/* already defined ? */
			if( llHdl->sigHdl != NULL ){
				DBGWRT_ERR((DBH, " *** XC02_SetStat: signal already instaled"));
				return(ERR_OSS_SIG_SET);
			}
			DBGWRT_2((DBH, " - XC02_BLK_DOWN_SIG_SET\n"));
			/* install signal+alarm */
			if( (error = OSS_SigCreate(llHdl->osHdl,
									   alm->signal, &llHdl->sigHdl)) )
				return(error);

            if( (error = OSS_AlarmSet(llHdl->osHdl, llHdl->alarmHdl,
									  alm->msec, 1, &realMsec)) )
				return(error);
            break;
		}
        /*---------------------------------------+
        |  deinstall signal for shutdown event   |
        +---------------------------------------*/
        case XC02_DOWN_SIG_CLR:
			/* not defined ? */
			if( llHdl->sigHdl == NULL ){
				DBGWRT_ERR((DBH, " *** XC02_SetStat: signal not installed"));
				return(ERR_OSS_SIG_CLR);
			}
			DBGWRT_2((DBH, " - XC02_BLK_DOWN_SIG_CLR\n"));
  			/* remove signal+alarm */
			if( (error = OSS_SigRemove(llHdl->osHdl, &llHdl->sigHdl)) )
				return(error);

			if( (error = OSS_AlarmClear(llHdl->osHdl, llHdl->alarmHdl)) )
				return(error);

            break;

        /*--------------------------+
        |  TEMP_HIGH                |
        +--------------------------*/
        case XC02_TEMP_HIGH:
			if( (value < TEMP_RANGE_MIN) || (value > TEMP_RANGE_MAX )){
				error = ERR_LL_ILL_PARAM;
				break;
			}
			DBGWRT_2((DBH, " - XC02_TEMP_HIGH: %d degree (0x%02x)\n",
					  XC02_TEMP2ADC(value), value ));
			SMB_W_BYTE( XC02C_TEMP_HIGH, XC02_TEMP2ADC(value));
			break;


        /*--------------------------+
        |  TEMP_LOW                 |
        +--------------------------*/
        case XC02_TEMP_LOW:

			if ( (value < TEMP_RANGE_MIN ) || (value > TEMP_RANGE_MAX )) {
				error = ERR_LL_ILL_PARAM;
				break;
			}
			DBGWRT_2((DBH, " - XC02_TEMP_LOW: %d degree (0x%02x)\n",
					  XC02_TEMP2ADC(value), value ));
			/* value is in degree celsius */
			SMB_W_BYTE( XC02C_TEMP_LOW, XC02_TEMP2ADC(value) );
			break;

        /*--------------------------+
        |  brightness source        |
        +--------------------------*/
        case XC02_BR_SRC:
			if ( (value !=0) && (value !=1)) {
				error = ERR_LL_ILL_PARAM;
				break;
			}
			DBGWRT_2((DBH, " - XC02_BR_SRC: src = %d\n", value ));
			SMB_W_BYTE(	XC02C_BR_SRC, value );
			break;

        /*--------------------------+
        |  brightness multiplier    |
        +--------------------------*/
        case XC02_BR_MULT:
			/* allow factors 0..10, default = 1 */
			if ( (value < 0) || (value > 100)) {
				error = ERR_LL_ILL_PARAM;
				break;
			}
			DBGWRT_2((DBH, " - XC02_BR_MULT: %d\n", value ));
			SMB_W_BYTE(	XC02C_AUTO_BR_FAK, value );
			break;


        /*--------------------------+
        |  brightness offset        |
        +--------------------------*/
        case XC02_BR_OFFS:
			if ( (value < 0) || (value > 0xfe)) {
				error = ERR_LL_ILL_PARAM;
				break;
			}
			DBGWRT_2((DBH, " - XC02_BR_OFFS: %d\n", value ));
			SMB_W_BYTE(	XC02C_AUTO_BR_OFFS, value );
			break;

        /*--------------------------+
        | switch display            |
        +--------------------------*/
        case XC02_SW_DISP:
			if ( (value >=0 ) && (value <=3 )) {
				DBGWRT_2((DBH, " - XC02_SW_DISP: value = %d\n", value ));
				SMB_W_BYTE( XC02C_SW_DISP, value );
			} else {
				error = ERR_LL_ILL_PARAM;
			}
			break;

        /*--------------------------+
        | initial display status    |
        +--------------------------*/
        case XC02_DISP_INITSTAT:
			if ( (value >=0 ) && (value <=3 )) {
				DBGWRT_2((DBH, " - XC02_DISP_INITSTAT: value = %d\n", value ));
				SMB_W_BYTE( XC02C_INIT_DS, value );
			} else {
				error = ERR_LL_ILL_PARAM;
				break;
			}
			break;

        /*--------------------------+
        | brightness direction      |
        +--------------------------*/
        case XC02_BRIGHT_DIRECTION:
			if ( (value == 0 ) || (value == 1 )) {
				DBGWRT_2((DBH, " - XC02_BRIGHT_DIRECTION: value = %d\n", value ));
				SMB_W_BYTE( SC21C_BR_DIR, value );
			} else {
				error = ERR_LL_ILL_PARAM;
				break;
			}
			break;

        /*--------------------------+
        |  start WDOG               |
        +--------------------------*/
        case WDOG_START:
			DBGWRT_2((DBH, " - XC02_WDOG_START\n" ));
			SMB_W_BYTE( XC02C_WDOG_STATE, XC02C_WDOG_ON );
 			if( error )
				break;
			llHdl->wdState = 1;
           break;

        /*--------------------------+
        |  stop WDOG                |
        +--------------------------*/
        case WDOG_STOP:
			DBGWRT_2((DBH, " - XC02_WDOG_STOP\n" ));
			SMB_W_BYTE( XC02C_WDOG_STATE, XC02C_WDOG_OFF );
			if( error  )
				break;
			llHdl->wdState = 0;
            break;

        /*--------------------------+
        |  trigger WDOG             |
        +--------------------------*/
        case WDOG_TRIG:
			/* watchdog not enabled ? */
			if ( llHdl->wdState == 0 ) {
				error = ERR_LL_DEV_NOTRDY;
				break;
			}
			DBGWRT_2((DBH, " - XC02_WDOG_TRIG\n" ));
			SMB_W_BYTE( XC02C_WDOG_TRIG, XC02C_FLAG );
            break;
        /*--------------------------+
        |  WDOG time in ms          |
        +--------------------------*/
        case WDOG_TIME:
			/* check range */
			wdtime = value/100;
			if( (wdtime < XC02C_WDOG_TOUT_MINMODE) ||
				( wdtime > XC02C_WDOG_TOUT_MAXMODE) ){
					error = ERR_LL_ILL_PARAM;
					break;
			}
			DBGWRT_2((DBH, " - XC02_WDOG_TIME: %d ms \n", wdtime*100 ));
			SMB_W_BYTE( XC02C_WDOG_TOUT, wdtime );
            break;

        /*--------------------------+
        |  Test registers #1..2     |
		|  (for PIC firmware test)  |
        +--------------------------*/
	    case XC02_TEST1:
			 DBGWRT_2((DBH, " - XC02_TEST1\n"));
			 SMB_W_BYTE( XC02C_TEST1, value );
			break;

        case XC02_TEST2:
			DBGWRT_1((DBH, " - XC02_TEST2\n"));
			SMB_W_BYTE( XC02C_TEST2, value );
			break;

        /*--------------------------+
        |  prim. display brightness |
        +--------------------------*/
	    case XC02_BRIGHTNESS:
			DBGWRT_1((DBH, " - XC02_BRIGHTNESS\n"));
			SMB_W_BYTE( XC02C_SET_BR, value & 0xff );
			break;

		/*--------------------------+
        | initial display brightness|
        +--------------------------*/
	    case XC02_INIT_BRIGHT:
			/* set initial brightness for display 0 or 1,
			 *   current channel selects display */
			switch (ch) {
			case 0:
				SMB_W_BYTE( XC02C_INIT_BR1, value & 0xff );
				break;
			case 1:
				SMB_W_BYTE( XC02C_INIT_BR2, value & 0xff );
				break;
			default:
				DBGWRT_ERR((DBH," *** XC02_INIT_BRIGHT: illegal chan. %d\n",
							ch));
				error = ERR_LL_ILL_PARAM;
			}
			break;

		/* next 2 are effective on DC1 Rev 01 only */
        /*--------------------------+
        | brightness 2nd display    |
        +--------------------------*/
	    case XC02_BRIGHTNESS2:
			DBGWRT_1((DBH, " - XC02_BRIGHTNESS2\n"));
			SMB_W_BYTE( XC02C_SET_BR_2, value & 0xff );
			break;

        /*--------------------------+
        |  minicard slot power      |
        +--------------------------*/
	    case XC02_MINICARD_PWR:
			DBGWRT_1((DBH, " - XC02_MINICARD_PWR\n"));
			SMB_W_BYTE( XC02C_SW_MINICARD, value & 0xff );
			break;

        /*--------------------------+
        |  KEY_IN behavior control  |
        +--------------------------*/
	    case XC02_KEY_IN_CTRL:
			DBGWRT_1((DBH, " - XC02_KEY_IN_CTRL\n"));
			SMB_W_BYTE( SC21C_KEY_CTRL, value & 0x01 );
			break;

        /*--------------------------+
        |  auto brightness behavior |
        +--------------------------*/
	    case XC02_AUTO_BRIGHT_CTRL:
			DBGWRT_1((DBH, " - XC02_AUTO_BRIGHT_CTRL\n"));
			SMB_W_BYTE( SC21C_BR_DIR, value & 0x01 );
			break;


		/*--------------------------+
        |  unknown                  |
        +--------------------------*/
        default:
			error = ERR_LL_UNK_CODE;
    }

	return(error);
}

/****************************** XC02_GetStat *********************************/
/** Get the driver status
 *
 *  The driver supports \ref getstat_setstat_codes "these status codes"
 *  in addition to the standard codes (see mdis_api.h).
 *
 *  \param llHdl      		\IN  Low-level handle
 *  \param code       		\IN  \ref getstat_setstat_codes "status code"
 *  \param ch         		\IN  Current channel
 *  \param value32_or_64P	\IN  Pointer to block data structure (M_SG_BLOCK) for
 *                         			block status codes
 *  \param value32_or_64P	\OUT Data pointer or pointer to block data structure
 *                         			(M_SG_BLOCK) for block status codes
 *
 *  \return           \c 0 On success or error code
 */
static int32 XC02_GetStat(
    LL_HANDLE *llHdl,
    int32  code,
    int32  ch,
    INT32_OR_64 *value32_or_64P
)
{
	int32	error = ERR_SUCCESS;
	u_int8	regVal;

	int32		*valueP	  = (int32*)value32_or_64P;		/* pointer to 32bit value  */
	INT32_OR_64	*value64P = value32_or_64P;		 		/* stores 32/64bit pointer  */

    DBGWRT_1((DBH, "LL - XC02_GetStat: ch=%d code=0x%04x\n",
			  ch,code));

    switch(code)
    {
        /*--------------------------+
        |  debug level              |
        +--------------------------*/
        case M_LL_DEBUG_LEVEL:
            *valueP = llHdl->dbgLevel;
            break;
        /*--------------------------+
        |  number of channels       |
        +--------------------------*/
        case M_LL_CH_NUMBER:
            *valueP = CH_NUMBER;
            break;
        /*--------------------------+
        |  channel direction        |
        +--------------------------*/
        case M_LL_CH_DIR:
            *valueP = M_CH_INOUT;
            break;
        /*--------------------------+
        |  channel length [bits]    |
        +--------------------------*/
        case M_LL_CH_LEN:
            *valueP = 8;
            break;
        /*--------------------------+
        |  channel type info        |
        +--------------------------*/
        case M_LL_CH_TYP:
            *valueP = M_CH_UNKNOWN;
            break;
        /*--------------------------+
        |  id check enabled         |
        +--------------------------*/
        case M_LL_ID_CHECK:
            *valueP = llHdl->idCheck;
            break;
        /*--------------------------+
        |   ident table pointer     |
        |   (treat as non-block!)   |
        +--------------------------*/
        case M_MK_BLK_REV_ID:
           *value64P = (INT32_OR_64)&llHdl->idFuncTbl;
           break;
        /*--------------------------+
        |  WOT                      |
        +--------------------------*/
        case XC02_WOT:
		{
			u_int8 lVal, hVal;

			SMB_R_BYTE( XC02C_WOT_L, &lVal );
			if( error  )
				break;

			SMB_R_BYTE( XC02C_WOT_H, &hVal );
			if( error  )
				break;

			*valueP = (int32)(hVal<<8 | lVal);
			DBGWRT_2((DBH, " - XC02_WOT:= %d min\n", *valueP ));
			break;
		}

        /*--------------------------+
        |  WDOG_ERR                 |
        +--------------------------*/
        case XC02_WDOG_ERR:
			SMB_R_BYTE( XC02C_WDOG_ERR, valueP );
			DBGWRT_2((DBH, " - XC02_WDOG_ERR:= %d\n", *valueP ));
			break;
        /*--------------------------+
        |  DOWN_DELAY               |
        +--------------------------*/
        case XC02_DOWN_DELAY:
			SMB_R_BYTE( XC02C_DOWN_DELAY, valueP );
			DBGWRT_2((DBH, " - XC02_DOWN_DELAY:= %d\n", *valueP ));
			break;
        /*--------------------------+
        |  OFF_DELAY                |
        +--------------------------*/
        case XC02_OFF_DELAY:
			SMB_R_BYTE( XC02C_OFF_DELAY, valueP );
			DBGWRT_2((DBH, " - XC02_OFF_DELAY:= %d\n", *valueP ));
			break;
        /*--------------------------+
        |  DOWN_EVT                 |
        +--------------------------*/
        case XC02_DOWN_EVT:
			SMB_R_BYTE( XC02C_STATUS, &regVal );
			*valueP = (int32)(regVal & 0x3);
			DBGWRT_2((DBH, " - XC02_STATUS:= %d\n", *valueP ));
			break;
        /*--------------------------+
        |  IN                       |
        +--------------------------*/
        case XC02_IN:
			SMB_R_BYTE( XC02C_IN, valueP );
			DBGWRT_2((DBH, " - XC02_IN:= %d\n", *valueP ));
			break;

        /*--------------------------+
        | timestamp  (char wise)    |
        +--------------------------*/
	    case XC02_TIMESTAMP:
			SMB_R_BYTE( XC02C_TIMESTAMP, valueP );
			DBGWRT_2((DBH, " - XC02_TIMESTAMP: value = %x ('%c')",
					  *valueP, *valueP));
			break;

        /*--------------------------+
        |  TEMP                     |
        +--------------------------*/
	    case XC02_TEMP:
			SMB_R_BYTE( XC02C_TEMP, &regVal );
			*valueP = (int32)regVal;
			DBGWRT_2((DBH, " - XC02_TEMP:   value = %x ", *valueP));
			break;

        /*--------------------------+
        |  TEMP_HIGH                |
        +--------------------------*/
        case XC02_TEMP_HIGH:
			SMB_R_BYTE( XC02C_TEMP_HIGH, &regVal );
			*valueP = (int32)XC02_ADC2TEMP(regVal);
			DBGWRT_2((DBH, " - XC02_TEMP_HIGH:= %d\n", *valueP ));
			break;

        /*--------------------------+
        |  TEMP_LOW                 |
        +--------------------------*/
	    case XC02_TEMP_LOW:
			SMB_R_BYTE( XC02C_TEMP_LOW, &regVal );
			*valueP = (int32)( XC02_ADC2TEMP(regVal) );
			DBGWRT_2((DBH, " - XC02_TEMP_LOW:= %d\n", *valueP ));
			break;


        /*--------------------------+
        |  VOLTAGE                  |
        +--------------------------*/
        case XC02_VOLTAGE:
			SMB_R_BYTE( XC02C_VOLT, &regVal );
			*valueP = (int32)regVal;
			DBGWRT_3((DBH, " - XC02_VOLTAGE: value = %x ", *valueP));
			break;


        /*--------------------------+
        |  VOLT_HIGH                |
        +--------------------------*/
        case XC02_VOLT_HIGH:
			SMB_R_BYTE( XC02C_VOLT_HIGH, &regVal );
			*valueP = (int32)XC02_ADC2TEMP(regVal);
			DBGWRT_2((DBH, " - XC02_TEMP_HIGH:= %d\n", *valueP ));
			break;


        /*--------------------------+
        |  VOLT_LOW                 |
        +--------------------------*/
	    case XC02_VOLT_LOW:
			SMB_R_BYTE( XC02C_VOLT_LOW, &regVal );
			*valueP = (int32)( XC02_ADC2TEMP(regVal) );
			DBGWRT_2((DBH, " - XC02_TEMP_LOW:= %d\n", *valueP ));
			break;


        /*--------------------------+
        |  XC02_DISP_INITSTAT       |
        +--------------------------*/
        case XC02_DISP_INITSTAT:    /* 1: ON, BIOS etc. shown  0: OFF */
			SMB_R_BYTE( XC02C_INIT_DS, &regVal );
		    *valueP = (int32)(regVal);
			DBGWRT_2((DBH, " - XC02_DISP_INITSTAT:= %d\n", *valueP ));
			break;

		/*--------------------------+
        | initial display brightness|
        +--------------------------*/
	    case XC02_INIT_BRIGHT:
			switch (ch) {
			case 0:
				SMB_R_BYTE( XC02C_INIT_BR1, &regVal );
				*valueP = ((int32)(regVal)) & 0xff;
				break;
			case 1:
				SMB_R_BYTE( XC02C_INIT_BR2, &regVal );
				*valueP = ((int32)(regVal)) & 0xff;
				break;
			default:
				DBGWRT_ERR((DBH," *** XC02_INIT_BRIGHT: illegal chan. %d\n",
							ch));
				error = ERR_LL_ILL_PARAM;
			}
			break;

        /*--------------------------+
        |  brightness source        |
        +--------------------------*/
        case XC02_BR_SRC:
			SMB_R_BYTE( XC02C_BR_SRC, &regVal );
			*valueP = (int32)(regVal);
			DBGWRT_2((DBH, " - XC02_BR_SRC: %d\n", *valueP ));
			break;

        /*--------------------------+
        |  brightness multiplier    |
        +--------------------------*/
        case XC02_BR_MULT:
			SMB_R_BYTE( XC02C_AUTO_BR_FAK, &regVal );
			*valueP = (int32)(regVal);
			DBGWRT_2((DBH, " - XC02_BR_MULT: %d\n", *valueP ));
			break;

        /*--------------------------+
        |  brightness offset        |
        +--------------------------*/
	    case XC02_BR_OFFS:
			SMB_R_BYTE( XC02C_AUTO_BR_OFFS, &regVal );
			*valueP = (int32)(regVal);
			DBGWRT_2((DBH, " - XC02_BR_OFFS: %d\n", *valueP ));
			break;

        /*--------------------------+
        |  switch display           |
        +--------------------------*/
        case XC02_SW_DISP:
			SMB_R_BYTE( XC02C_SW_DISP, &regVal );
			*valueP = (int32)(regVal);
			DBGWRT_2((DBH, " - XC02_SW_DISP:= %d\n", *valueP ));
			break;

        /*--------------------------+
        |  WDOG time in ms          |
        +--------------------------*/
        case WDOG_TIME:
			SMB_R_BYTE( XC02C_WDOG_TOUT, &regVal );
			*valueP = (int32)(regVal * 100);
			DBGWRT_2((DBH, " - WDOG_TIME:= %d\n", *valueP ));
            break;

        /*--------------------------+
        |  WDOG status              |
        +--------------------------*/
        case WDOG_STATUS:
			SMB_R_BYTE( XC02C_WDOG_STATE, valueP );
			DBGWRT_2((DBH, " - WDOG_STATUS:= %d\n", *valueP ));
            break;

        /*--------------------------+
        |  WDOG shot                |
        +--------------------------*/
        case WDOG_SHOT:
			SMB_R_BYTE( XC02C_WDOG_ERR, &regVal );
			*valueP = (int32)(regVal ? 1 : 0);
			DBGWRT_2((DBH, " - WDOG_SHOT:= %d\n", *valueP ));
            break;

        /*--------------------------+
        | brightness 1st display    |
        +--------------------------*/
	    case XC02_BRIGHTNESS:
			SMB_R_BYTE( XC02C_SET_BR, &regVal );
			*valueP = (int32)(regVal & 0xff);
			DBGWRT_2((DBH, " - XC02_BRIGHTNESS:= %d\n", *valueP ));
			break;

        /*--------------------------+
        | brightness 2nd display    |
        +--------------------------*/
	    case XC02_BRIGHTNESS2:
			SMB_R_BYTE( XC02C_SET_BR_2, &regVal );
			*valueP = (int32)(regVal & 0xff);
			DBGWRT_2((DBH, " - XC02_BRIGHTNESS2:= %d\n", *valueP ));
			break;

        /*--------------------------+
        |  minicard slot power      |
        +--------------------------*/
	    case XC02_MINICARD_PWR:
			SMB_R_BYTE( XC02C_SW_MINICARD, &regVal );
			*valueP = (int32)(regVal & 0xff);
			DBGWRT_2((DBH, " - XC02_MINICARD_PWR:= %d\n", *valueP ));
			break;

        /*--------------------------+
        |  KEY_IN behavior control  |
        +--------------------------*/
	    case XC02_KEY_IN_CTRL:
			SMB_R_BYTE( SC21C_KEY_CTRL, &regVal );
			*valueP = (int32)(regVal & 0x01);
			DBGWRT_2((DBH, " - XC02_KEY_IN_CTRL:= %d\n", *valueP ));
			break;

        /*--------------------------+
        |  read 12V drawn current   |
        +--------------------------*/
	    case SC21_BL_CURRENT:
			SMB_R_BYTE( SC21C_BL_CURR, &regVal );
			*valueP = (int32)(regVal &0xff);
			DBGWRT_2((DBH, " - SC21_BL_CURRENT:= %d\n", *valueP ));
			break;

        /*--------------------------+
        |  auto brightness behavior |
        +--------------------------*/
	    case XC02_AUTO_BRIGHT_CTRL:
			SMB_R_BYTE( SC21C_BR_DIR, &regVal );
			*valueP = (int32)(regVal & 0x01);
			DBGWRT_2((DBH, " - XC02_AUTO_BRIGHT_CTRL:= %d\n", *valueP ));
			break;

        /*--------------------------+
        | raw photo ADC value       |
        +--------------------------*/
	    case XC02_RAW_BRIGHTNESS:
			SMB_R_BYTE( SC21C_BR_RAW, &regVal );
			*valueP = (int32)(regVal & 0xff);
			DBGWRT_2((DBH, " - XC02_RAW_BRIGHTNESS:= %d\n", *valueP ));
			break;

        /*--------------------------+
        |  Test registers #1..3     |
		|  (for PIC firmware test)  |
        +--------------------------*/
        case XC02_TEST1:
			SMB_R_BYTE( XC02C_TEST1, valueP );
            break;
        case XC02_TEST2:
			SMB_R_BYTE( XC02C_TEST2, valueP );
            break;
        case XC02_TEST3:
			SMB_R_BYTE( XC02C_TEST3, valueP );
            break;
        case XC02_TEST4:
			SMB_R_BYTE( XC02C_TEST4, valueP );
            break;

		/*--------------------------+
        |  unknown                  |
        +--------------------------*/
        default:
			error = ERR_LL_UNK_CODE;
    }

	return(error);
}


/******************************* XC02_BlockRead ******************************/
/** Read a data block from the device
 *
 *  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
 *
 *  \param llHdl       \IN  Low-level handle
 *  \param ch          \IN  Current channel
 *  \param buf         \IN  Data buffer
 *  \param size        \IN  Data buffer size
 *  \param nbrRdBytesP \OUT Number of read bytes
 *
 *  \return            \c 0 On success or error code
 */
static int32 XC02_BlockRead(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrRdBytesP
)
{
    DBGWRT_1((DBH, "LL - XC02_BlockRead: ch=%d, size=%d\n",ch,size));
	return(ERR_LL_ILL_FUNC);
}

/****************************** XC02_BlockWrite ******************************/
/** Write a data block from the device
 *
 *  The function is not supported and always returns an ERR_LL_ILL_FUNC error.
 *
 *  \param llHdl  	   \IN  Low-level handle
 *  \param ch          \IN  Current channel
 *  \param buf         \IN  Data buffer
 *  \param size        \IN  Data buffer size
 *  \param nbrWrBytesP \OUT Number of written bytes
 *
 *  \return            \c ERR_LL_ILL_FUNC
 */
static int32 XC02_BlockWrite(
     LL_HANDLE *llHdl,
     int32     ch,
     void      *buf,
     int32     size,
     int32     *nbrWrBytesP
)
{
    DBGWRT_1((DBH, "LL - XC02_BlockWrite: ch=%d, size=%d\n",ch,size));

	/* return number of written bytes */
	*nbrWrBytesP = 0;

	return(ERR_LL_ILL_FUNC);
}


/****************************** XC02_Irq ************************************/
/** Interrupt service routine - unused
 *
 *  If the driver can detect the interrupt's cause it returns
 *  LL_IRQ_DEVICE or LL_IRQ_DEV_NOT, otherwise LL_IRQ_UNKNOWN.
 *
 *  \param llHdl  	   \IN  Low-level handle
 *  \return LL_IRQ_DEVICE	IRQ caused by device
 *          LL_IRQ_DEV_NOT  IRQ not caused by device
 *          LL_IRQ_UNKNOWN  Unknown
 */
static int32 XC02_Irq(
   LL_HANDLE *llHdl
)
{
	return(LL_IRQ_DEV_NOT);
}

/****************************** XC02_Info ***********************************/
/** Get information about hardware and driver requirements
 *
 *  The following info codes are supported:
 *
 * \code
 *  Code                      Description
 *  ------------------------  -----------------------------
 *  LL_INFO_HW_CHARACTER      Hardware characteristics
 *  LL_INFO_ADDRSPACE_COUNT   Number of required address spaces
 *  LL_INFO_ADDRSPACE         Address space information
 *  LL_INFO_IRQ               Interrupt required
 *  LL_INFO_LOCKMODE          Process lock mode required
 * \endcode
 *
 *  The LL_INFO_HW_CHARACTER code returns all address and
 *  data modes (ORed) which are supported by the hardware
 *  (MDIS_MAxx, MDIS_MDxx).
 *
 *  The LL_INFO_ADDRSPACE_COUNT code returns the number
 *  of address spaces used by the driver.
 *
 *  The LL_INFO_ADDRSPACE code returns information about one
 *  specific address space (MDIS_MAxx, MDIS_MDxx). The returned
 *  data mode represents the widest hardware access used by
 *  the driver.
 *
 *  The LL_INFO_IRQ code returns whether the driver supports an
 *  interrupt routine (TRUE or FALSE).
 *
 *  The LL_INFO_LOCKMODE code returns which process locking
 *  mode the driver needs (LL_LOCK_xxx).
 *
 *  \param infoType	   \IN  Info code
 *  \param ...         \IN  Argument(s)
 *
 *  \return            \c 0 On success or error code
 */
static int32 XC02_Info(
   int32  infoType,
   ...
)
{
    int32   error = ERR_SUCCESS;
    va_list argptr;

    va_start(argptr, infoType );

    switch(infoType) {
		/*-------------------------------+
        |  hardware characteristics      |
        |  (all addr/data modes ORed)   |
        +-------------------------------*/
        case LL_INFO_HW_CHARACTER:
		{
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);

			*addrModeP = MDIS_MA08;
			*dataModeP = MDIS_MD08 | MDIS_MD16;
			break;
	    }
		/*-------------------------------+
        |  nr of required address spaces |
        |  (total spaces used)           |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE_COUNT:
		{
			u_int32 *nbrOfAddrSpaceP = va_arg(argptr, u_int32*);

			*nbrOfAddrSpaceP = ADDRSPACE_COUNT;
			break;
	    }
		/*-------------------------------+
        |  address space type            |
        |  (widest used data mode)       |
        +-------------------------------*/
        case LL_INFO_ADDRSPACE:
		{
			u_int32 addrSpaceIndex = va_arg(argptr, u_int32);
			u_int32 *addrModeP = va_arg(argptr, u_int32*);
			u_int32 *dataModeP = va_arg(argptr, u_int32*);
			u_int32 *addrSizeP = va_arg(argptr, u_int32*);

			if((int32)addrSpaceIndex >= ADDRSPACE_COUNT)
				error = ERR_LL_ILL_PARAM;
			else {
				*addrModeP = MDIS_MA08;
				*dataModeP = MDIS_MD16;
				*addrSizeP = ADDRSPACE_SIZE;
			}

			break;
	    }
		/*-------------------------------+
        |   interrupt required           |
        +-------------------------------*/
        case LL_INFO_IRQ:
		{
			u_int32 *useIrqP = va_arg(argptr, u_int32*);

			*useIrqP = USE_IRQ;
			break;
	    }
		/*-------------------------------+
        |   process lock mode            |
        +-------------------------------*/
        case LL_INFO_LOCKMODE:
		{
			u_int32 *lockModeP = va_arg(argptr, u_int32*);

			*lockModeP = LL_LOCK_CALL;
			break;
	    }
		/*-------------------------------+
        |   (unknown)                    |
        +-------------------------------*/
        default:
          error = ERR_LL_ILL_PARAM;
    }

    va_end(argptr);
    return(error);
}

/*******************************  Ident  ***********************************/
/** Return ident string
 *
 *  \return            Pointer to ident string
 */
static char* Ident( void )
{
    return( (char*) IdentString );
}

/********************************* Cleanup *********************************/
/** Close all handles, free memory and return error code
 *
 *	\warning The low-level handle is invalid after this function is called.
 *
 *  \param llHdl      \IN  Low-level handle
 *  \param retCode    \IN  Return value
 *
 *  \return           \IN   retCode
 */
static int32 Cleanup(
   LL_HANDLE    *llHdl,
   int32        retCode
)
{
    /*------------------------------+
    |  close handles                |
    +------------------------------*/
	/* clean up desc */
	if(llHdl->descHdl)
		DESC_Exit(&llHdl->descHdl);

	/* clean up alarm */
	if (llHdl->alarmHdl)
		OSS_AlarmRemove(llHdl->osHdl, &llHdl->alarmHdl);

	/* clean up signal */
	if (llHdl->sigHdl)
		OSS_SigRemove(llHdl->osHdl, &llHdl->sigHdl);

	/* clean up debug */
	DBGEXIT((&DBH));

    /*------------------------------+
    |  free memory                  |
    +------------------------------*/
    /* free my handle */
    OSS_MemFree(llHdl->osHdl, (int8*)llHdl, llHdl->memAlloc);

    /*------------------------------+
    |  return error code            |
    +------------------------------*/
	return(retCode);
}

/******************************* AlarmHandler *******************************
 *
 *  Description: Handler for alarm
 *
 *---------------------------------------------------------------------------
 *  Input......: arg		ll handle
 *  Output.....: -
 *  Globals....: -
 ****************************************************************************/
static void AlarmHandler(void *arg)
{
	LL_HANDLE	*llHdl = (LL_HANDLE*)arg;
	int32		error;
	u_int8		status;

	DBGWRT_1((DBH,">>> LL - XC02 AlarmHandler:\n"));

	SMB_R_BYTE( XC02C_STATUS, &status );

	if( status & XC02C_STATUS_DOWN_EVT ){
		OSS_SigSend( llHdl->osHdl, llHdl->sigHdl );
		DBGWRT_3((DBH, " shutdown event --> send signal\n"));
	}
}


