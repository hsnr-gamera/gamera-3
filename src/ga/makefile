# -*- Mode: makefile -*-
# Copyright (c) 1994-1996 Massachusetts Institute of Technology
# Copyright (c) 1996-1999 Matthew Wall
# -----------------------------------------------------------------------------

include ../makevars
include makefile.sources
include ../../../gamera/config.mk

INC_DIRS= -I..

$(LIB): $(OBJS) 
	$(PRELINK)
	$(AR) $(LIB) $?
	@echo $(LIB) is now up-to-date
.C.a:;

clean:
	$(RM) $(LIB)
	$(RM) *.o *~ *.bak core ii_files ptrepository
	$(RM) vc60.* ga.lib

install:: $(LIB)
	$(INSTALL) $(LIB) $(LIB_DEST_DIR)
	$(RM) $(HDR_DEST_DIR)/ga; $(MKDIR) $(HDR_DEST_DIR)/ga
	$(CP) $(HDRS) $(TMPL_SRCS) ../VERSION $(HDR_DEST_DIR)/ga

remove:
	$(RM) $(LIB_DEST_DIR)/$(LIB) $(HDR_DEST_DIR)/ga

depend:
	$(MKDEPEND) -I$(CXX_INC_DIR) $(INC_DIRS) $(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.
