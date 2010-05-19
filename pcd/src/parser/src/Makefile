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
#  Makefile for pcdparser application

CC = gcc
CFLAGS = -MMD -O2 -Wall -fomit-frame-pointer -DPCD_HOST_BUILD
LDFLAGS =

# includes
CFLAGS += -I../include -I../../../include

obj-y := $(patsubst %.c,%.o,$(shell ls *.c 2> /dev/null))
TARGET = pcdparser

all: $(TARGET)

$(TARGET): $(obj-y)
	@echo "  LINK	 	$@"
	@$(CC) $(CFLAGS) $(obj-y) -o $@ $(LDFLAGS) -Wl,-Map,$@.map

install: $(TARGET)

clean:
	@rm -f $(TARGET) $(obj-y) $(obj-y:.o=.d) $(TARGET).map


%.o: %.c
	@echo "  CC [C] 	$@"
	@$(CC) $(CFLAGS) -c $(CURDIR)/$< -o $@

-include $(obj-y:.o=.d)

