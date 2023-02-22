#include <stdio.h>
#include <windows.h>
#include <tlhelp32.h>
#include <string.h>
#include <stdlib.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

int DumpProc(int pid, char* dmpout) {
    int pidnamelen = strlen(dmpout) + 1;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, dmpout, pidnamelen, NULL, 0);
    wchar_t* wdmpout = (wchar_t*)malloc(wlen * sizeof(wchar_t));

    MultiByteToWideChar(CP_UTF8, 0, dmpout, pidnamelen, wdmpout, wlen);
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!process) {
        return 1;
    }

    HANDLE dumpf = CreateFileW(wdmpout, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (dumpf == INVALID_HANDLE_VALUE) {
        CloseHandle(process);
        return 1;
    }

    BOOL result = MiniDumpWriteDump(process, pid, dumpf, MiniDumpWithDataSegs, NULL, NULL, NULL);
    if (!result) {
        return 1;
    }

    CloseHandle(dumpf);
    CloseHandle(process);
    return 0;
}

void help(char* progname) {
    printf(R"EOF(usage: %s PROCESS OUTPUT
    options:
      PROCESS,                  process name (ex: notepad.exe)
      OUTPUT,                   output file
)EOF", progname);
}

DWORD GetPID(char* pid) {
    int pidnamelen = strlen(pid) + 1;
    int wlen = MultiByteToWideChar(CP_UTF8, 0, pid, pidnamelen, NULL, 0);
    wchar_t* wpid = (wchar_t*)malloc(wlen * sizeof(wchar_t));

    MultiByteToWideChar(CP_UTF8, 0, pid, pidnamelen, wpid, wlen);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        return 1;
    }

    PROCESSENTRY32 procentry;
    procentry.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(snap, &procentry)) {
        do {
            if (lstrcmpi(procentry.szExeFile, wpid) == 0) {
                CloseHandle(snap);
                return procentry.th32ProcessID;
            }
        } while (Process32Next(snap, &procentry));
    }

    CloseHandle(snap);
    return 1;
}

int main(int argc, char* argv[]) {
    int dmp;
    if (argv[1] == NULL || argv[2] == NULL) {
        help(argv[0]);
        return 1;
    }

    DWORD pid = GetPID(argv[1]);
    if (pid == 1) {
        printf("[-] It was not possible to perform the process. Make sure you have permission\n");
        return 1;
    }
    printf("[+] %s's PID is %d\n", argv[1], pid);
    printf("[+] Pouring the process %d\n", pid);

    dmp = DumpProc(pid, argv[2]);
    if (dmp != 0) {
        puts("[-] It was not possible to perform the process. Make sure you have permission");
        return 1;
    }

    return 0;
}