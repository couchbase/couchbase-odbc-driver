# Prerequisites:
#### In all test files, the default DSN name is set as "Couchbase DSN (ANSI)," but it needs to be adjusted according to the DSN you have created

# OS: MAC

## Command to compile a sample odbc test applicaion:
The following command requires path to driver manager's include and lib directories.
This will vary based on configuration, see: [Link](../DEV_README.md#note)
```bash
gcc -o your_executable_name -I/path/to/DRIVER_MANAGER/include test_file_name.c utils.c -L/path/to/DRIVER_MANAGER/lib -lodbc -lodbcinst
```

## Command to run the sample odbc test applicaion:
```bash
./your_executable_name
```

## Enable libcouchbase logging
```bash
export LCB_LOGLEVEL=5
```
# OS: WIN

## Command to compile a sample odbc test applicaion:
```bash
cl test_file_name.c utils.c  /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\um" odbc32.lib
```

## Command to run the sample odbc test applicaion:
```bash
test_file_name.exe
```
## Enable libcouchbase logging
```bash
set LCB_LOGLEVEL=5
```