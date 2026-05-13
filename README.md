# Raspberry Pi CPU Emulator & Cache Simulator
A Linux kernel project that emulates a custom CPU on a Raspberry Pi, exposes it through character devices, and tracks memory accesses for post-execution cache simulation. 
The project combines kernel module development, embedded I/O, instruction set emulation, and cache performance analysis. Programs are assembled in userspace, sent to the kernel driver instruction by instruction, and executed with support for memory access, branching, and GPIO-backed input/output.

---

## Highlights
* **CPU Emulator (Kernel Module)**: Implements a custom CPU execution engine.
* **Trace Collector (Kernel Module)**: Intercepts and buffers memory access traces in chunks of 256 entries for performance analysis.
* **Character Device Interfaces**: Uses `/dev/cpu_emulator` for execution and `/dev/trace_collector` for memory trace retrieval.
* **Userspace Assembler**: Parses custom assembly into an 11-byte machine instruction format.
* **Cache Simulator**: Runs in userspace after program execution to analyze the collected memory traces.
* **GPIO Integration**: Supports LEDs, run/pause control, step signals, and simple input/output devices.

---

## System Architecture
The project is split into multiple interacting layers:
1. **Assembler (Userspace)**: Parses assembly files from the `./doc/` directory and converts them into machine code.
2. **Execution Client (Userspace)**: Loads instructions into simulated memory, communicates with `/dev/cpu_emulator`, services memory/MMIO requests, and triggers the cache simulation upon completion.
3. **CPU Emulator (Kernelspace)**: Executes instructions, updates CPU state, handles GPIO-based control flow, and exposes the CPU device interface.
4. **Trace Collector (Kernelspace)**: Safely buffers memory access events generated during CPU execution using a chunk-based linked list protected by a mutex.

---

## CPU Model & Instruction Set
The emulator implements a compact CPU with 4 general-purpose registers (`R0` to `R3`), a Program Counter (`PC`), and a Flags Register (`FR`) supporting Carry (`CF`), Auxiliary Carry (`AF`), Zero (`ZF`), Sign (`SF`), and Overflow (`OF`) flags.

### Instruction Format
Each instruction occupies 11 bytes:
 opcode (1) | mode (1) | operand1 (1) | operand2 (8) |
### Supported Instructions
* **Arithmetic/Logic**: `ADD`, `SUB`, `AND`, `OR`, `NOT`
* **Data Movement**: `MOV`
* **Branching/Comparison**: `CMP`, `JMP`, `JE`, `JG`, `JL`

**Addressing Modes**: `IMMEDIATE`, `REGISTER`, `DIRECT_LOAD`, `DIRECT_STORE`, `LABEL`

---

## Hardware Integration (GPIO & MMIO)
The CPU module communicates with the physical world via Raspberry Pi GPIO pins.

* **LEDs (Register State)**: `GPIO17`, `GPIO27`, `GPIO22`, `GPIO26`
* **Switches**:
  * `GPIO23`: Run switch (Triggers execution)
  * `GPIO24`: Pause switch (Halts execution temporarily)
  * `GPIO25`: Input device / Step signal (Advances execution by one instruction when paused)
* **Output**: `GPIO16` (MMIO output device)

**MMIO Addresses**:
* `0xFFFD`: MMIO input
* `0xFFFE`: MMIO output
* `0xFFFF`: Halt marker

---

## Repository Structure
```text
.
├── cpu_driver/
│   ├── cpu_emulator_driver.c    # Kernel module implementing the CPU
│   └── Makefile                 
├── trace_collector_driver/
│   ├── trace_collector_driver.c # Kernel module for logging memory traces
│   └── Makefile                 
├── userspace/
│   ├── src/
│   │   ├── main.c               # Userspace entry point & cache sim trigger
│   │   ├── assembler.c          # Custom assembler
│   │   ├── cache_simulator.c    # Post-execution cache simulator
│   │   └── cpu_emulator_userspace.c
│   ├── inc/                     # Shared userspace headers
│   ├── doc/                     # Assembly test files
│   └── Makefile                 
├── cpu_uapi.h                   # Shared user/kernel interface definitions
└── run.sh                       # Automation script for loading modules and executing
```

---

## Build & Run Instructions
A helper script `run.sh` is provided to automate module loading and program execution.

### Usage
```bash
./run.sh [clock_speed_ms] <assembly_file>
```
> **Note:** The assembly file should be located in the `userspace/doc/` directory.

### Example
```bash
./run.sh 200 test1.txt
```

### What the script does
1. Removes any previously loaded `cpu_emulator_driver` and `trace_collector_driver` modules.
2. Loads the `cpu_emulator_driver.ko` module, optionally setting the `clock_speed` parameter (defaults to 500ms).
3. Loads the `trace_collector_driver.ko` module.
4. Launches the userspace executable which parses the assembly, executes it, and finally runs the cache simulation on the collected memory access traces.
