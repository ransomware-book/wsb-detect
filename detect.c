#include "detect.h"
#include "util.h"

BOOL wsb_detect_username(VOID)
{
    WCHAR wcUser[UNLEN + 1];
    RtlSecureZeroMemory(wcUser, sizeof(wcUser));

    DWORD dwLength = (UNLEN + 1);
    if (GetUserNameW(wcUser, &dwLength))
    {
        return (wcscmp(wcUser, SANDBOX_USER) == 0);
    }

    return FALSE;
}

BOOL wsb_detect_proc(VOID)
{
    BOOL bFound = FALSE;

    HANDLE hProcesses = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcesses == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    PROCESSENTRY32 pe32Entry;
    pe32Entry.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcesses, &pe32Entry))
    {
        return FALSE;
    }

    do
    {
        if (wcscmp(pe32Entry.szExeFile, HV_CONTAINER_NAME))
        {
            bFound = TRUE;
            break;
        }
    } 
    while (Process32Next(hProcesses, &pe32Entry));

    CloseHandle(hProcesses);

    return bFound;
}

BOOL wsb_detect_suffix(VOID)
{
    BOOL bFound = FALSE;

    DWORD dwAdapterFlags = 0;
    DWORD dwAddrSize = 0;
    if (GetAdaptersAddresses(AF_INET, dwAdapterFlags, NULL, NULL, &dwAddrSize) == 0)
    {
        return 0;
    }

    PIP_ADAPTER_ADDRESSES pAdapterAddrs;
    PIP_ADAPTER_ADDRESSES pAdapt;
    if ((pAdapterAddrs = (PIP_ADAPTER_ADDRESSES)GlobalAlloc(GPTR, dwAddrSize)) == NULL)
    {
        return 0;
    }

    if (GetAdaptersAddresses(AF_INET, dwAdapterFlags, NULL, pAdapterAddrs, &dwAddrSize) != ERROR_SUCCESS)
    {
        GlobalFree(pAdapterAddrs);
        return 0;
    }

    for (pAdapt = pAdapterAddrs; pAdapt; pAdapt = pAdapt->Next)
    {
        if (wcscmp(pAdapt->DnsSuffix, SANDBOX_DNS_SUFFIX) == 0)
        {
            bFound = TRUE;
            break;
        }
    }

    GlobalFree(pAdapterAddrs);
    return bFound;
}

BOOL wsb_detect_office(VOID)
{
    WCHAR wcDir[MAX_PATH + 1];
    RtlSecureZeroMemory(wcDir, sizeof(wcDir));

    if (GetWindowsDirectoryW(wcDir, MAX_PATH) == 0)
    {
        return FALSE;
    }

    WCHAR wcPath[MAX_PATH + 1];
    RtlSecureZeroMemory(wcPath, sizeof(wcPath));
    if (StringCbPrintfW(wcPath, sizeof(wcPath), SANDBOX_WD_OFFICE_FMT, wcDir) != S_OK)
    {
        return FALSE;
    }

    return util_path_exists(wcPath, 0);
}

BOOL wsb_detect_dev(VOID)
{
    return util_path_exists(HV_VMSMB_DEV, 0);
}

BOOL wsb_detect_genuine(VOID)
{
    GUID spUID;
    RtlSecureZeroMemory(&spUID, sizeof(spUID));

    RPC_WSTR spRPC = (RPC_WSTR)L"55c92734-d682-4d71-983e-d6ec3f16059f";
    if (UuidFromString(spRPC, &spUID) != RPC_S_OK)
    {
        return FALSE;
    }

    SL_GENUINE_STATE slState;
    if (SLIsGenuineLocal(&spUID, &slState, NULL) != S_OK)
    {
        return FALSE;
    }

    return (slState != SL_GEN_STATE_IS_GENUINE);
}

BOOL wsb_detect_cmd(VOID)
{
    BOOL bFound = FALSE;
    DWORD dwFlags = KEY_READ;

#ifndef _WIN64
    dwFlags &= KEY_WOW64_64KEY;
#endif

    HKEY hKey;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\RunOnce"), 0, dwFlags, &hKey) != ERROR_SUCCESS)
    {
        return bFound;
    }

    WCHAR achKey[MAX_KEY_LENGTH];
    WCHAR achValue[MAX_VALUE_NAME];
    RtlSecureZeroMemory(achKey, sizeof(achKey));
    RtlSecureZeroMemory(achValue, sizeof(achValue));

    DWORD cchKey = MAX_KEY_LENGTH;
    DWORD cchValue = MAX_VALUE_NAME;
    if (RegEnumValue(hKey, 0, achKey, &cchKey, NULL, NULL, (LPBYTE)achValue, &cchValue) == ERROR_SUCCESS)
    {
        if (wcscmp(achValue, SANDBOX_LOGON_CMD) == 0)
        {
            bFound = TRUE;
        }
    }

    return bFound;
}
