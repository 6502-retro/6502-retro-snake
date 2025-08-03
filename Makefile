# Sources and objects
C_SOURCES = \
	    vdp.c \
	    main.c
AS_SOURCES = \
	     6502retro_lib.asm

DRIVENAME = g://
APPNAME = snake

# DO NOT EDIT THIS
include Make.rules# Sources and objects

$(BUILD_DIR)/$(APPNAME).raw: $(AS_SOURCES) $(C_SOURCES)
	mkdir -pv $(BUILD_DIR)
	$(AS) $(ASFLAGS) -l $(BUILD_DIR)/vdp.lst -o $(BUILD_DIR)/6502retro_lib.o 6502retro_lib.asm
	$(CC) $(CCFLAGS) -O -o $(BUILD_DIR)/$(APPNAME).raw $(C_SOURCES) $(BUILD_DIR)/6502retro_lib.o lib/sfoslib.lib 

$(BUILD_DIR)/$(APPNAME).bin: $(BUILD_DIR)/$(APPNAME).raw
	$(LOADTRIM) $^ $@ $(LOAD_ADDR)

copy: build/$(APPNAME).com
	../6502-retro-os/py_sfs_v2/cli.py rm -i ../6502-retro-os/py_sfs_v2/6502-retro-sdcard.img -d $(DRIVENAME)$(APPNAME).com
	../6502-retro-os/py_sfs_v2/cli.py cp -i ../6502-retro-os/py_sfs_v2/6502-retro-sdcard.img -s build/$(APPNAME).com -d $(DRIVENAME)$(APPNAME).com
