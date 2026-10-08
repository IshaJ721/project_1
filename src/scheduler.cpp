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

CPUId_t FindReadyCore(ProcessId_t pid) {
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

    CPUId_t core = FindReadyCore(pid);
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
        coreStatus[core] = IDLE;
        SetCState(core, C6);
        std::cout << "Sleeping Core " + std::to_string(core) << std::endl;
    }
}

void TimerInterrupt(Time_t now) {
    CPUId_t core = 0;
    while(!readyQ.empty() && core != NUM_CORES) {
        ProcessId_t next = readyQ.front();
        core = FindReadyCore(next);
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
}
