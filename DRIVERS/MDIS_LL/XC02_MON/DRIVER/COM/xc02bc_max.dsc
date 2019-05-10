/************************** MDIS4 device descriptor *************************/
/*!
 *        \file  xc02bc_max.dsc
 *
 *      \author  thomas.schnuerer@men.de
 *        $Date: 2008/12/09 12:58:50 $
 *    $Revision: 1.2 $
 *
 *      \brief   Maximum descriptor necessary for XC02 driver 
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: xc02bc_max.dsc,v $
 * Revision 1.2  2008/12/09 12:58:50  ts
 * R: final checkin for release
 *
 *---------------------------------------------------------------------------
 * (c) Copyright 2008 by MEN Mikro Elektronik GmbH, Nuremberg, Germany
 ****************************************************************************/

XC02_1  {
	#------------------------------------------------------------------------
	#	general parameters (don't modify)
	#------------------------------------------------------------------------
    DESC_TYPE        = U_INT32  1           # descriptor type (1=device)
    HW_TYPE          = STRING   XC02        # hardware name of device

	#------------------------------------------------------------------------
	#	reference to base board
	#------------------------------------------------------------------------
    BOARD_NAME       = STRING   SMB2BB      # device name of baseboard
    DEVICE_SLOT      = U_INT32  0           # used slot on baseboard (0..n)


	#------------------------------------------------------------------------
	#	device parameters 
	#------------------------------------------------------------------------

    # Define wether PIC firmware ID is checked
    # 0 := disable -- ignore ID
    # 1 := enable
    ID_CHECK = U_INT32 1

    # SMBus bus number
    SMB_BUSNBR = U_INT32 0

    # SMBus address of XC02 PIC
    # For Linux, SMBus adresses are 7bit aligned ( 0x12>>1 = 0x09).
    SMB_DEVADDR = U_INT32 0x09

    # Watchdog timeout in 100ms units
    # Range: 1..255
    WDOG_TOUT = U_INT32 255

    # Shutdown delay mode
    # 0 := 0 min
    # 1 := 1 min
    # 2 := 2 min
    # 3 := 4 min
    # 4 := 8 min
    # 5 := 16 min
    # 6 := 32 min
    # 7 := 64 min
    DOWN_DELAY = U_INT32 0

    # Off delay mode
    # 0 := 0 min
    # 1 := 1 min
    # 2 := 2 min
    # 3 := 4 min
    # 4 := 8 min
    # 5 := 16 min
    OFF_DELAY = U_INT32 0

	# brightness control source
	# 0 := controlled via SMBus command
	# 1 := autonomous controlled via photo diode
	BRIGHT_SOURCE = U_INT32 0

	# maximum allowed temperature before display switch off
	# direct raw ADRESH value, default to maximum for production
	TEMP_HIGH = U_INT32 0xff

	# minimum allowed temperature before display switch off
	# direct raw ADRESH value, default to minimum for production
	TEMP_LOW = U_INT32 0

	# maximum allowed DCDC converter voltage before display switch off
	# direct raw ADRESH value, default to maximum for production
	VOLT_HIGH = U_INT32 0xff

	# minimum allowed DCDC converter voltage before display switch off
	# direct raw ADRESH value, default to minimum for production
	VOLT_LOW = U_INT32 0x00

	# Initial display status after powerup
	# 0 := display initially off (in field/public)
	# 1 := display initially on  (for development/debugging etc)
	INIT_DISPSTAT = U_INT32 1

	#------------------------------------------------------------------------
	#	debug levels (optional)
	#   this keys have only effect on debug drivers
	#------------------------------------------------------------------------
    DEBUG_LEVEL      = U_INT32  0xc0008000  # LL driver
    DEBUG_LEVEL_MK   = U_INT32  0xc0008000  # MDIS kernel
    DEBUG_LEVEL_OSS  = U_INT32  0xc0008000  # OSS calls
    DEBUG_LEVEL_DESC = U_INT32  0xc0008000  # DESC calls
    DEBUG_LEVEL_MBUF = U_INT32  0xc0008000  # MBUF calls
}

