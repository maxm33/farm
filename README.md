# Farm

> [!IMPORTANT]
>  - Please read &ensp;_Project_description.pdf_&ensp; to learn about the project.<br>
>  - This program works as intended only on UNIX 64-bit systems.

## Usage

- Install **_make_** if you don't have it already and compile (this will run pre-built tests)
```
sudo apt install make
make
```
OR
- Compile and run with whatever option you want (listed below)
```
gcc -pthread -o farm farm.c
gcc -pthread -o generafile generafile.c
./farm -n <integer> -q <integer> -t <integer> file*
```

## Options

- n is the number of threads used (default is 4).
- q is the max requests that can be made by MasterWorker at the same time, or max files that can be processed simultaneously (default is 8).
- t is the delay between each request made by MasterWorker (in milliseconds, default is 0).

> [!NOTE]
> None of these options are mandatory.
