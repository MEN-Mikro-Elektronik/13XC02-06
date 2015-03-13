#***************************  M a k e f i l e  *******************************
#
#         Author: dieter.pfeuffer@men.de/ts
#          $Date: 2008/12/09 11:38:22 $
#      $Revision: 1.3 $
#
#    Description: Makefile definitions for XC02_CTRL tool
#
#---------------------------------[ History ]---------------------------------
#
#   $Log: program.mak,v $
#   Revision 1.3  2008/12/09 11:38:22  ts
#   R: file missed in correct place in repository
#
#   Revision 1.2  2008/11/24 12:10:16  ts
#   R: ad78c.h was still included
#   M: changed to include xc02c.h
#
#   Revision 1.1  2008/09/04 11:47:03  ts
#   Initial Revision
#
#
#-----------------------------------------------------------------------------
#   (c) Copyright 2006 by MEN mikro elektronik GmbH, Nuernberg, Germany
#*****************************************************************************

MAK_NAME=xc02_ctrl

MAK_LIBS=$(LIB_PREFIX)$(MEN_LIB_DIR)/mdis_api$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_oss$(LIB_SUFFIX)	\
         $(LIB_PREFIX)$(MEN_LIB_DIR)/usr_utl$(LIB_SUFFIX)

MAK_INCL=$(MEN_INC_DIR)/xc02_drv.h	\
         $(MEN_INC_DIR)/xc02.h		\
         $(MEN_INC_DIR)/men_typs.h	\
         $(MEN_INC_DIR)/mdis_api.h	\
         $(MEN_INC_DIR)/usr_oss.h	\
         $(MEN_INC_DIR)/usr_utl.h

MAK_INP1=xc02_ctrl$(INP_SUFFIX)

MAK_INP=$(MAK_INP1)

