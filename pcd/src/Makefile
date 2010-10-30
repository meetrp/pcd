#
#  Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
# 
#  This application is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  version 2.1, as published by the Free Software Foundation.
# 
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
# 
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
#  Copyright (C) 2010 PCD Project - http://www.rt-embedded.com/pcd
# 
#  Makefile for pcd application

-include $(PCD_ROOT)/.config

CC := $(CONFIG_PCD_CROSS_COMPILER_PREFIX)gcc
CFLAGS += -MMD -Wall -fomit-frame-pointer -g

# includes
CFLAGS += -I$(PCD_ROOT)/pcd/include -I$(PCD_ROOT)/pcd/src/pcdapi/include -I$(PCD_ROOT)/ipc/include

# Libraries
LDFLAGS += -L$(PCD_ROOT)/ipc/src -lipc -L$(PCD_ROOT)/pcd/src/pcdapi/src -lpcd -lrt -lc

obj-y := $(patsubst %.c,%.o,$(shell ls *.c 2> /dev/null))
TARGET = pcd

all: $(TARGET) install

$(TARGET): $(obj-y)
	@echo "  LINK	 	$@"
	@$(CC) $(CFLAGS) $(obj-y) -o $@ $(LDFLAGS) -Wl,-Map,$@.map
	@install -p $(PCD_ROOT)/pcd/include/pcd.h $(PCD_ROOT)/pcd/include/ruleid.h $(PCD_ROOT)/include

install: $(TARGET) install_internal
ifdef CONFIG_PCD_INSTALL_DIR_PREFIX
	@mkdir -p $(CONFIG_PCD_INSTALL_DIR_PREFIX)/usr/sbin
	@install $(TARGET) $(CONFIG_PCD_INSTALL_DIR_PREFIX)/usr/sbin
endif	
	
install_internal:	
	@mkdir -p $(PCD_BIN)/target/usr/sbin
	@install $(TARGET) $(PCD_BIN)/target/usr/sbin

clean:
	@rm -f $(TARGET) $(obj-y) $(obj-y:.o=.d) $(TARGET).map
	@rm -f $(PCD_BIN)/target/usr/sbin/$(TARGET)
	@rm -f $(CONFIG_PCD_INSTALL_DIR_PREFIX)/usr/sbin/$(TARGET)

%.o: %.c
	@echo "  CC [C] 	$@"
	@$(CC) $(CFLAGS) -c $(CURDIR)/$< -o $@

-include $(obj-y:.o=.d)

