# CPU Scheduler with Qt GUI

A kernel-level CPU scheduler simulator featuring **Priority Scheduling with Aging** algorithm and a comprehensive Qt6 graphical interface for real-time visualization and control.

## Features

- **Priority-based Preemptive Scheduling** with configurable time quantum
- **Aging Mechanism** to prevent process starvation
- **Real-time Qt6 GUI** with:
  - Color-coded process table (states: NEW, READY, RUNNING, WAITING, TERMINATED)
  - Live statistics dashboard (CPU utilization, wait times, context switches)
  - Dynamic process management (add/terminate processes)
  - Configurable scheduler parameters
  - Event logging system
- **Thread-safe** implementation using spinlocks
- **Detailed statistics** tracking and logging

## Architecture

### Core Components

```
cpu-scheduler-project/
├── src/
│   ├── kernel/           # Core scheduling logic
│   │   ├── process.h/cpp        # Process Control Block (PCB)
│   │   ├── scheduler.h/cpp      # Main scheduler with priority + aging
│   │   ├── ready_queue.h/cpp    # Priority queue for ready processes
│   │   └── spinlock.h           # Spinlock synchronization primitive
│   ├── gui/              # Qt6 GUI components
│   │   ├── mainwindow.h/cpp     # Main application window
│   │   ├── process_table_widget.h/cpp  # Process display table
│   │   └── stats_widget.h/cpp   # Statistics panel
│   ├── utils/            # Utilities
│   │   └── logger.h/cpp         # Thread-safe logging facility
│   └── main.cpp          # Application entry point
├── build/                # Build output directory
├── docs/                 # Documentation
├── CMakeLists.txt        # Build configuration
└── README.md             # This file
```

### Scheduling Algorithm

**Priority Scheduling with Aging:**

1. **Priority Queue**: Processes ordered by effective priority (0 = highest)
2. **Aging**: `effective_priority = base_priority + (wait_time / aging_factor)`
3. **Preemption**: Time quantum-based (default 100ms)
4. **Starvation Prevention**: Waiting processes get priority boost over time

### Process States

- **NEW** → **READY** → **RUNNING** → **TERMINATED**
- **WAITING** state for simulated I/O operations

## Requirements

### macOS (Native Build)

- macOS 10.15 or later
- Xcode Command Line Tools
- Homebrew
- Qt6
- CMake

### Linux/Docker

- Docker Desktop (for macOS users)
- Ubuntu-based image with Qt6 and build tools

## Installation

### macOS Native

1. **Install dependencies:**
   ```bash
   # Install Homebrew (if not already installed)
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   
   # Install CMake and Qt6
   brew install cmake qt@6
   ```

2. **Clone/navigate to project:**
   ```bash
   cd /Users/apple/Desktop/cpu-scheduler-project
   ```

3. **Build:**
   ```bash
   mkdir -p build && cd build
   cmake ..
   make -j4
   ```

4. **Run:**
   ```bash
   ./cpu_scheduler
   ```

### Docker (Alternative)

1. **Build Docker image:**
   ```bash
   docker build -t cpu-scheduler .
   ```

2. **Run with X11 forwarding:**
   ```bash
   # On macOS, install XQuartz first
   brew install --cask xquartz
   
   # Allow X11 connections
   xhost + localhost
   
   # Run container
   docker run -e DISPLAY=host.docker.internal:0 cpu-scheduler
   ```

## Usage

### GUI Controls

**Control Panel:**
- **Start Scheduler**: Begin scheduling processes
- **Pause/Resume**: Pause/resume operation
- **Stop**: Stop scheduler and reset
- **Add Process**: Create new process (name, priority, burst time)
- **Kill Selected**: Terminate selected process

**Configuration:**
- **Time Quantum**: Time slice per process (10-1000ms)
- **Aging Factor**: Seconds per priority level boost (1-60)
- **Apply**: Apply configuration changes

**Process Table:**
- Real-time view of all processes
- Columns: PID, Name, State, Priority, Remaining Time, Wait Time
- Color-coded by state:
  - 🟦 NEW (light blue)
  - 🟨 READY (yellow)
  - 🟩 RUNNING (green)
  - 🟥 WAITING (red)
  - ⬜ TERMINATED (gray)

**Statistics Panel:**
- Total/Running/Ready/Waiting/Terminated process counts
- CPU Utilization percentage
- Context switch count
- Average wait and turnaround times

### Example Workflow

1. **Start the application**
2. **Add processes** with varying priorities:
   - Process A: Priority 1, Burst 500ms (high priority)
   - Process B: Priority 5, Burst 1000ms (medium priority)
   - Process C: Priority 9, Burst 300ms (low priority)
3. **Configure scheduler**:
   - Time Quantum: 100ms
   - Aging Factor: 5 seconds
4. **Start scheduler** and observe:
   - Process A runs first (highest priority)
   - Process C gets priority boost over time (aging)
   - Statistics update in real-time

## Testing Scenarios

### 1. Basic Scheduling
Create 5 processes with different priorities and verify execution order.

### 2. Aging Verification
Create low-priority process, then add high-priority processes later. Verify low-priority process eventually executes.

### 3. Preemption Test
Create long-running process with 100ms quantum. Verify preemption occurs.

### 4. Stress Test
Rapidly create 50+ processes and verify scheduler handles load without crashes.

## Build Configuration

**CMake Options:**
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..    # Release build
cmake -DCMAKE_BUILD_TYPE=Debug ..      # Debug build with symbols
```

**VS Code Tasks:**
- `Cmd+Shift+B`: Build project
- `Cmd+Shift+P` → "Run Task" → "Run": Build and run

## Logs

Scheduler events are logged to:
- **Console**: Real-time stdout
- **File**: `sched_stats.log` (timestamped entries)
- **GUI**: Log viewer panel in application

## Troubleshooting

**Qt6 not found:**
```bash
# Ensure Qt6 is in CMake path
export CMAKE_PREFIX_PATH="/usr/local/opt/qt@6"
cmake ..
```

**Build errors:**
```bash
# Clean build
rm -rf build
mkdir build && cd build
cmake .. && make clean && make
```

**GUI doesn't appear:**
- macOS: Ensure XQuartz is running if using Docker
- Check console for Qt library errors

## Performance

- Supports 100+ concurrent processes
- Sub-millisecond scheduling decisions
- Real-time GUI updates at 10 FPS (100ms intervals)
- Thread-safe multi-core execution

## Development

**Code Structure:**
- C++17 standard
- Qt6 for GUI (Widgets module)
- CMake build system
- Header/implementation separation

**Key Design Patterns:**
- Singleton (Logger)
- Observer (Stats callbacks)
- RAII (SpinlockGuard)
- Priority Queue (Ready queue)

## License

Educational/Academic project for CPU scheduler demonstration.

## Authors

CPU Scheduler Project Team

## Acknowledgments

- Linux kernel scheduling concepts
- Qt6 documentation and examples
- Modern C++ best practices
