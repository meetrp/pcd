#
#  Copyright (C) 2010 Texas Instruments Incorporated - http://www.ti.com/
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
#  Copyright (C) 2010 PCD Project - http://www.rt-embedded.com/pcd
# 
#  Makefile for pcdparser application

-include $(PCD_ROOT)/.config

CC = gcc
CFLAGS += -MMD -O2 -Wall -fomit-frame-pointer -DPCD_HOST_BUILD
LDFLAGS =

# includes
CFLAGS += -I$(PCD_ROOT)/pcd/src/parser/include -I$(PCD_ROOT)/pcd/include

obj-y := $(patsubst %.c,%.o,$(shell ls *.c 2> /dev/null))
TARGET = pcdparser

all: $(TARGET)

$(TARGET): $(obj-y)
	@echo "  LINK	 	$@"
	@$(CC) $(CFLAGS) $(obj-y) -o $@ $(LDFLAGS) -Wl,-Map,$@.map

install: $(TARGET)
	@install $(TARGET) $(PCD_BIN)/host
ifneq ($(CONFIG_PCD_INSTALL_DIR_HOST),)
	@install $(TARGET) $(CONFIG_PCD_INSTALL_DIR_HOST)
endif	

clean:
	@rm -f $(TARGET) $(obj-y) $(obj-y:.o=.d) $(TARGET).map
	@rm -f $(PCD_ROOT)/bin/host/$(TARGET)
	@rm -f $(CONFIG_PCD_INSTALL_DIR_HOST)/$(TARGET)

%.o: %.c
	@echo "  CC [C] 	$@"
	@$(CC) $(CFLAGS) -c $(CURDIR)/$< -o $@

-include $(obj-y:.o=.d)

