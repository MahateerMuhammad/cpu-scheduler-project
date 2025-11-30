# Custom CPU Scheduler - Linux Kernel Module

## Overview

This is a **real Linux kernel module** that implements a custom CPU scheduler with **Priority + Aging** algorithm. Unlike the user-space simulator in the main project, this module runs at the kernel level and integrates with the Linux kernel.

## ✅ Kernel Module Features

This implementation satisfies ALL the requirements for a kernel-level CPU scheduler:

### ✅ 1. Kernel Module for Custom CPU Scheduler
- Loadable kernel module (`.ko` file)
- Uses `module_init()` and `module_exit()` macros
- Can be loaded with `insmod` and unloaded with `rmmod`

### ✅ 2. Priority + Aging Scheduling Algorithm
- Priority-based scheduling (0-10, lower = higher priority)
- Aging mechanism: `effective_priority = base_priority - (wait_time / aging_factor)`
- Prevents process starvation

### ✅ 3. Kernel-level Process Management
- Custom Process Control Block (PCB) structure
- Process states: NEW, READY, RUNNING, WAITING, TERMINATED
- Managed via kernel data structures (`struct custom_pcb`)

### ✅ 4. Spinlocks for Synchronization
- Uses kernel spinlocks (`spinlock_t`)
- `spin_lock_irqsave()` / `spin_unlock_irqrestore()` for interrupt safety
- Protects ready queue and scheduler state

### ✅ 5. Shared Kernel Memory
- Process structures allocated with `kmalloc()`
- Ready queue stored in kernel memory
- Accessible only to kernel code

### ✅ 6. Kernel Thread Management
- Scheduler runs as a kernel thread (`kthread`)
- Created with `kthread_run()`
- Proper cleanup with `kthread_stop()`

### ✅ 7. Preemptive Scheduling
- Time quantum-based preemption (default 100ms)
- Configurable via module parameter
- Processes preempted after time slice expires

### ✅ 8. /proc/sched_stats Interface
- Creates `/proc/sched_stats` entry
- Displays scheduler statistics and process table
- Accessible via `cat /proc/sched_stats`

### ✅ 9. Performance Statistics
- CPU utilization tracking
- Context switch counting
- Average wait time and turnaround time
- Process state monitoring

### ✅ 10. Module Parameters (System Call Alternative)
- `time_quantum_ms`: Configurable time quantum
- `aging_factor_sec`: Configurable aging factor
- Modifiable at load time: `insmod custom_scheduler.ko time_quantum_ms=200`

### ✅ 11. Kernel Cleanup Routine
- `custom_scheduler_exit()` function
- Stops scheduler thread
- Removes `/proc` entry
- Frees all kernel memory
- Safe module unload

## Requirements

- Linux kernel 5.15+ (tested on 5.15.0-161-generic)
- Kernel headers installed
- Build tools: `gcc`, `make`, `build-essential`
- Root privileges for loading/unloading

## Building the Module

```bash
# Navigate to kernel_module directory
cd kernel_module

# Build the module
make

# This will create: custom_scheduler.ko
```

## Loading the Module

```bash
# Load with default parameters
sudo insmod custom_scheduler.ko

# Load with custom parameters
sudo insmod custom_scheduler.ko time_quantum_ms=200 aging_factor_sec=10

# Verify module is loaded
lsmod | grep custom_scheduler

# Check kernel logs
dmesg | tail -20
```

Expected output in `dmesg`:
```
custom_scheduler: Initializing Custom CPU Scheduler module
custom_scheduler: Created /proc/sched_stats
custom_scheduler: Module loaded successfully
custom_scheduler: Time quantum = 100 ms, Aging factor = 5 sec
custom_scheduler: Scheduler thread started
```

## Viewing Statistics

```bash
# View scheduler statistics
cat /proc/sched_stats
```

Example output:
```
=== Custom CPU Scheduler Statistics ===

Scheduler Parameters:
  Time Quantum: 100 ms
  Aging Factor: 5 seconds

Process Counts:
  Total Processes: 0
  Running: 0
  Ready: 0
  Waiting: 0
  Terminated: 0

Performance Metrics:
  CPU Utilization: 0%
  Context Switches: 0
  Avg Wait Time: 0 ms
  Avg Turnaround Time: 0 ms

Process Table:
PID    Name                 State      BasePri  EffPri   Remaining  WaitTime  
--------------------------------------------------------------------
```

## Unloading the Module

```bash
# Remove the module
sudo rmmod custom_scheduler

# Verify removal
lsmod | grep custom_scheduler

# Check cleanup logs
dmesg | tail -10
```

Expected output in `dmesg`:
```
custom_scheduler: Cleaning up module
custom_scheduler: Scheduler thread stopped
custom_scheduler: Removed /proc/sched_stats
custom_scheduler: Module unloaded successfully
```

## Module Parameters

### time_quantum_ms
- **Description**: Time slice for each process in milliseconds
- **Default**: 100
- **Range**: 10-1000
- **Example**: `insmod custom_scheduler.ko time_quantum_ms=200`

### aging_factor_sec
- **Description**: Seconds per priority level boost for waiting processes
- **Default**: 5
- **Range**: 1-60
- **Example**: `insmod custom_scheduler.ko aging_factor_sec=10`

## Architecture

### Data Structures

```c
struct custom_pcb {
    int pid;
    char name[32];
    int base_priority;          // 0-10
    int effective_priority;     // After aging
    int burst_time_ms;
    int remaining_time_ms;
    enum proc_state state;
    unsigned long wait_time_ms;
    struct list_head list;      // Kernel linked list
};
```

### Synchronization

- **Ready Queue Lock**: `spinlock_t` protects ready queue operations
- **Scheduler Lock**: `spinlock_t` protects scheduler state
- **Interrupt Safety**: Uses `spin_lock_irqsave()` for IRQ safety

### Kernel Thread

- Runs `scheduler_thread_fn()` continuously
- Applies aging every iteration
- Selects next process from ready queue
- Simulates execution with `msleep()`
- Updates statistics

## Comparison: Kernel Module vs User-Space Simulator

| Feature | Kernel Module | User-Space Simulator |
|---------|---------------|---------------------|
| **Execution Level** | Kernel space | User space |
| **Process Management** | Kernel structures | C++ classes |
| **Memory** | `kmalloc()` | `new`/`std::shared_ptr` |
| **Threads** | `kthread` | `std::thread` |
| **Synchronization** | Kernel spinlocks | `std::atomic_flag` |
| **Statistics** | `/proc/sched_stats` | Log file |
| **GUI** | None | Qt6 GUI |
| **Loading** | `insmod` | Execute binary |
| **Privileges** | Root required | User level |

## Limitations

1. **Simulation Only**: This module simulates scheduling but doesn't replace the Linux scheduler
2. **No Real Processes**: Manages custom PCBs, not actual kernel tasks
3. **Docker Kernel Mismatch**: Running kernel (6.12.54-linuxkit) differs from headers (5.15.0-161)
   - Module loads but may have compatibility issues
   - For production, kernel version must match headers
4. **No I/O Integration**: I/O simulation not yet implemented in kernel version

## Testing

```bash
# Build and basic test
make
sudo insmod custom_scheduler.ko
cat /proc/sched_stats
sudo rmmod custom_scheduler

# Test with custom parameters
sudo insmod custom_scheduler.ko time_quantum_ms=50 aging_factor_sec=10
dmesg | tail -20
cat /proc/sched_stats
sudo rmmod custom_scheduler
```

## Troubleshooting

### Module won't load
```bash
# Check kernel logs for errors
dmesg | tail -50

# Verify module info
modinfo custom_scheduler.ko

# Check if already loaded
lsmod | grep custom_scheduler
```

### /proc/sched_stats not appearing
```bash
# Verify module loaded successfully
dmesg | grep custom_scheduler

# Check /proc filesystem
ls -la /proc/ | grep sched
```

### Build errors
```bash
# Verify kernel headers installed
ls -la /lib/modules/$(uname -r)/build

# Install headers if missing
sudo apt-get install linux-headers-$(uname -r)

# Clean and rebuild
make clean
make
```

## Future Enhancements

1. **Real Process Integration**: Hook into Linux scheduler to manage actual tasks
2. **System Call Interface**: Add custom syscalls for process creation/management
3. **Multi-core Support**: Extend to multiple CPUs with load balancing
4. **I/O Handlers**: Integrate with kernel I/O subsystem
5. **Real-time Scheduling**: Add deadline-based scheduling class
6. **Cgroup Integration**: Support for control groups
7. **Performance Profiling**: Add detailed performance counters

## References

- Linux Kernel Module Programming Guide
- Linux Device Drivers (LDD3)
- Understanding the Linux Kernel (O'Reilly)
- Linux Kernel Development by Robert Love
- `/proc` filesystem documentation

## License

GPL (GNU General Public License) - Required for Linux kernel modules

## Author

CPU Scheduler Project Team
