PKG_NAME=lorabasicsmodem
# PKG_URL=https://github.com/lorabasics/lorabasicsmodem.git
PKG_URL=https://github.com/sada45/BoRa-lorabasicsmodem.git
# PKG_VERSION=04e415bcfbdc7f1f4bff918a4867fab53bc8bb8a
PKG_VERSION=ff2d7120f8d749f518ce494cba186d01b575617f
PKG_LICENSE=BSD

include $(RIOTBASE)/pkg/pkg.mk

CFLAGS += -Wno-error=unused-parameter
CFLAGS += -Wno-error=unused-function

IGNORE_MODULES := lorabasics_driver_sx1280_hal
                  #

LORABASICS_MODULES := $(filter-out $(IGNORE_MODULES),$(filter lorabasics%,$(USEMODULE)))

all: $(LORABASICS_MODULES)
	@true

lorabasics_smtc_ral:
	$(QQ)"$(MAKE)" -C $(PKG_SOURCE_DIR)/smtc_ral/src -f $(RIOTBASE)/Makefile.base MODULE=$@ SRC="ral.c ral_sx1280.c"

lorabasics_sx1280_driver:
	$(QQ)"$(MAKE)" -C $(PKG_SOURCE_DIR)/sx1280_driver/src -f $(RIOTBASE)/Makefile.base MODULE=$@
