# A little excursion to the grand scheme of things:
#
# Unlike how it used to be in the past, now we don't use the external lxdialog
# utility in make menuconfig, but we link the lxdialog suite directly against
# mconf. Also, mconf is linked against ncurses only over there, regardless
# to what you see here in HOST_LOADLIBES; here we merely check for ncurses
# presence.
#
# 2005-12-11 Petr Baudis <pasky@suse.cz>

HOST_EXTRACFLAGS := -Wall -Wstrict-prototypes -O2 -fomit-frame-pointer -DLOCALE 
ifeq ($(shell uname),SunOS)
HOST_LOADLIBES   := -lcurses
else
HOST_LOADLIBES   := -lncurses
endif

ifeq (/usr/include/ncurses/ncurses.h, $(wildcard /usr/include/ncurses/ncurses.h))
	HOST_EXTRACFLAGS += -I/usr/include/ncurses -DCURSES_LOC="<ncurses.h>"
else
ifeq (/usr/include/ncurses/curses.h, $(wildcard /usr/include/ncurses/curses.h))
	HOST_EXTRACFLAGS += -I/usr/include/ncurses -DCURSES_LOC="<ncurses/curses.h>"
else
ifeq (/usr/include/ncurses.h, $(wildcard /usr/include/ncurses.h))
	HOST_EXTRACFLAGS += -DCURSES_LOC="<ncurses.h>"
else
	HOST_EXTRACFLAGS += -DCURSES_LOC="<curses.h>"
endif
endif
endif

hostprogs-y	:= lxdialog
always		:= ncurses $(hostprogs-y)

liblxdialog-objs := checklist.o menubox.o textbox.o yesno.o inputbox.o \
		util.o msgbox.o

lxdialog-objs := checklist.o menubox.o textbox.o yesno.o inputbox.o \
		util.o msgbox.o lxdialog.o

all: $(lxdialog-objs)
	$(CC) $^ -o $(hostprogs-y) -lncurses

liblxdialog.a: $(liblxdialog-objs)
	$(AR) rs $@ $^ 

%.o: %.c
	$(CC) $(HOST_EXTRACFLAGS) -fPIC -c $< 

clean:
	rm -f $(hostprogs-y) $(liblxdialog-objs) $(lxdialog-objs) liblxdialog.a

.PHONY: $(obj)/ncurses
$(obj)/ncurses:
	@echo "main() {}" > lxtemp.c
	@if $(HOSTCC) lxtemp.c  $(HOST_LOADLIBES); then \
		rm -f lxtemp.c a.out; \
	else \
		rm -f lxtemp.c; \
		echo -e "\007" ;\
		echo ">> Unable to find the Ncurses libraries." ;\
		echo ">>" ;\
		echo ">> You must install ncurses-devel in order" ;\
		echo ">> to use 'make menuconfig'" ;\
		echo ;\
		exit 1 ;\
	fi
