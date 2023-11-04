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

std::string getGoldfishCertPathWindows(){
    std::wstring ws( fetchDriverKeyDataFromCouchbaseODBCDriverWinReg() );
    std::string DriverPath( ws.begin(), ws.end() );

    /*
        Driver Path Example-> C:\Program Files\couchbase-odbc\bin\couchbaseodbcw.dll
        It is dynamic, and depends on what user selects at the time of installation
        It will have an entry in Windows Registry under :
        HKEY_LOCAL_MACHINE\SOFTWARE\\ODBC\\ODBCINST.INI\\Couchbase ODBC Driver (Unicode)
        Key : Driver, value : Driver Path

        Installation of Couchbase ODBC Driver, installs the GoldfisgCertificate.txt at
        GoldfishCertPath Example-> C:\Program Files\couchbase-odbc\share\doc\couchbase-odbc\config\GoldfishCertificate.txt
        This path is wrt the Driver Path Example above, and will change with the Driver Path.
    */

    std::string searchString = "\\bin";
    size_t pos = DriverPath.find(searchString);
    if (pos != std::string::npos) {
        std::string temp = DriverPath.substr(0, pos);
        for (size_t i = 0; i < temp.length(); ++i) {
            if (temp[i] == '\\') {
                temp.insert(i, 1, '\\');
                i++;
            }
        }
        return temp + "\\\\share\\\\doc\\\\couchbase-odbc\\\\config\\\\GoldfishCertificate.txt";
    } else {
        std::cerr << "Search string not found." << std::endl;
    }
    return "";
}