#
# AA241x fixed wing control application
#

MODULE_COMMAND	= aa241x_fw_control

SRCS		= aa241x_fw_control_main.cpp \
		  aa241x_fw_control_params.c \
		  aa241x_fw_control.cpp \
		  aa241x_fw_aux.cpp \
		  ../aa241x_mission/aa241x_mission_main.cpp \
		  ../aa241x_mission/aa241x_mission_params.c

MODULE_STACKSIZE = 1200

MAXOPTIMIZATION = -Os