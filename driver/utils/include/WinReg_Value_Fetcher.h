#include <windows.h>
#include <iostream>
#include <string>
#define KEY_WOW64_32KEY         (0x0200)
#define KEY_WOW64_64KEY         (0x0100)

WCHAR* fetchDriverKeyDataFromCouchbaseODBCDriverWinReg();
std::string getGoldfishCertPathWindows();