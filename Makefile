VIVADO_SCRIPT = hw/scripts/build_proj.tcl
VITIS_SCRIPT = sw/scripts/build_sw.py

all: generate_bitstream generate_elf

generate_bitstream:
	vivado -mode batch -source $(VIVADO_SCRIPT)

generate_elf:
	vitis -s $(VITIS_SCRIPT)
	ln -s hw/sw_project/thermistor_app/build/compile_commands.json .

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
