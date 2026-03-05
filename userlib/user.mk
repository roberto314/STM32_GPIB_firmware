USERLIB = ./userlib
# List of all the Userlib files
USERSRC =  $(USERLIB)/src/comm.c \
           $(USERLIB)/src/gpib.c \
           $(USERLIB)/src/usbcfg.c\
                     
# Required include directories
USERINC =  $(USERLIB) \
           $(USERLIB)/include 
