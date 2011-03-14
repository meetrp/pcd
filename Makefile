#
#  Copyright (C) 2010 PCD Project - http://www.rt-embedded.com/pcd
# 
#  This application is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  version 2.1, as published by the Free Software Foundation.
# 
#  This application is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
# 
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
#  Makefile for pcd project

PCD_ROOT := $(CURDIR)
PCD_BIN := $(PCD_ROOT)/bin
PCD_CFG_DIR := $(PCD_ROOT)/scripts/configs
PCD_AUTO_CFG_FILE := $(PCD_CFG_DIR)/auto.conf
PCD_KCFG_DIR := $(PCD_ROOT)/scripts/kconfig

PCD_CONFIG_IN := $(PCD_CFG_DIR)/PCDConfig.in
PCD_DEF_CFG_FILE := $(PCD_CFG_DIR)/pcd_defconfig
QCONF := $(PCD_KCFG_DIR)/qconf
MCONF := $(PCD_KCFG_DIR)/mconf
CONF := $(PCD_KCFG_DIR)/conf

export CFLAGS += -I$(PCD_CFG_DIR)

# Optimization selection
ifdef PCD_OPTIMIZE_FOR_SIZE
CFLAGS += -Os
else
CFLAGS += -O2
endif

# Export user's flags
CFLAGS += $(shell echo $(CONFIG_PCD_EXTRA_CFLAGS))
LDFLAGS += $(shell echo $(CONFIG_PCD_EXTRA_LDFLAGS)) 

export PCD_ROOT PCD_BIN

-include $(PCD_ROOT)/.config

all: pcd

help:
	  @echo "Configuration commands:"
	  @echo "- make defconfig - Configure default PCD settings."
	  @echo "- make menuconfig - Configure the PCD settings in a textual menu."
	  @echo "- make xconfig - Configure the PCD settings in a graphical menu (requires qt library)."
	  @echo "- make oldconfig - Configure the PCD using existing settings in .config file."
	  @echo " "
	  @echo "Compilation commands:"
	  @echo "- make pcd - Compile all PCD components."
	  @echo "- make install - Install all PCD components in the filesystem."
	  @echo "- make clean - Cleans all PCD components (executables, libraries and objects)."
	  @echo "- make distclean - Cleans also all configuration files."

conf:
	@$(MAKE) -C $(PCD_KCFG_DIR) -s 2> /dev/null

pcd_title:
	@echo "*--------------------------------------------------"
	@echo "*  Process Control Daemon"
	@echo "*  Build date: $(shell date -R) :"
	@echo "*--------------------------------------------------"

defconfig: pcd_title conf
	@echo Configuring PCD...
	@rm -f $(PCD_ROOT)/.config
	@cp $(PCD_DEF_CFG_FILE) $(PCD_ROOT)/.config
	@$(CONF) -s $(PCD_CONFIG_IN)
	@if [ -f $(PCD_ROOT)/autoconf.h ]; then \
    	mv auto.conf $(PCD_CFG_DIR) ;\
		mv autoconf.h $(PCD_CFG_DIR)/pcd_autoconf.h ;\
	fi
	@echo PCD Configuration completed.
	@echo Please make sure that you have write permissions to installation directories.

oldconfig: pcd_title conf
	@echo Configuring PCD...
	@if [ ! -f $(PCD_ROOT)/.config ]; then \
	  cp $(PCD_DEF_CFG_FILE) $(PCD_ROOT)/.config ;\
	fi
	@$(CONF) -s $(PCD_CONFIG_IN)
	@if [ -f $(PCD_ROOT)/autoconf.h ]; then \
    	mv auto.conf $(PCD_CFG_DIR) ;\
		mv autoconf.h $(PCD_CFG_DIR)/pcd_autoconf.h ;\
	fi
	@echo PCD Configuration completed.
	@echo Please make sure that you have write permissions to installation directories.

xconfig: pcd_title conf
	@echo Configuring PCD...
	@if [ ! -f $(PCD_ROOT)/.config ]; then \
	  cp $(PCD_DEF_CFG_FILE) $(PCD_ROOT)/.config ;\
	fi
	@$(QCONF) $(PCD_CONFIG_IN) $(PCD_ROOT)/.config
	@$(CONF) -s $(PCD_CONFIG_IN)
	@if [ -f $(PCD_ROOT)/autoconf.h ]; then \
    	mv auto.conf $(PCD_CFG_DIR) ;\
		mv autoconf.h $(PCD_CFG_DIR)/pcd_autoconf.h ;\
	fi
	@echo PCD Configuration completed.
	@echo Please make sure that you have write permissions to installation directories.

menuconfig: pcd_title conf
	@echo Configuring PCD...
	@if [ ! -f $(PCD_ROOT)/.config ]; then \
	  cp $(PCD_DEF_CFG_FILE) $(PCD_ROOT)/.config ;\
	fi
	@$(MCONF) $(PCD_CONFIG_IN) $(PCD_ROOT)/.config
	@$(CONF) -s $(PCD_CONFIG_IN)
	@if [ -f $(PCD_ROOT)/autoconf.h ]; then \
    	mv auto.conf $(PCD_CFG_DIR) ;\
		mv autoconf.h $(PCD_CFG_DIR)/pcd_autoconf.h ;\
	fi
	@echo PCD Configuration completed. 
	@echo Please make sure that you have write permissions to installation directories.

check_permissions:
	@echo Checking write permission: $(CONFIG_PCD_INSTALL_HEADERS_DIR_PREFIX)
	@mkdir -p $(CONFIG_PCD_INSTALL_HEADERS_DIR_PREFIX) && touch $(CONFIG_PCD_INSTALL_HEADERS_DIR_PREFIX)/pcd.tmp 2> /dev/null && rm $(CONFIG_PCD_INSTALL_HEADERS_DIR_PREFIX)/pcd.tmp
	@echo Checking write permission: $(CONFIG_PCD_INSTALL_DIR_HOST)
	@mkdir -p $(CONFIG_PCD_INSTALL_DIR_HOST) && touch $(CONFIG_PCD_INSTALL_DIR_HOST)/pcd.tmp 2> /dev/null && rm $(CONFIG_PCD_INSTALL_DIR_HOST)/pcd.tmp
	@echo Checking write permission: $(CONFIG_PCD_INSTALL_DIR_PREFIX)
	@mkdir -p $(CONFIG_PCD_INSTALL_DIR_PREFIX) && touch $(CONFIG_PCD_INSTALL_DIR_PREFIX)/pcd.tmp 2> /dev/null && rm $(CONFIG_PCD_INSTALL_DIR_PREFIX)/pcd.tmp

check_config:
	@if [ ! -f $(PCD_ROOT)/.config ]; then \
	  echo "PCD Configuration not found." ;\
	  echo "Please run one of the configuration commands (run make help for details)." ;\
	  exit 1;\
	fi

pcd: pcd_title check_config check_permissions
	@echo "Building PCD..."
	@$(MAKE) -C ./ipc/src
	@$(MAKE) -C ./pcd/src/pcdapi/src
	@$(MAKE) -C ./pcd/src
	@$(MAKE) -C ./pcd/src/parser/src
	@install -p $(PCD_ROOT)/scripts/configs/pcd_autoconf.h $(PCD_ROOT)/include
	@echo PCD build completed. 

install: pcd_title check_permissions
	@echo "Installing PCD..."
	@$(MAKE) -C ./ipc/src install
	@$(MAKE) -C ./pcd/src/pcdapi/src install
	@$(MAKE) -C ./pcd/src install	
	@$(MAKE) -C ./pcd/src/parser/src install
	@if [ "$(PCD_ROOT)/include" != "$(CONFIG_PCD_INSTALL_HEADERS_DIR_PREFIX)" ]; then \
		install -p $(PCD_ROOT)/include/*.h $(CONFIG_PCD_INSTALL_HEADERS_DIR_PREFIX) ;\
	fi

clean:
	@$(MAKE) -C ./pcd/src clean -s
	@$(MAKE) -C ./pcd/src/parser/src clean -s
	@$(MAKE) -C ./pcd/src/pcdapi/src clean -s
	@$(MAKE) -C ./ipc/src clean -s
	@$(MAKE) -C $(PCD_KCFG_DIR) clean -s
	@rm -f $(PCD_ROOT)/include/*

distclean: clean
	@rm -f .config .config.old $(PCD_CFG_DIR)/pcd_autoconf.h $(PCD_CFG_DIR)/auto.conf
	@rm -f $(PCD_KCFG_DIR)/.config $(PCD_KCFG_DIR)/.config.old $(PCD_KCFG_DIR)/pcd_autoconf.h $(PCD_KCFG_DIR)/auto.conf

.PHONY: all install check_permissions check_config clean distclean help conf pcd_title menuconfig xconfig defconfig oldconfig