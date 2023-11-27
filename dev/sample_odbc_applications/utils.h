#ifndef UTILS_H
#define UTILS_H

#ifdef _WIN32
  #include <windows.h>
#endif

#include <sql.h>
#include <sqlext.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <odbcinst.h>

void check_error(SQLRETURN e,char * fn, SQLHANDLE handle, SQLSMALLINT type, FILE *fp );
bool check_and_print_null(const char * nameBuff, SQLLEN * ind, FILE *fp);
void extract_error(char * fn, SQLHANDLE handle, SQLSMALLINT type, FILE *fp);
bool is_dsn_availability(SQLHENV env, const char * dsn);
bool is_driver_availability(SQLHENV env, const char * driver_name);
bool check_and_print_null_two_params(SQLLEN * ind, FILE *fp);
#endif
