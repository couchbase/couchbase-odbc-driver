#ifdef _WIN32
#include <windows.h>
#include <iostream>
#include <string>
#define KEY_WOW64_32KEY         (0x0200)
#define KEY_WOW64_64KEY         (0x0100)

std::wstring fetchDriverKeyDataFromCouchbaseODBCDriverWinReg();
std::string getLcbLogPath();
#endif