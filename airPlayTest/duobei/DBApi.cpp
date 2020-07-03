#include "DBApi.h"

namespace duobei {

#ifndef __APPLE__
int DBApi::ShellExecuteExOpen(std::wstring &exe_) {
    SHELLEXECUTEINFO sei = { sizeof(SHELLEXECUTEINFO) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = TEXT("runas");
    sei.lpFile = exe_.c_str();
    sei.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteEx(&sei)) {
        DWORD dwStatus = GetLastError();
        if (dwStatus == ERROR_CANCELLED) {
            Callback::statusCodeCall(WINDOWS_MDNS_NOT_OPEN);
        } else if (dwStatus == ERROR_FILE_NOT_FOUND) {
            Callback::statusCodeCall(PROC_EXE_PATH_ERROR);
        } else {
            return 0;
        }
    }
    return -1;
}

int DBApi::GetProcessIDByName(std::wstring &name_) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hSnapshot) {
        return -1;
    }
    PROCESSENTRY32 pe = { sizeof(pe) };
    for (bool ret = Process32First(hSnapshot, &pe); ret; ret = Process32Next(hSnapshot, &pe)) {
        if (wcscmp(pe.szExeFile, name_.c_str()) == 0) {
            CloseHandle(hSnapshot);
            return pe.th32ProcessID;
        }
    }
    CloseHandle(hSnapshot);
    return -1;

}

int DBApi::CheckProcess(std::wstring &path){
    std::wstring processName = L"mDNSResponder.exe";
    int ret = 0;
    if ((ret = GetProcessIDByName(processName)) == -1) {
        return ShellExecuteExOpen(path);
    } else {
        return ret;
    }
}
#endif

int DBApi::startApi(std::string &uid, std::wstring &path, int w, int h) {
    int ret = 0;
#ifndef __APPLE__
    if ((ret = CheckProcess(path)) < 0) {
        return ret;
    }
#endif
    if ((ret = airPlayServer.initServer()) < 0) {
        WriteErrorLog("airPlayServer.initServer error : %d", ret);
        return ret;
    }
    if ((ret = airPlayServer.startServer()) < 0) {
        WriteErrorLog("airPlayServer.startServer error : %d", ret);
        return ret;
    }
    if ((ret = airPlayServer.publishServer()) < 0) {
        WriteErrorLog("airPlayServer.publishServer error : %d", ret);
        return ret;
    }

    parser.Init(uid, w, h);
    airPlayServer.setParser(parser);

    return ret;
}

int DBApi::stopApi() {
    airPlayServer.stopServer();
    return 0;
}

}