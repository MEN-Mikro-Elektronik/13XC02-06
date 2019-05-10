/************************** MDIS4 device descriptor *************************/
/*!
 *        \file  xc02bc_min.dsc
 *
 *      \author  thomas.schnuerer@men.de
 *        $Date: 2008/12/09 12:58:59 $
 *    $Revision: 1.2 $
 *
 *      \brief   Minimum descriptor necessary for XC02 driver 
 */
 /*-------------------------------[ History ]--------------------------------
 *
 * $Log: xc02bc_min.dsc,v $
 * Revision 1.2  2008/12/09 12:58:59  ts
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
    BOARD_NAME       = STRING   smb2bb      # device name of baseboard
    DEVICE_SLOT      = U_INT32  0           # used slot on baseboard (0..n)

	#------------------------------------------------------------------------
	#	device parameters
	#------------------------------------------------------------------------

    # SMBus bus number
    SMB_BUSNBR = U_INT32 0

    # SMBus address of XC02 PIC
    # For Linux, SMBus adresses are 7bit aligned. 0x12 >> 1 = 0x09
    SMB_DEVADDR = U_INT32 0x09

}

