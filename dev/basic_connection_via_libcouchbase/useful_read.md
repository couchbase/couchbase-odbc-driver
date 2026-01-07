### Client cert Authentication:
- https://docs.google.com/document/d/1nDcKF57SBtAQEMB352FjEd0Nq24m99iI5jk4tdMtWFk/edit?usp=sharing

### See this doc for following setup
 - https://docs.google.com/document/d/1vAiaGy2rKAHK2ud_dgsaUtmYQ4hTyGN_InnDVRc0QZ0/edit?usp=sharing
 - CB server runnning locally using docker
 - EA running locally using docker
 - USE alternate address to do the port mapping correctly


#### How to run sample program on windows?
- If you want to run `/couchbase-odbc-driver/dev/sample_odbc_applications.minimal_cba_connect.c`

Follow this:
- Open `x64 Native Tools Cpmmand promt for Vs`
- Go to the dir `sample_odbc_applications`
- Compile
    -> `cl minimal_cba_connect.c utils.c odbc32.lib`
- Run:
    -> `minimal_cba_connect.exe`

To turn on the logging:
- `echo $LCB_LOGLEVEL=5`
