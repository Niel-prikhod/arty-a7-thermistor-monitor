# Arty A7 MicroBlaze Temperature Monitor

A bare-metal MicroBlaze embedded application running on the Digilent Arty A7-100T FPGA. This project reads an external NTC thermistor via the Xilinx XADC (System Monitor), calculates the temperature using the Steinhart-Hart equation, and outputs the data over a 115200-baud serial connection.

This repository is structured for standard FPGA Git workflows: hardware is fully reproducible via Tcl scripts, and software is cleanly separated from generated BSPs.

This project is similar to another my [project](https://github.com/Niel-prikhod/Nucleo_temp_measure) with NTC, but on STM32 Nucleo platform.

---

## Table of Contents

1. [Hardware Requirements & Wiring](#hardware-requirements--wiring)
2. [Technical Background](#technical-background)
3. [Repository Structure](#repository-structure)
4. [Prerequisites](#prerequisites)
5. [How to Reproduce (Build Instructions)](#how-to-reproduce-build-instructions)
6. [Programming & Running](#programming--running)
7. [Usage & Output](#usage--output)
8. [Troubleshooting](#troubleshooting)
9. [Learning Outcomes](#learning-outcomes)

---

## Hardware Requirements & Wiring

### Board & Components

| Component | Description |
|-----------|-------------|
| **Board** | Digilent Arty A7-100T |
| **Sensor** | 10kΩ NTC Thermistor (KY-013 is used) |
| **Cable** | Micro-USB for PROG/UART |

### Pin Mapping

| Signal | Arty A7 Pin | Function |
|--------|-------------|----------|
| VAUX4 (A0) | C5/C6 | Thermistor analog input (ChipKit outer header) |
| UART TX | D10 | Serial output to USB |
| UART RX | A9 | Serial input from USB |
| clk_100MHz | E3 | System clock (on-board 100MHz oscillator) |
| reset | C2 | System reset button |

### Wiring Diagram

The XADC on the Arty A7 accepts analog inputs from 0V to 1V. Construct a voltage divider as follows:

```
                         +3.3V
                           |
                           |
                    +------+------+
                    |   10kΩ     |
                    |  Resistor  |
                    +------+------+
                           |
                           +----> VAUX4 (A0) ---> XADC Channel 4
                           |     (Pin C6/C5)
                           |
                    +------+------+
                    |  10kΩ NTC   |
                    | Thermistor  |
                    +------+------+
                           |
                          GND
```

> **Note:** The voltage at VAUX4 is the midpoint of the divider, scaled by R107 (2.32kΩ) and R140 (1kΩ) in the firmware to compensate for the voltage divider ratio.

---

## Technical Background

### NTC Thermistors

An NTC (Negative Temperature Coefficient) thermistor is a resistor whose resistance decreases as temperature increases. They are commonly used for temperature measurement because of their high sensitivity.

### The Steinhart-Hart Equation

The Steinhart-Hart equation provides accurate temperature readings for NTC thermistors:

$$ \frac{1}{T} = C_1 + C_2 \ln(R) + C_3 \big(\ln(R)\big)^3 $$

Where:
- T = Temperature in Kelvin
- R = Thermistor resistance in Ohms
- C1, C2, C3 = Steinhart-Hart coefficients (specific to each thermistor)

The coefficients used in this project (C1=1.129148e-3, C2=2.34125e-4, C3=8.76741e-8) are typical for 10kΩ NTC thermistors with β≈3950.

### XADC (System Monitor)

The Xilinx XADC is a 12-bit analog-to-digital converter built into Artix-7 FPGAs. It provides:
- 17 differential analog inputs (or 34 single-ended)
- 1MHz sampling rate
- Built-in calibration
- Access via AXI4-Lite interface

In this project, we configure the XADC to continuously sample VAUX4 (channel 4), which maps to the ChipKit analog pin A0.

### Voltage Divider Scaling

The firmware applies a scaling factor to account for the actual voltage divider ratio:

```c
v_sig = v_out * (R107 + R140) / R140;
signal.resistance = v_sig * RESISTOR / (V_REF - v_sig);
```

Where R107=2320Ω and R140=1000Ω are scaling resistors on the board (or applied in firmware), and RESISTOR=10000Ω is the fixed resistor value.

---

## Repository Structure

```
arty-a7-thermistor-monitor/
├── README.md                    # This file
├── Makefile                     # Build automation
│
├── hw/                          # Hardware sources
│   ├── constraints/
│   │   └── arty_a7.xdc          # Pin constraints for Arty A7
│   ├── scripts/
│   │   ├── build_proj.tcl      # Main Vivado build script
│   │   └── create_bd.tcl       # Block Design generation
│   ├── microblaze_soft_core/   # Vivado project (auto-generated)
│   └── prebuilt/               # Pre-built bitstream and XSA
│       ├── system.bit          # FPGA bitstream
│       └── system.xsa          # Hardware platform for Vitis
│
├── sw/                          # Software sources
│   ├── src/
│   │   └── main.c              # Main application code
│   ├── scripts/
│   │   ├── build_sw.py         # Vitis build script
│   │   ├── compile.py          # Re-compile only
│   │   └── load_elf.tcl        # ELF loading script
│   └── prebuilt/
│       └── thermistor.elf      # Pre-built executable
│
└── docs/                        # Documentation 
```

---

## Prerequisites

Before building this project, ensure you have:

| Tool | Version | Purpose |
|------|---------|---------|
| **Vivado** | 2025.2 | FPGA synthesis and implementation |
| **Vitis** | 2025.2 | Software IDE and compiler |
| **openFPGALoader** | Latest | Programming the FPGA |
| **minicom** | Latest | Serial terminal for output |
| **Python** | 3.8+ | Running build scripts |

---

## How to Reproduce (Build Instructions)

This project avoids committing large binary files. You can rebuild the entire system from source in approximately 10 minutes.

### Option A: Build Everything at Once

```bash
make all
```

This will:
1. Run Vivado to synthesize and implement the hardware
2. Export the hardware platform (.xsa)
3. Run Vitis to build the software application
4. Produce `hw/prebuilt/system.bit` and `sw/prebuilt/thermistor.elf`

### Option B: Build Hardware Only

```bash
make generate_bitstream
```

This runs the Vivado Tcl script (`hw/scripts/build_proj.tcl`) which:
- Creates a new Vivado project
- Generates the Block Design (MicroBlaze + XADC + UART + BRAM)
- Synthesizes and implements the design
- Exports `system.xsa` and `system.bit` to `hw/prebuilt/`

### Option C: Build Software Only

```bash
make generate_elf
```

Or for a quick recompile of C code only:

```bash
make compile
```

### Manual Build (Advanced)

#### Phase 1: Hardware (Vivado)

```bash
cd /path/to/arty-a7-thermistor-monitor
vivado -mode batch -source hw/scripts/build_proj.tcl
```

#### Phase 2: Software (Vitis)

```bash
vitis -s sw/scripts/build_sw.py
```

---

## Programming & Running

### Programming the Board

Connect your Arty A7 via Micro-USB (the USB-JTAG/UART port).

```bash
# Programs FPGA bitstream and loads ELF
make program
```

This command:
1. Programs the FPGA with `hw/prebuilt/system.bit` using openFPGALoader
2. Loads the software executable `sw/prebuilt/thermistor.elf` using xsct

### Viewing Serial Output

```bash
# Opens minicom at 115200 baud
make watch
```

> **Note:** If your device uses a different tty (e.g., ttyUSB0), modify the Makefile or run:
> ```bash
> minicom -D /dev/ttyUSB0 -b 115200
> ```

---

## Usage & Output

### Example Output

```
--- Arty A7 Thermistor Monitor Started ---
XADC Initialized Successfully. Reading VAUX4 (Pin A0)...

Raw_value,Voltage(V),Resistance(Ohm),Temperature(C)
651,0.524,11171,22.49
652,0.525,11193,22.54
653,0.526,11215,22.59
```

### Output Format

| Column | Description |
|--------|-------------|
| Raw_value | 12-bit ADC reading (0-4095) |
| Voltage(V) | Measured voltage at VAUX4 (scaled) |
| Resistance(Ohm) | Calculated thermistor resistance |
| Temperature(C) | Temperature in Celsius |

---

## Troubleshooting

### Error: "Voltage too low. Check wiring!"

**Cause:** The XADC reads near 0V, indicating an open circuit.

**Solutions:**
1. Verify all four connections: 3.3V → Resistor → VAUX4 → Thermistor → GND
2. Check that the thermistor is not shorted to GND
3. Use a multimeter to verify voltage at VAUX4 is between 0.1V and 3.0V

### No Serial Output

**Causes & Solutions:**
1. **Wrong serial port:** Check `/dev/ttyUSB1` or use `ls /dev/ttyUSB*`
2. **Wrong baud rate:** Verify 115200 baud, 8N1
3. **Board not detected:** Try a different USB cable (some are power-only)
4. **FPGA not programmed:** Ensure `make program` completed successfully

### XADC Initialization Failed

**Cause:** XADC hardware not properly configured.

**Solutions:**
1. Verify the Block Design includes XADC (xadc_wiz_0)
2. Check that VAUX4 is enabled in the XADC sequencer
3. Ensure `system.bit` was properly generated

### Build Errors

- **Vivado errors:** Ensure you have the correct version (2025.2) and the Arty A7 board files installed
- **Vitis errors:** Ensure the hardware platform (.xsa) was successfully exported from Vivado

---

## Learning Outcomes

This project demonstrates the following skills and concepts:

- **Embedded FPGA Design:** Integration of a soft-core processor (MicroBlaze) with FPGA fabric
- **Mixed-Signal FPGA:** Configuring and interacting with the Xilinx XADC (System Monitor) via AXI4-Lite
- **Bare-Metal C Programming:** Writing hardware-level drivers (xsysmon.h), configuring UART polling, and managing limited BRAM footprints
- **Algorithm Implementation:** Converting raw 12-bit ADC data into real-world temperature values using floating-point math and the Steinhart-Hart equation
- **Scripted Reproducibility:** Utilizing Tcl to maintain a clean, Git-friendly FPGA project structure
- **Hardware/Software Co-Design:** Understanding the relationship between hardware IP configuration and software driver development
