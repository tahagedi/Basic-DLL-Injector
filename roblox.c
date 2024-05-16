#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <tlhelp32.h>
#include <commdlg.h>
#include <Shellapi.h> // For ShellExecute function

void printLastError(const char* msg) {
    DWORD errorCode = GetLastError();
    char* errorMsg;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, errorCode, 0, (LPSTR)&errorMsg, 0, NULL);
    printf("%s: %s\n", msg, errorMsg);
    LocalFree(errorMsg);
}

void listProcesses() {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        printLastError("CreateToolhelp32Snapshot failed");
        return;
    }

    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(snapshot, &processEntry)) {
        printf("PID\tProcess Name\n");
        do {
            printf("%lu\t%s\n", processEntry.th32ProcessID, processEntry.szExeFile);
        } while (Process32Next(snapshot, &processEntry));
    } else {
        printLastError("Process32First failed");
    }

    CloseHandle(snapshot);
}

DWORD GetProcessIdByName(const char *processName) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        printLastError("CreateToolhelp32Snapshot failed");
        return 0;
    }

    if (Process32First(snapshot, &processEntry)) {
        do {
            if (strcmp(processEntry.szExeFile, processName) == 0) {
                CloseHandle(snapshot);
                return processEntry.th32ProcessID;
            }
        } while (Process32Next(snapshot, &processEntry));
    }

    printLastError("Process not found");
    CloseHandle(snapshot);
    return 0;
}

int InjectDLL(DWORD processId, const char *dllPath) {
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processId);
    if (process == NULL) {
        printLastError("OpenProcess failed");
        return 1;
    }

    LPVOID allocatedMemory = VirtualAllocEx(process, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (allocatedMemory == NULL) {
        printLastError("VirtualAllocEx failed");
        CloseHandle(process);
        return 1;
    }

    if (!WriteProcessMemory(process, allocatedMemory, dllPath, strlen(dllPath) + 1, NULL)) {
        printLastError("WriteProcessMemory failed");
        VirtualFreeEx(process, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(process);
        return 1;
    }

    HANDLE remoteThread = CreateRemoteThread(process, NULL, 0, (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle("kernel32.dll"), "LoadLibraryA"), allocatedMemory, 0, NULL);
    if (remoteThread == NULL) {
        printLastError("CreateRemoteThread failed");
        VirtualFreeEx(process, allocatedMemory, 0, MEM_RELEASE);
        CloseHandle(process);
        return 1;
    }

    WaitForSingleObject(remoteThread, INFINITE);

    VirtualFreeEx(process, allocatedMemory, 0, MEM_RELEASE);
    CloseHandle(remoteThread);
    CloseHandle(process);

    printf("DLL injected successfully.\n");
    return 0;
}

void setConsoleColorGreen() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void showLoadingScreen() {
    system("cls");
    setConsoleColorGreen();
    printf("DLL Injector by Taha\n");
    printf("Loading...\n");
    // Simulate loading process
    Sleep(5000); // Sleep for 5 seconds (5000 milliseconds)
    system("cls");
}

void openWebsite(const char* url) {
    ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}

void displayMenu() {
    int choice;
    char dllPath[MAX_PATH];
    char processName[MAX_PATH];

    do {
        system("cls");

        printf("DLL Injector by Taha\n");
        printf("1. Inject DLL\n");
        printf("2. List Processes\n");
        printf("3. Creds\n");
        printf("4. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        system("cls");

        switch (choice) {
            case 1:
                printf("Enter the name of the target process (e.g., target.exe): ");
                scanf("%s", processName);

                selectDLL(dllPath);

                DWORD processId = GetProcessIdByName(processName);
                if (processId == 0) {
                    printf("Could not find the target process.\n");
                } else {
                    InjectDLL(processId, dllPath);
                }
                break;
            case 2:
                listProcesses();
                break;
            case 3:
                openWebsite("https://github.com/TahaGedi\n"); // Change the URL
                break;
            case 4:
                printf("Exiting...\n");
                break;
            default:
                printf("Invalid choice, please try again.\n");
        }

        if (choice != 4) {
            printf("\nPress key to return to menu...");
            getchar();
            getchar();
        }

    } while (choice != 4);
}

void selectDLL(char *dllPath) {
    OPENFILENAME ofn;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Dynamic Link Libraries\0*.dll\0";
    ofn.lpstrFile = szFileName;
    ofn.lpstrTitle = "Select DLL to Inject";
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (GetOpenFileName(&ofn)) {
        strcpy(dllPath, szFileName);
    }
}

int main() {
    SetConsoleTitle("DLL Injector by Taha");
    showLoadingScreen();
    setConsoleColorGreen();
    displayMenu();
    return 0;
}
