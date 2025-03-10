# name of your application
APPLICATION = BoRa_decoder_test

# If no BOARD is found in the environment, use this default:
# BOARD ?= nucleo-l073rz
BOARD ?= nucleo-f722ze

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 0
OUTPUT = 0
CFLAGS += -DOUTPUT=$(OUTPUT)

# Use a peripheral timer for the delay, if available
FEATURES_REQUIRED += periph_timer

USEMODULE += BoRa_decoder
USEMODULE += printf_float
USEMODULE += sema
USEMODULE += sdmmc
USEMODULE += shell
USEMODULE += ztimer
USEMODULE += ztimer_usec
USEMODULE += fmt
CFLAGS += -DBORA=1
CFLAGS += -DADC_DMA_BUFFER_LEN=8192
CFLAGS += -DADC_DMA_TRANSFRE_NUM=4096
# The valid LORA_BW = 250, 500, 203, 406
# BW = 250, 1Msps
# BW = 500, 1Msps. For 1.5Msps, set LORA_BW = 501
# BW = 203, 1.625Msps
# BW = 406, 1.625Msps
CFLAGS += -DLORA_BW=501


WERROR = 0

include $(RIOTBASE)/Makefile.include
