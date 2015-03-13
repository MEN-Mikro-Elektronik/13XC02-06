#***************************  M a k e f i l e  *******************************
#
#         Author: ts
#          $Date: 2008/12/09 12:45:44 $
#      $Revision: 1.3 $
#
#    Description: Makefile definitions for the XC02BC driver
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: driver.mak,v $
#   Revision 1.3  2008/12/09 12:45:44  ts
#   R: Cosmetics, fixed revision texts and description
#
#   Revision 1.2  2008/11/25 10:52:37  ts
#   R: warning during MDVE run
#   M: added include of xc02.h and smb2.h
#
#   Revision 1.1  2008/09/04 11:43:11  ts
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=xc02

MAK_SWITCH=$(SW_PREFIX)MAC_IO_MAPPED

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/desc$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/oss$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/dbg$(LIB_SUFFIX)	\

MAK_INCL=$(MEN_INC_DIR)/xc02_drv.h	\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/oss.h		\
         $(MEN_INC_DIR)/smb2.h		\
		 $(MEN_INC_DIR)/xc02.h		\
         $(MEN_INC_DIR)/mdis_err.h	\
         $(MEN_INC_DIR)/maccess.h	\
         $(MEN_INC_DIR)/desc.h		\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/mdis_com.h	\
         $(MEN_INC_DIR)/ll_defs.h	\
         $(MEN_INC_DIR)/ll_entry.h	\
         $(MEN_INC_DIR)/dbg.h		\
         $(MEN_INC_DIR)/wdog.h		\

MAK_INP1=xc02_drv$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

