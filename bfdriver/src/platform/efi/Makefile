

TOP			:= 	$(shell pwd)
ARCH		:= 	$(shell uname -m)

include Make.efi

SYSROOT		?= ../../../../../build/prefixes/x86_64-userspace-elf

LIBDIR		:= $(SYSROOT)/lib

CRTOBJ 		:= $(LIBDIR)/crt0-efi-$(ARCH).o
LDSCRIPT 	:= $(LIBDIR)/elf_$(ARCH)_efi.lds

CFLAGS  	+= -I$(TOP)/include \
			   -I$(TOP)/modules \
			   -I$(TOP)/include/gnuefi \
			   -I$(TOP)/../../../include \
			   -I$(TOP)/../../../../bfsdk/include \
			   -I$(TOP)/../../../../bfelf_loader/include \
			   -I$(SYSROOT)/include/efi \
			   -I$(SYSROOT)/include/efi/x86_64 


LDFLAGS		+= -shared -Bsymbolic -L$(LIBDIR)
LDFLAGS		+= --defsym=EFI_SUBSYSTEM=0xa --no-undefined
LOADLIBES 	+= $(CRTOBJ) -lefi -lgnuefi
LOADLIBES	+=  -T $(LDSCRIPT)

FORMAT		:= -O binary --target=efi-app-$(ARCH)

FILES		:= 	bfloader console boot platform common start x86_64 gnuefi/bflib
OBJS		:=	$(foreach FILE,$(FILES),src/$(FILE).o)
OBJS 		+=	$(foreach FILE,$(wildcard modules/*.c),$(FILE:.c=.o))
TARGET 		:=	bfloader

all: $(TARGET).efi 

$(TARGET).efi: $(TARGET).so
	objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
			-j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* \
			-j .reloc $(FORMAT) $^ $@
	rm -f bfloader.so

$(TARGET).so: $(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@ $(LOADLIBES)

$(OBJS): Makefile


%.o: %.S
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -c -o $@

src/common.o: ../../common.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -c -o $@

clean: 
	rm -rf $(TARGET).efi $(TARGET).so src/*.o modules/*.o

.PHONY: clean 
