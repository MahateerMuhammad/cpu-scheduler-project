cmd_/workspace/kernel_module/modules.order := {   echo /workspace/kernel_module/custom_scheduler.ko; :; } | awk '!x[$$0]++' - > /workspace/kernel_module/modules.order
