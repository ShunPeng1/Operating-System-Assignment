Thuan: When run testcase in /input remember to change the #define in /include/os-cfg.h or it will not run
    Deactivate #define MM_FIXED_MEMSZ:
    ./os sched
    ./os sched_0
    ./os sched_1
    ./os os_1_singleCPU_mlq

    Activate #define MM_FIXED_MEMSZ: other testcases
