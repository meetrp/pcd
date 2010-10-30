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
#  Makefile for pcd library

-include $(PCD_ROOT)/.config

CC := $(CONFIG_PCD_CROSS_COMPILER_PREFIX)gcc
AR := $(CONFIG_PCD_CROSS_COMPILER_PREFIX)ar
CFLAGS += -MMD -Wall -fomit-frame-pointer -fPIC -g
LDFLAGS += -shared

# includes
CFLAGS += -I$(PCD_ROOT)/pcd/include -I$(PCD_ROOT)/pcd/src/pcdapi/include -I$(PCD_ROOT)/ipc/include

obj-y := $(patsubst %.c,%.o,$(shell ls *.c 2> /dev/null))
TARGET = libpcd

all: $(TARGET) install

$(TARGET): $(obj-y)
	@echo "  LINK	 	$@"
ifdef PCD_USE_STATIC_LIBS
	@$(AR) rcs $@.a $(obj-y)
else
	@$(CC) $(CFLAGS) $(obj-y) -o $@.so $(LDFLAGS) -Wl,-Map,$@.map
endif
	@install -p $(PCD_ROOT)/pcd/src/pcdapi/include/*.h $(PCD_ROOT)/include

install: $(TARGET) install_internal
ifdef CONFIG_PCD_INSTALL_DIR_PREFIX
	@mkdir -p $(CONFIG_PCD_INSTALL_DIR_PREFIX)/lib
ifdef USE_STATIC_LIBS
	@install $(TARGET).a $(CONFIG_PCD_INSTALL_DIR_PREFIX)/lib
else
	@install $(TARGET).so $(CONFIG_PCD_INSTALL_DIR_PREFIX)/lib
endif
endif

install_internal:
	@mkdir -p $(PCD_BIN)/target/lib
ifdef USE_STATIC_LIBS
	@install $(TARGET).a $(PCD_BIN)/target/lib
else
	@install $(TARGET).so $(PCD_BIN)/target/lib
endif

clean:
	@rm -f $(TARGET).so $(TARGET).a $(obj-y) $(obj-y:.o=.d) $(TARGET).map
	@rm -f $(PCD_BIN)/target/lib/$(TARGET).*
	@rm -f $(CONFIG_PCD_INSTALL_DIR_PREFIX)/lib/$(TARGET).*

%.o: %.c
	@echo "  CC [C] 	$@"
	@$(CC) $(CFLAGS) -c $(CURDIR)/$< -o $@

-include $(obj-y:.o=.d)

