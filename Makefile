VIVADO_SCRIPT = hw/scripts/build_proj.tcl
VITIS_SCRIPT = sw/scripts/build_sw.py
VITIS_WS = sw/vitis_workspace
APP_NAME = thermistor_app

all: generate_bitstream generate_elf

generate_bitstream:
	vivado -mode batch -source $(VIVADO_SCRIPT)

generate_elf:
	vitis -s $(VITIS_SCRIPT)
	ln -s $(VITIS_WS)/thermistor_app/build/compile_commands.json . || true

compile:
	vitis -s sw/scripts/compile.py
	cp $(VITIS_WS)/$(APP_NAME)/build/$(APP_NAME).elf sw/prebuilt/thermistor.elf

re: compile program-sw

clean: 
	rm -rf hw/microblaze_soft_core sw/arty_platform .Xil
	rm *.log *.jou
	
fclean: clean
	rm sw/prebuild/* hw/prebuild/*

program: program-hw program-sw

program-hw:
	openFPGALoader -b arty_a7_100t hw/prebuilt/system.bit

program-sw:
	xsct sw/scripts/load_elf.tcl

watch:
	minicom -D /dev/ttyUSB1 -b 115200
