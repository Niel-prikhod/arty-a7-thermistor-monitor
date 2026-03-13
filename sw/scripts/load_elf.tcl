connect
# targets -set -filter {name =~ "MicroBlaze*"}
targets 3
rst -processor
dow "sw/prebuilt/thermistor.elf"
con
exit
