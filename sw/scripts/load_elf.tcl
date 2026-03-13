connect
targets -set -filter {name =~ "microblaze*"}
rst -processor
dow "hw/prebuilt/thermistor.elf"
con
exit
