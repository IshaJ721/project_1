//
//  scheduler.cpp
//  Processor Scheduler
//
//  Created by ELMOOTAZBELLAH ELNOZAHY on 9/13/26.
//

#include <queue>
#include "scheduler.hpp"

#define NUM_CORES 8
#define NUM_SMALL_CORES 4
#define MIGRATION_THRESHOLD 0

enum CoreStatus {
    IDLE,
    READY,
    RUNNING,
    WAKING
};

std::queue<ProcessId_t> readyQ;
ProcessId_t running[NUM_CORES] = {InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId()};
ProcessId_t pending[NUM_CORES] = {InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId(), InvalidProcessId()};
CoreStatus coreStatus[NUM_CORES] = {READY, READY, READY, READY, READY, READY, READY, READY};

CPUId_t FindAvailableCore(ProcessId_t pid) {
    for(CPUId_t core = NUM_SMALL_CORES; core < NUM_CORES; core++) {
        if(coreStatus[core] == READY && pending[core] == InvalidProcessId()) {
            std::cout << " Process " + std::to_string(pid) + " Pending on Ready Core " + std::to_string(core) << std::endl;
            pending[core] = pid;
            return core;
        } else if(coreStatus[core] == IDLE && pending[core] == InvalidProcessId()) {
            std::cout << " Process " + std::to_string(pid) + " Pending on Idle Core " + std::to_string(core) << std::endl;
            pending[core] = pid;
            SetCState(core, C1);
            coreStatus[core] = WAKING;
            return core;
        }
    }
    for(CPUId_t core = 0; core < NUM_SMALL_CORES; core++) {
        if(coreStatus[core] == READY && pending[core] == InvalidProcessId()) {
            std::cout << " Process " + std::to_string(pid) + " Pending on Ready Core " + std::to_string(core) << std::endl;
            pending[core] = pid;
            return core;
        } else if(coreStatus[core] == IDLE && pending[core] == InvalidProcessId()) {
            std::cout << " Process " + std::to_string(pid) + " Pending on Idle Core " + std::to_string(core) << std::endl;
            pending[core] = pid;
            SetCState(core, C1);
            coreStatus[core] = WAKING;
            return core;
        }
    }
    std::cout << "No Available Core Found for Process " + std::to_string(pid) << std::endl;
    return NUM_CORES;
}

CPUId_t FindReadySmallCore(ProcessId_t pid) {
    for(CPUId_t core = NUM_SMALL_CORES; core < NUM_CORES; core++) {
        if(coreStatus[core] == READY && pending[core] == InvalidProcessId()) {
            std::cout << " Process " + std::to_string(pid) + " Pending on Small Core " + std::to_string(core) << std::endl;
            pending[core] = pid;
            return core;
        }
    }
    std::cout << "No Small Core Found for Process " + std::to_string(pid) << std::endl;
    return NUM_CORES;
}

CPUId_t FindProcessCore(ProcessId_t pid) {
    for(CPUId_t core = 0; core < NUM_CORES; core++) {
        if(running[core] == pid){
            return core;
        }
    }
    return NUM_CORES;
}

void CreateProcess(ProcessId_t pid) {
    // A new process has been created. Update the scheduler's data structures and decisions accordingly.
    std::cout << "CreateProcess(" + std::to_string(pid) + ")" << std::endl;
    SimOutput("CreateProcess(" + std::to_string(pid) + ")", 4);

    CPUId_t core = FindAvailableCore(pid);
    if(core != NUM_CORES) {
        if(coreStatus[core] == READY) {
            pending[core] = InvalidProcessId();
            running[core] = pid;
            coreStatus[core] = RUNNING;
            LoadContext(pid, core);
            RunCore(core);
            std::cout << "Running Process " + std::to_string(pid) + " on Core " + std::to_string(core) << std::endl;
        }
    } else {  // There is already a running process
        readyQ.push(pid);
        std::cout << "Adding Process " + std::to_string(pid) + " to Queue" << std::endl;
    }
}

void ExitProcess(ProcessId_t pid) {
    CPUId_t core = FindProcessCore(pid);
    if(core == NUM_CORES) {
        ThrowException("Process exited but was not assigned to a core");
    }
    std::cout << "Process " + std::to_string(pid) + " Finished on Core " + std::to_string(core) << std::endl;
    if(!readyQ.empty()){
        ProcessId_t next = readyQ.front();
        readyQ.pop();
        running[core] = next;
        LoadContext(next, core);
        RunCore(core);
        std::cout << "Loading Next Process " + std::to_string(next) + " on Core " + std::to_string(core) << std::endl;
    }
    else {
        running[core] = InvalidProcessId();
        if(core < NUM_SMALL_CORES) {
            coreStatus[core] = IDLE;
            SetCState(core, C6);
            std::cout << "Sleeping Core " + std::to_string(core) << std::endl;
        } else {
            coreStatus[core] = READY;
        }
    }
}

void TimerInterrupt(Time_t now) {
    CPUId_t core = 0;
    while(!readyQ.empty() && core != NUM_CORES) {
        ProcessId_t next = readyQ.front();
        core = FindAvailableCore(next);
        std::cout << "Checking Availability for Process " + std::to_string(next) << std::endl;
        if(core != NUM_CORES) {
            if(coreStatus[core] == READY) {
                pending[core] = InvalidProcessId();
                running[core] = next;
                coreStatus[core] = RUNNING;
                LoadContext(next, core);
                RunCore(core);
                std::cout << "Running Process " + std::to_string(next) + " on Core " + std::to_string(core) << std::endl;
            }
            readyQ.pop();
        }
    }

    // Migration Logic
    // CPUId_t big_core = 0;
    // while(core >= NUM_SMALL_CORES && core < NUM_CORES && big_core < NUM_SMALL_CORES) {
    //     std::cout << "Checking Migration for Process " + std::to_string(running[big_core]) + " with Time: " + FormatTime(GetRemaining(running[big_core])) << std::endl;
    //     if(GetRemaining(running[big_core]) > MIGRATION_THRESHOLD) {
    //         std::cout << "Trying to Migrate Process " + std::to_string(running[big_core]) + " with Time: " + FormatTime(GetRemaining(running[big_core])) << std::endl;
    //         core = FindReadySmallCore(running[big_core]);
    //         if(core != NUM_CORES) {
    //             std::cout << "Migrating Process " + std::to_string(running[big_core]) + " to Core " + std::to_string(core) << std::endl;
    //             coreStatus[big_core] = READY;
    //             coreStatus[core] = RUNNING;
    //             SaveContext(running[big_core], big_core);
    //             LoadContext(pending[core], core);
    //             RunCore(core);
    //             running[big_core] = InvalidProcessId();
    //             running[core] = pending[core];
    //             pending[core] = InvalidProcessId();
    //             big_core++;
    //         }
    //     }
    // }

    if(readyQ.empty()) {
        bool found_sleep = false;
        CPUId_t sleep_core = 7;
        while(sleep_core >= NUM_SMALL_CORES && !found_sleep) {
            if(coreStatus[sleep_core] == READY) {
                coreStatus[sleep_core] = IDLE;
                SetCState(sleep_core, C6);
                std::cout << "Sleeping Small Core " + std::to_string(sleep_core) << std::endl;
                found_sleep = true;
            }
            sleep_core--;
        }
    }
}

void CStateTransitionComplete(CPUId_t core_id){
    if(coreStatus[core_id] == WAKING) {
        std::cout << "Process " + std::to_string(pending[core_id]) + " Ready to Run on Transitioned Core " + std::to_string(core_id) << std::endl;
        SetPState(core_id, P0);
        LoadContext(pending[core_id], core_id);
        RunCore(core_id);
        coreStatus[core_id] = RUNNING;
        running[core_id] = pending[core_id];
        pending[core_id] = InvalidProcessId();
    }
}

void SimulationComplete(Time_t now) {
    // Add any bookkeeping or statistics that you would want to collect. Program terminates after this function returns.
    std::cout << "Run stopped at " << FormatTime(now) << " after consuming " << GetTotalEnergyConsumed()/3600000000.0 << " kWh" << std::endl;
    std::cout << "EDP: " << (now/3600000000.0) * (GetTotalEnergyConsumed()/3600000000.0) << std::endl;
}
