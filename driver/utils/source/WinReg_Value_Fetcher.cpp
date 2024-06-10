#ifdef _WIN32
#include "driver/utils/include/WinReg_Value_Fetcher.h"

WCHAR* fetchDriverKeyDataFromCouchbaseODBCDriverWinReg(){
    HKEY hKey;
    LONG lRes = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\ODBC\\ODBCINST.INI\\Couchbase ODBC Driver (Unicode)", 0, KEY_READ|KEY_WOW64_64KEY, &hKey);

    if (lRes == ERROR_SUCCESS) {
        WCHAR value[1024];
        DWORD valueSize = sizeof(value);

        lRes = RegQueryValueExW(hKey, L"Driver", NULL, NULL, reinterpret_cast<LPBYTE>(value), &valueSize);

        if (lRes == ERROR_SUCCESS) {
            std::wcout << L"Driver Path: " << value << std::endl;
            return value;
        } else {
            std::cerr << "Failed to query registry value. Error code: " << lRes << std::endl;
        }

        RegCloseKey(hKey);
    } else {
        std::cerr << "Failed to open registry key. Error code: " << lRes << std::endl;
    }
    return 0;
}
#endif