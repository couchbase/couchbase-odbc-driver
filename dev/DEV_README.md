# Dev Setup for Mac

## Setup Driver Manager

Prerequisites:
- automake
- autoconf
- libtool

Open Source Driver Managers:
- [UnixODBC](https://www.unixodbc.org/), I prefer this as it's compatible for both mac and linux.
- [iODBC](https://www.iodbc.org/dataspace/doc/iodbc/wiki/iodbcWiki/WelcomeVisitors)

Steps:
- Download the ```.tar.gz``` file
- ``` gunzip <driver_manager>.tar.gz ```
- ``` tar xvf <driver_manager>.tar```
- ```cd <driver_manager>```
- ```./configure```
- ```make```
- ```sudo make install```

#### Note:
> By default the files are installed into ```/usr/local```. As is usual with configure, this location can be changed by altering the prefix option to configure.

> example: ``` ./configure --prefix=/usr/local/driver_manager_name```

> This will install the lib, bin, include and etc directories in /usr/local/driver_manager/lib etc.

> We need to make sure that our installed libraries are visible to linker and loaders.
> ```echo 'export DYLD_LIBRARY_PATH="$DYLD_LIBRARY_PATH:/path/to/installed/libraries/directory"' >> ~/.zshrc```

## Setup Driver

Prerequisites:
- ```brew install icu4c```


Steps:
* If you don't have repo cloned yet, ```git clone --recursive https://github.com/couchbaselabs/couchbase-odbc-driver.git```
* If you have repo already cloned, then after fetching a patch, run this command: ```cd couchbase-odbc-driver/contrib && git submoudle update --init```
* ```cd couchbase-odbc-driver```
* ```mkdir build```
* ```cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DICU_ROOT=$(brew --prefix)/opt/icu4c -S . -B build```
* ```cd build```
* ```make```
* ```sudo make install```, this will install _libcouchbaseodbc.dylib_ and _libcouchbaseodbcw.dylib_ in _/usr/local/lib_


## Test Driver

For now (initial code base) we need to test in on a clickHouse server

### Setup ClickHouse on docker

- ```docker run -d -p 18123:8123 -p19000:9000 --name some-clickhouse-server --ulimit nofile=262144:262144 clickhouse/clickhouse-server```
- Test whether installation succeed or not, ```echo 'SELECT version()' | curl 'http://localhost:18123/' --data-binary @-```
- If you want to accesss clickHouse UI, visit ```http://localhost:18123/play```

### Setup Data Source Configurations

I am specifying steps for UnixODBC, same can be done for iODBC.
I have my unixODBC installed at /usr/local/unixODBC

Steps:
- ```cd /usr/local/unixODBC```
- ```ls``` , output: _bin etc include lib share_
- ```cd etc```
- Now you need to create 2 files here _odbc.ini_ and _odbcinst.ini_
- Copy content from _couchbase-odbc-driver/dev/sample_odbc.ini_ and _couchbase-odbc-driver/dev/sample_odbcinst.ini_

### Check Connection

#### Method 1 (With Custom Application)
We can write custom applications to connect to our dataSource, I have added a sample minimal application to connect to the data source _couchbase-odbc-driver/dev/minimum_odbc_application.c_

Steps to compile and Run Custom Applications
- ```cd couchbase-odbc-driver/dev ```
- To compile it, ```gcc -o main -I/usr/local/unixODBC/include minimum_odbc_application.c -L/usr/local/unixODBC/lib -lodbc```
- ```./main```

#### Method 2 (Using Driver Manager tools)
Driver Manager provide us tools to connect and issue query to our Data Source. This is nifty for developers to test their driver, since they don't have to write custom odbc applications.

Steps:
- ```cd /usr/local/unixODBC/bin```
- ```./isql -k DSN="Couchbase DSN (ANSI)"```
- you can also add isql to the PATH env variable, ```echo 'export PATH="/usr/local/unixODBC/bin:$PATH"' >> ~/.zshrc```

On a successful connection you should see an output like this

```text
+--------------------------------------+
| Connected!                           |
|                                      |
| sql-statement                        |
| help [tablename]                     |
| echo [string]                        |
| quit                                 |
|                                      |
+--------------------------------------+
SQL>

```