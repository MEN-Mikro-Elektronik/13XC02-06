#***************************  M a k e f i l e  *******************************
#
#         Author: ts
#          $Date: 2008/12/09 12:45:44 $
#      $Revision: 1.3 $
#
#    Description: Makefile definitions for the XC02BC driver
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2005-2019, MEN Mikro Elektronik GmbH
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

