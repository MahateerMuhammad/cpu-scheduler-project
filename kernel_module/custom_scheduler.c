/*
 * Custom CPU Scheduler Kernel Module
 * Implements Priority + Aging scheduling algorithm
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/spinlock.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CPU Scheduler Team");
MODULE_DESCRIPTION("Custom CPU Scheduler with Priority + Aging");
MODULE_VERSION("1.2");

/* Scheduler parameters (modifiable via /proc) */
static int time_quantum_ms = 100;
static int aging_factor_sec = 5;

module_param(time_quantum_ms, int, 0644);
MODULE_PARM_DESC(time_quantum_ms, "Time quantum in milliseconds");

module_param(aging_factor_sec, int, 0644);
MODULE_PARM_DESC(aging_factor_sec, "Aging factor in seconds");

/* Process states */
enum proc_state {
    PROC_NEW = 0,
    PROC_READY,
    PROC_RUNNING,
    PROC_WAITING,
    PROC_TERMINATED
};

/* Custom Process Control Block */
struct custom_pcb {
    int pid;
    char name[32];
    int base_priority;          /* 0-10, lower is higher priority */
    int effective_priority;     /* After aging */
    int burst_time_ms;
    int remaining_time_ms;
    enum proc_state state;
    
    /* Timing statistics */
    unsigned long arrival_time_jiffies;
    unsigned long wait_time_ms;
    unsigned long turnaround_time_ms;
    unsigned long last_update_jiffies;
    unsigned long wakeup_time_jiffies; /* For waiting state */
    
    struct list_head list;      /* For ready queue */
    struct list_head global_list; /* For all_processes list */
};

/* Ready queue with spinlock protection */
struct ready_queue {
    struct list_head head;
    spinlock_t lock;
    int count;
};

/* Scheduler statistics */
struct sched_stats {
    unsigned long total_processes;
    unsigned long running_processes;
    unsigned long ready_processes;
    unsigned long waiting_processes;
    unsigned long terminated_processes;
    unsigned long context_switches;
    unsigned long cpu_utilization_percent;
    unsigned long avg_wait_time_ms;
    unsigned long avg_turnaround_time_ms;
    unsigned long total_cpu_time_ms;
    unsigned long total_idle_time_ms;
};

/* Global scheduler data */
static struct ready_queue ready_q;
static struct custom_pcb *current_proc = NULL;
static struct sched_stats stats;
static spinlock_t sched_lock;
static struct task_struct *scheduler_thread = NULL;
static bool scheduler_running = false;
static LIST_HEAD(all_processes);
static atomic_t next_pid = ATOMIC_INIT(1);  /* FIXED: Use atomic for thread safety */

/* Proc filesystem entry */
static struct proc_dir_entry *proc_entry = NULL;

/* Function prototypes */
static int scheduler_thread_fn(void *data);
static void apply_aging(void);
static struct custom_pcb *select_next_process(void);
static void update_statistics(void);
static void enqueue_process(struct custom_pcb *proc);
static struct custom_pcb *dequeue_process(void);
static void check_waiting_processes(void);

/*
 * Check for processes that should wake up from waiting
 */
static void check_waiting_processes(void)
{
    struct custom_pcb *proc;
    unsigned long flags;
    
    spin_lock_irqsave(&sched_lock, flags);
    
    list_for_each_entry(proc, &all_processes, global_list) {
        if (proc->state == PROC_WAITING) {
            if (time_after_eq(jiffies, proc->wakeup_time_jiffies)) {
                proc->state = PROC_READY;
                proc->last_update_jiffies = jiffies;
                enqueue_process(proc);
                pr_info("custom_scheduler: Process %d (%s) woke up\n", proc->pid, proc->name);
            }
        }
    }
    
    spin_unlock_irqrestore(&sched_lock, flags);
}

/*
 * Enqueue process to ready queue (priority-based insertion)
 */
static void enqueue_process(struct custom_pcb *proc)
{
    struct custom_pcb *p;
    struct list_head *pos;
    unsigned long flags;
    
    spin_lock_irqsave(&ready_q.lock, flags);
    
    /* Insert in priority order (lower effective_priority = higher priority) */
    list_for_each(pos, &ready_q.head) {
        p = list_entry(pos, struct custom_pcb, list);
        if (proc->effective_priority < p->effective_priority) {
            list_add_tail(&proc->list, pos);
            ready_q.count++;
            spin_unlock_irqrestore(&ready_q.lock, flags);
            return;
        }
    }
    
    /* Add to end if lowest priority */
    list_add_tail(&proc->list, &ready_q.head);
    ready_q.count++;
    
    spin_unlock_irqrestore(&ready_q.lock, flags);
}

/*
 * Dequeue highest priority process from ready queue
 */
static struct custom_pcb *dequeue_process(void)
{
    struct custom_pcb *proc = NULL;
    unsigned long flags;
    
    spin_lock_irqsave(&ready_q.lock, flags);
    
    if (!list_empty(&ready_q.head)) {
        proc = list_first_entry(&ready_q.head, struct custom_pcb, list);
        list_del(&proc->list);
        ready_q.count--;
    }
    
    spin_unlock_irqrestore(&ready_q.lock, flags);
    
    return proc;
}

/*
 * Apply aging to all processes in ready queue
 * Increases effective priority based on wait time
 */
static void apply_aging(void)
{
    struct custom_pcb *proc;
    unsigned long flags;
    unsigned long current_jiffies = jiffies;
    
    spin_lock_irqsave(&ready_q.lock, flags);
    
    list_for_each_entry(proc, &ready_q.head, list) {
        if (proc->state == PROC_READY) {
            /* Calculate wait time in seconds */
            unsigned long wait_sec = (current_jiffies - proc->last_update_jiffies) / HZ;
            
            /* Apply aging: effective_priority = base_priority - (wait_time / aging_factor) */
            /* Lower number = higher priority, so we subtract */
            if (aging_factor_sec > 0) {
                proc->effective_priority = proc->base_priority - (wait_sec / aging_factor_sec);
                if (proc->effective_priority < 0)
                    proc->effective_priority = 0;
            }
        }
    }
    
    spin_unlock_irqrestore(&ready_q.lock, flags);
}

/*
 * Select next process to run
 */
static struct custom_pcb *select_next_process(void)
{
    struct custom_pcb *proc;
    unsigned long flags;
    
    spin_lock_irqsave(&sched_lock, flags);
    
    if (current_proc && current_proc->state == PROC_RUNNING) {
        spin_unlock_irqrestore(&sched_lock, flags);
        return current_proc;
    }
    
    proc = dequeue_process();
    if (proc) {
        proc->state = PROC_RUNNING;
        current_proc = proc;
        stats.context_switches++;
    }
    
    spin_unlock_irqrestore(&sched_lock, flags);
    
    return proc;
}

/*
 * Update scheduler statistics
 */
static void update_statistics(void)
{
    struct custom_pcb *proc;
    unsigned long flags;
    unsigned long total_wait = 0, total_turnaround = 0;
    int count = 0;
    
    spin_lock_irqsave(&sched_lock, flags);
    
    stats.running_processes = 0;
    stats.ready_processes = 0;
    stats.waiting_processes = 0;
    stats.terminated_processes = 0;
    
    list_for_each_entry(proc, &all_processes, global_list) {
        count++;
        
        switch (proc->state) {
        case PROC_RUNNING:
            stats.running_processes++;
            break;
        case PROC_READY:
            stats.ready_processes++;
            break;
        case PROC_WAITING:
            stats.waiting_processes++;
            break;
        case PROC_TERMINATED:
            stats.terminated_processes++;
            break;
        default:
            break;
        }
        
        total_wait += proc->wait_time_ms;
        total_turnaround += proc->turnaround_time_ms;
    }
    
    stats.total_processes = count;
    
    if (count > 0) {
        stats.avg_wait_time_ms = total_wait / count;
        stats.avg_turnaround_time_ms = total_turnaround / count;
    }
    
    /* Calculate CPU utilization */
    if (stats.total_cpu_time_ms + stats.total_idle_time_ms > 0) {
        stats.cpu_utilization_percent = 
            (stats.total_cpu_time_ms * 100) / 
            (stats.total_cpu_time_ms + stats.total_idle_time_ms);
    }
    
    spin_unlock_irqrestore(&sched_lock, flags);
}

/*
 * Main scheduler thread
 */
static int scheduler_thread_fn(void *data)
{
    struct custom_pcb *proc;
    struct custom_pcb *p;
    unsigned long flags;
    int exec_time;
    
    pr_info("custom_scheduler: Scheduler thread started\n");
    
    while (!kthread_should_stop() && scheduler_running) {
        /* Check for waking processes */
        check_waiting_processes();

        /* Apply aging to prevent starvation */
        apply_aging();
        
        /* Select next process */
        proc = select_next_process();
        
        if (proc) {
            /* Simulate execution for time quantum */
            exec_time = min(time_quantum_ms, proc->remaining_time_ms);
            
            msleep(exec_time);
            
            spin_lock_irqsave(&sched_lock, flags);
            
            proc->remaining_time_ms -= exec_time;
            stats.total_cpu_time_ms += exec_time;
            
            /* Update wait time for ready processes */
            list_for_each_entry(p, &all_processes, global_list) {
                if (p->state == PROC_READY) {
                    p->wait_time_ms += exec_time;
                }
            }
            
            /* Check if process was moved to WAITING state (e.g. by user command) */
            if (proc->state == PROC_WAITING) {
                current_proc = NULL;
                pr_info("custom_scheduler: Process %d (%s) is waiting, not re-enqueuing\n", 
                        proc->pid, proc->name);
            }
            /* Check if process completed */
            else if (proc->remaining_time_ms <= 0) {
                proc->state = PROC_TERMINATED;
                proc->turnaround_time_ms = jiffies_to_msecs(jiffies - proc->arrival_time_jiffies);
                current_proc = NULL;
                pr_info("custom_scheduler: Process %d (%s) terminated\n", 
                        proc->pid, proc->name);
            } else {
                /* Preempt and re-enqueue */
                proc->state = PROC_READY;
                proc->last_update_jiffies = jiffies;
                enqueue_process(proc);
                current_proc = NULL;
            }
            
            spin_unlock_irqrestore(&sched_lock, flags);
        } else {
            /* No process to run, idle */
            stats.total_idle_time_ms += time_quantum_ms;
            msleep(time_quantum_ms);
        }
        
        /* Update statistics */
        update_statistics();
    }
    
    pr_info("custom_scheduler: Scheduler thread stopped\n");
    return 0;
}

/*
 * /proc/sched_stats show function
 */
static int sched_stats_show(struct seq_file *m, void *v)
{
    struct custom_pcb *proc;
    unsigned long flags;
    const char *state_str;
    
    seq_printf(m, "=== Custom CPU Scheduler Statistics ===\n\n");
    
    seq_printf(m, "Scheduler Parameters:\n");
    seq_printf(m, "  Time Quantum: %d ms\n", time_quantum_ms);
    seq_printf(m, "  Aging Factor: %d seconds\n\n", aging_factor_sec);
    
    seq_printf(m, "Process Counts:\n");
    seq_printf(m, "  Total Processes: %lu\n", stats.total_processes);
    seq_printf(m, "  Running: %lu\n", stats.running_processes);
    seq_printf(m, "  Ready: %lu\n", stats.ready_processes);
    seq_printf(m, "  Waiting: %lu\n", stats.waiting_processes);
    seq_printf(m, "  Terminated: %lu\n\n", stats.terminated_processes);
    
    seq_printf(m, "Performance Metrics:\n");
    seq_printf(m, "  CPU Utilization: %lu%%\n", stats.cpu_utilization_percent);
    seq_printf(m, "  Context Switches: %lu\n", stats.context_switches);
    seq_printf(m, "  Avg Wait Time: %lu ms\n", stats.avg_wait_time_ms);
    seq_printf(m, "  Avg Turnaround Time: %lu ms\n\n", stats.avg_turnaround_time_ms);
    
    seq_printf(m, "Process Table:\n");
    seq_printf(m, "%-6s %-20s %-10s %-8s %-8s %-10s %-10s\n",
               "PID", "Name", "State", "BasePri", "EffPri", "Remaining", "WaitTime");
    seq_printf(m, "--------------------------------------------------------------------\n");
    
    spin_lock_irqsave(&sched_lock, flags);
    
    if (list_empty(&all_processes)) {
        seq_printf(m, "No processes in list\n");
    } else {
        list_for_each_entry(proc, &all_processes, global_list) {
            switch (proc->state) {
            case PROC_NEW:        state_str = "NEW"; break;
            case PROC_READY:      state_str = "READY"; break;
            case PROC_RUNNING:    state_str = "RUNNING"; break;
            case PROC_WAITING:    state_str = "WAITING"; break;
            case PROC_TERMINATED: state_str = "TERM"; break;
            default:              state_str = "UNKNOWN"; break;
            }
            
            seq_printf(m, "%-6d %-20s %-10s %-8d %-8d %-10d %-10lu\n",
                       proc->pid, proc->name, state_str,
                       proc->base_priority, proc->effective_priority,
                       proc->remaining_time_ms, proc->wait_time_ms);
        }
    }
    
    spin_unlock_irqrestore(&sched_lock, flags);
    
    return 0;
}

static int sched_stats_open(struct inode *inode, struct file *file)
{
    return single_open(file, sched_stats_show, NULL);
}

/*
 * /proc/sched_stats write function
 * Format: "NEW <name> <burst_time> <priority>"
 * Example: "NEW proc1 1000 5"
 */
static ssize_t sched_stats_write(struct file *file, const char __user *buffer,
                               size_t count, loff_t *ppos)
{
    char kbuffer[128];
    char cmd[8];
    char name[32];
    int burst_time, priority;
    struct custom_pcb *proc;
    unsigned long flags;
    
    if (count > sizeof(kbuffer) - 1)
        return -EINVAL;
        
    if (copy_from_user(kbuffer, buffer, count))
        return -EFAULT;
        
    kbuffer[count] = '\0';
    
    /* Parse command */
    if (sscanf(kbuffer, "%7s", cmd) != 1)
        return -EINVAL;
        
    if (strcmp(cmd, "NEW") == 0) {
        if (sscanf(kbuffer, "%*s %31s %d %d", name, &burst_time, &priority) != 3) {
            pr_info("custom_scheduler: Invalid NEW format. Buffer: '%s'\n", kbuffer);
            return -EINVAL;
        }
        
        /* Validate parameters */
        if (burst_time <= 0 || priority < 0 || priority > 10) {
            pr_info("custom_scheduler: Invalid parameters. Burst > 0, Priority 0-10\n");
            return -EINVAL;
        }
        
        /* Create new process */
        proc = kmalloc(sizeof(struct custom_pcb), GFP_KERNEL);
        if (!proc)
            return -ENOMEM;
        
        memset(proc, 0, sizeof(struct custom_pcb));
            
        proc->pid = atomic_inc_return(&next_pid);
        strncpy(proc->name, name, 31);
        proc->name[31] = '\0';
        proc->base_priority = priority;
        proc->effective_priority = priority;
        proc->burst_time_ms = burst_time;
        proc->remaining_time_ms = burst_time;
        proc->state = PROC_READY;
        
        proc->arrival_time_jiffies = jiffies;
        proc->last_update_jiffies = jiffies;
        proc->wait_time_ms = 0;
        proc->turnaround_time_ms = 0;
        
        INIT_LIST_HEAD(&proc->list);
        INIT_LIST_HEAD(&proc->global_list);
        
        /* Add to lists */
        spin_lock_irqsave(&sched_lock, flags);
        list_add_tail(&proc->global_list, &all_processes);
        spin_unlock_irqrestore(&sched_lock, flags);
        
        enqueue_process(proc);
        
        pr_info("custom_scheduler: Created process %d (%s) with burst %d ms, prio %d\n",
                proc->pid, proc->name, burst_time, priority);
                
    } else if (strcmp(cmd, "WAIT") == 0) {
        int pid, wait_ms;
        struct custom_pcb *p;
        bool found = false;
        
        if (sscanf(kbuffer, "%*s %d %d", &pid, &wait_ms) != 2) {
            pr_info("custom_scheduler: Invalid WAIT format. Use: WAIT <pid> <ms>\n");
            return -EINVAL;
        }
        
        spin_lock_irqsave(&sched_lock, flags);
        list_for_each_entry(p, &all_processes, global_list) {
            if (p->pid == pid) {
                if (p->state == PROC_RUNNING || p->state == PROC_READY) {
                    /* If running, preempt it */
                    if (p->state == PROC_RUNNING) {
                        current_proc = NULL;
                    } else {
                        /* If ready, remove from ready queue */
                        unsigned long rq_flags;
                        spin_lock_irqsave(&ready_q.lock, rq_flags);
                        list_del(&p->list);
                        ready_q.count--;
                        spin_unlock_irqrestore(&ready_q.lock, rq_flags);
                    }
                    
                    p->state = PROC_WAITING;
                    p->wakeup_time_jiffies = jiffies + msecs_to_jiffies(wait_ms);
                    pr_info("custom_scheduler: Process %d put to sleep for %d ms\n", pid, wait_ms);
                } else {
                    pr_info("custom_scheduler: Process %d is not RUNNING or READY\n", pid);
                }
                found = true;
                break;
            }
        }
        spin_unlock_irqrestore(&sched_lock, flags);
        
        if (!found)
            pr_info("custom_scheduler: Process %d not found\n", pid);
            
    } else {
        pr_info("custom_scheduler: Unknown command: %s\n", cmd);
        return -EINVAL;
    }
            
    return count;
}

static const struct proc_ops sched_stats_fops = {
    .proc_open    = sched_stats_open,
    .proc_read    = seq_read,
    .proc_write   = sched_stats_write,
    .proc_lseek   = seq_lseek,
    .proc_release = single_release,
};

/*
 * Module initialization
 */
static int __init custom_scheduler_init(void)
{
    pr_info("custom_scheduler: Initializing Custom CPU Scheduler module v1.2\n");
    
    /* Initialize ready queue */
    INIT_LIST_HEAD(&ready_q.head);
    spin_lock_init(&ready_q.lock);
    ready_q.count = 0;
    
    /* Initialize global process list explicitly */
    INIT_LIST_HEAD(&all_processes);
    
    /* Initialize scheduler lock */
    spin_lock_init(&sched_lock);
    
    /* Initialize statistics */
    memset(&stats, 0, sizeof(stats));
    
    /* Create /proc/sched_stats entry with write permissions */
    proc_entry = proc_create("sched_stats", 0666, NULL, &sched_stats_fops);  /* FIXED: Changed to 0666 */
    if (!proc_entry) {
        pr_err("custom_scheduler: Failed to create /proc/sched_stats\n");
        return -ENOMEM;
    }
    
    pr_info("custom_scheduler: Created /proc/sched_stats\n");
    
    /* Start scheduler thread */
    scheduler_running = true;
    scheduler_thread = kthread_run(scheduler_thread_fn, NULL, "custom_sched");
    if (IS_ERR(scheduler_thread)) {
        pr_err("custom_scheduler: Failed to create scheduler thread\n");
        proc_remove(proc_entry);
        return PTR_ERR(scheduler_thread);
    }
    
    pr_info("custom_scheduler: Module loaded successfully\n");
    pr_info("custom_scheduler: Time quantum = %d ms, Aging factor = %d sec\n",
            time_quantum_ms, aging_factor_sec);
    
    return 0;
}

/*
 * Module cleanup
 */
static void __exit custom_scheduler_exit(void)
{
    struct custom_pcb *proc, *tmp;
    unsigned long flags;
    
    pr_info("custom_scheduler: Cleaning up module\n");
    
    /* Stop scheduler thread */
    scheduler_running = false;
    if (scheduler_thread) {
        kthread_stop(scheduler_thread);
        pr_info("custom_scheduler: Scheduler thread stopped\n");
    }
    
    /* Remove /proc entry */
    if (proc_entry) {
        proc_remove(proc_entry);
        pr_info("custom_scheduler: Removed /proc/sched_stats\n");
    }
    
    /* Free all process structures */
    spin_lock_irqsave(&sched_lock, flags);
    
    list_for_each_entry_safe(proc, tmp, &all_processes, global_list) {
        list_del(&proc->global_list);
        kfree(proc);
    }
    
    spin_unlock_irqrestore(&sched_lock, flags);
    
    pr_info("custom_scheduler: Module unloaded successfully\n");
}

module_init(custom_scheduler_init);
module_exit(custom_scheduler_exit);