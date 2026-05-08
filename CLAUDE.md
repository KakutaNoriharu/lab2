# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

C++ discrete-time simulation for **Information Floating (IF) attack countermeasures** — modeling DTN/vehicular networks where malicious nodes inject false information and defense methods based on route history are evaluated.

## Build & Run

```bash
cd main
make          # produces a.out
./a.out       # run with hardcoded defaults
```

**Compiler**: `g++ -I../utility/ -Wall -Wextra -pedantic -Wshadow -Wstrict-aliasing -Wconversion -Wunused-result -lm -pthread -g -O2`

**CLI options**:
- `-p <file>` — load `.prm` parameter file (enables multi-param sweep mode)
- `-g <0|1|2>` — gnuplot output: 0=none, 1=GIF, 2=script
- `-pb` — show progress bar
- `-s <int>` — random seed
- `-1` to `-4` — method-specific parameters (md_v1–md_v4)

**Batch run** (30 seeds, fixed params):
```bash
./loop.sh
```

**Test build**:
```bash
g++ -I../utility/ test.cpp -o test.out -lm -pthread && ./test.out
```
Test loads `params/test.prm`, runs 10,000 steps, writes `test-log.log`.

## High-Level Architecture

The simulation runs a fixed-step loop (default 50,000 steps = seconds) using the following phase order each step:

```
appear() → remove() → move() → observe() → broadcast() → check()
```

### Core Components (`main/`)

| File | Role |
|------|------|
| `main.cpp` | Entry point; argument parsing, threading (default 4 threads), parameter sweep, event scheduling |
| `simulation.hpp` | Core loop: node lifecycle, communication, statistics |
| `simulation_plot.hpp` | Extends `Simulation` with gnuplot rendering |
| `method3.hpp` | Defense method implementations (trust evaluation logic) — largest file |
| `node.hpp` | Node class; delegates movement and trust to `Movement` and method objects |
| `parameters.hpp` | All global simulation parameters; structured types for TA/RA zones |
| `map.hpp` | Grid road network; creates edges and routing tables |
| `edge.hpp` | `EdgeRouting` — per-edge directional routing probabilities |
| `floatingInformation.hpp` | Information structures: TRUE/FAKE flows, receive events, road traversal records |
| `store.hpp` | Template accumulator of per-node communication history (`GlobalStore`) |
| `simulation_event.hpp` | Scheduled event hierarchy (node arrival triggers, attack start) |
| `area.hpp` | Rectangular TA/RA zone definitions |
| `movement.hpp` | Point-to-point movement with distance tracking |

### Utilities (`utility/`)

| File | Role |
|------|------|
| `parameterManager.hpp` | Thread-safe Cartesian product sweep over `.prm` parameter sets |
| `parameterLoader.hpp` | `.prm` file parser |
| `logger.h` | Logging |
| `progress.hpp` | Progress bar and timer |
| `gnuplot.h` | GnuPlot command generation |
| `fileWrapper.h`, `fileManager.h` | File I/O helpers |

### Information Model

- Two concurrent information flows: **TRUE** (info_id=0) and **FAKE** (info_id=1)
- **TA** (Target Area): geographic zone where information is relevant
- **RA** (Related Area): secondary zone affecting node behavior
- Node types: `NORMAL`, `MALICIOUS` (attacker), `FIXED_SOURCE`

### Parameter Files (`.prm`)

Stored in `main/params/`. Key fields:

```
simulation_loop:50000;
field_length:(4000.0,4000.0);
Nxy:(1,0);          # road grid size
lambda:0.01;        # Poisson node arrival rate [s^-1]
Vm:1.0;             # node velocity [m/s]
Cr:20.0;            # connection range [m]
LTA:{...};          # true information Target Areas
LfTA:{...};         # fake information Target Areas
LRA:;               # Related Areas
```

### Execution Flow

1. Load parameters → `ParameterManager` generates sweep combinations
2. Worker threads each call `run_simulation()`:
   - 50,000 warmup steps (skipped from recording)
   - 50,000 recorded steps with event schedule
3. Output: CSV to `data/<result_id>/sim_<param_string>.csv`; road probability tables to `prob_data/`

### Important Conventions

- **Node IDs**: incremental integers, unique per simulation run
- **Thread safety**: `ParameterManager` uses mutex; `GlobalStore` is write-once during simulation
- **Gnuplot mode**: forces single-threaded execution (`-g` disables multi-threading)
- **Japanese**: source comments and documentation are in Japanese
- **Memory**: map vectors are pre-allocated to avoid pointer invalidation
