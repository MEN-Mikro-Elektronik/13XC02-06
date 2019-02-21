#***************************  M a k e f i l e  *******************************
#
#         Author: ts
#          $Date: 2008/09/04 11:48:12 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for XC02_TEST tool
#
#-----------------------------------------------------------------------------
#   Copyright (c) 2006-2019, MEN Mikro Elektronik GmbH
#*****************************************************************************

MAK_NAME=xc02_test

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/xc02_drv.h	\
         $(MEN_INC_DIR)/xc02.h		\
         $(MEN_INC_DIR)/wdog.h		\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/usr_oss.h	\
         $(MEN_INC_DIR)/usr_utl.h

MAK_INP1=xc02_test$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
