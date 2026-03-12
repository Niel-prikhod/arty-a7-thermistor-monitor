
Generate Vivado's project and `.xsa` file:
```tcl
vivado -mode batch -source hw/scripts/build_proj.tcl
```
Generate Vitis's project, compile, and program device:
```bash
vitis -s sw/scripts/build_sw.py
```
