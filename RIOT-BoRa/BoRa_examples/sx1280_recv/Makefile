BOARD ?= nucleo-l073rz
APPLICATION = sx1280_recv

# required modules
USEMODULE += sx1280
USEMODULE += shell_cmds_default

USEMODULE += iolist
USEMODULE += xtimer
RIOTBASE ?= $(CURDIR)/../..
include $(RIOTBASE)/Makefile.include
QUIET ?= 1
# DEVELHELP enabled by default for all tests, set 0 to disable
DEVELHELP ?= 1