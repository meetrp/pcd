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
#  Makefile for ipc library

CC ?= gcc
AR ?= ar
CFLAGS = -MMD -O2 -Wall -fomit-frame-pointer -fPIC -g
LDFLAGS = -shared

# includes
CFLAGS += -I../include

ifdef INCLUDE_DIR_PREFIX
CFLAGS += -I$(INCLUDE_DIR_PREFIX)
endif

obj-y := $(patsubst %.c,%.o,$(shell ls *.c 2> /dev/null))
TARGET = libipc

all: $(TARGET) install

$(TARGET): $(obj-y)
	@echo "  LINK	 	$@"
	@$(CC) $(CFLAGS) $(obj-y) -o $@.so $(LDFLAGS) -Wl,-Map,$@.map
	@$(AR) rcs $@.a $(obj-y)

install: $(TARGET)
ifdef INSTALL_DIR_PREFIX
	@mkdir -p $(INSTALL_DIR_PREFIX)/lib
	@install $?.so $(INSTALL_DIR_PREFIX)/lib
	@install $?.a $(INSTALL_DIR_PREFIX)/lib
endif

clean:
	@rm -f $(TARGET).so $(TARGET).a $(obj-y) $(obj-y:.o=.d) $(TARGET).map


%.o: %.c
	@echo "  CC [C] 	$@"
	@$(CC) $(CFLAGS) -c $(CURDIR)/$< -o $@

-include $(obj-y:.o=.d)

