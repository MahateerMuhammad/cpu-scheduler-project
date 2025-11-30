# Quick Start Guide - Kernel Module Testing

## Prerequisites

You have a Linux Docker container running with kernel headers installed:
```bash
docker start linux-dev
docker exec -it linux-dev bash
```

## Step 1: Navigate to Kernel Module Directory

```bash
cd /workspace/kernel_module
```

## Step 2: Build the Module

```bash
make clean
make
```

**Expected Output**:
```
CC [M]  custom_scheduler.o
MODPOST Module.symvers
LD [M]  custom_scheduler.ko
```

**Result**: `custom_scheduler.ko` file created (237KB)

## Step 3: Load the Module

```bash
insmod custom_scheduler.ko
```

**Verify Loading**:
```bash
lsmod | grep custom_scheduler
dmesg | tail -10
```

**Expected dmesg Output**:
```
custom_scheduler: Initializing Custom CPU Scheduler module
custom_scheduler: Created /proc/sched_stats
custom_scheduler: Module loaded successfully
custom_scheduler: Time quantum = 100 ms, Aging factor = 5 sec
custom_scheduler: Scheduler thread started
```

## Step 4: View Statistics

```bash
cat /proc/sched_stats
```

**Expected Output**:
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

## Step 5: Monitor Scheduler Activity

```bash
# Watch statistics in real-time
watch -n 1 cat /proc/sched_stats

# Or check kernel logs
dmesg -w
```

## Step 6: Unload the Module

```bash
rmmod custom_scheduler
```

**Verify Unloading**:
```bash
lsmod | grep custom_scheduler  # Should return nothing
dmesg | tail -10
```

**Expected dmesg Output**:
```
custom_scheduler: Cleaning up module
custom_scheduler: Scheduler thread stopped
custom_scheduler: Removed /proc/sched_stats
custom_scheduler: Module unloaded successfully
```

## Advanced: Load with Custom Parameters

```bash
# Load with 200ms time quantum and 10 second aging factor
insmod custom_scheduler.ko time_quantum_ms=200 aging_factor_sec=10

# Verify parameters
cat /proc/sched_stats | head -10
```

## Troubleshooting

### Module already loaded
```bash
rmmod custom_scheduler
insmod custom_scheduler.ko
```

### Permission denied
```bash
# You may need to run as root in the container
# The container should already have root access
whoami  # Should show 'root'
```

### /proc/sched_stats not found
```bash
# Check if module loaded successfully
lsmod | grep custom_scheduler
dmesg | grep custom_scheduler

# Verify /proc entry
ls -la /proc/ | grep sched
```

### Build errors
```bash
# Verify kernel headers
ls -la /lib/modules/5.15.0-161-generic/build

# Clean and rebuild
make clean
make
```

## Complete Test Sequence

```bash
# Full automated test
cd /workspace/kernel_module

# Build
make clean && make

# Load
insmod custom_scheduler.ko

# Wait a bit
sleep 3

# View stats
cat /proc/sched_stats

# Check logs
dmesg | tail -20

# Unload
rmmod custom_scheduler

# Verify cleanup
dmesg | tail -10
```

## What to Expect

Since this is a scheduler simulation without actual processes being created, you'll see:
- ✅ Module loads successfully
- ✅ `/proc/sched_stats` is created
- ✅ Scheduler thread runs in background
- ✅ Statistics are available (mostly zeros without processes)
- ✅ Module unloads cleanly

## Next Steps

To see the scheduler in action, you would need to:
1. Add process creation functionality
2. Create test processes via module interface
3. Watch them being scheduled
4. Monitor statistics changing in real-time

This would require additional development to add process creation mechanisms (e.g., via `/proc` write interface or ioctl).

## Success Criteria

✅ Module compiles without errors  
✅ Module loads without kernel panic  
✅ `/proc/sched_stats` appears and is readable  
✅ Kernel logs show proper initialization  
✅ Module unloads cleanly  
✅ No memory leaks (check dmesg for warnings)  

---

**Status**: All basic functionality working! ✅
