#***************************  M a k e f i l e  *******************************
#
#         Author: ts
#          $Date: 2008/11/24 12:11:39 $
#      $Revision: 1.1 $
#
#    Description: Makefile definitions for the XC02_MON example program
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.1  2008/11/24 12:11:39  ts
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2005 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=xc02_simp

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX) \
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/xc02_drv.h \
         $(MEN_INC_DIR)/men_typs.h \
         $(MEN_INC_DIR)/mdis_api.h \
         $(MEN_INC_DIR)/usr_oss.h \

MAK_INP1=xc02_simp$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)
