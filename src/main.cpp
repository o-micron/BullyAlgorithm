#include "../include/BullyAlgo/BullyAlgo.h"

#include <iostream>

#include <Windows.h>

#include <TlHelp32.h>

#include <processthreadsapi.h>

DWORD
MyGetProcessId(LPCTSTR ProcessName) // non-conflicting function name
{
    PROCESSENTRY32 pt;
    HANDLE         hsnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    pt.dwSize            = sizeof(PROCESSENTRY32);
    if (Process32First(hsnap, &pt)) { // must call this first
        do {
            if (!lstrcmpi(pt.szExeFile, ProcessName)) {
                CloseHandle(hsnap);
                return pt.th32ProcessID;
            }
        } while (Process32Next(hsnap, &pt));
    }
    CloseHandle(hsnap); // close handle on failure
    return 0;
}

int
main()
{
    // bully::SimulationConfig config = {};
    // config.initialNodeCount        = 5;
    // bully::Simulation::instance().simulate(config);
    DWORD pid = MyGetProcessId(TEXT("BullyAlgo.exe"));
    std::cout << pid;
    if (pid == 0) {
        printf("error 1");
        getchar();
    } // error

    Sleep(10000);
    return 0;
}

// Try to connect to the routing server
// If you fail to connect to router server create a new one
// fetch available indices from routing server and select the least one as the new node id
// fetch list of all available nodes
// fetch current coordinator
// if coordinator index is less than the new node index call for election

// 1. loop from 0 to 1000 (or given maximum index)
// 2. if you fail to connect to a pipe server with the name and index then use this as your server (fill least indices first to
// fight fragmentation)
// 3. Mark `foundParkingSlot=true` and `parkingSlotIndex=${the free index you just found}`
// 4. keep on moving untile you successfully connect to a node
// 5. Mark `FirstNetworkNode=${the index you just hit}`
// 6. query this node asking for the available nodes in the network and the current coordinator
// 7. If you still haven't found a parking slot check any free slot between the passed list of network nodes
// 8. broadcast to the whole network to let them know you now exist
// 9. if your index is greater than the coordinator then call for election

// NOTES:
// you can have external debugging processes which can read and write commands to the nodes
