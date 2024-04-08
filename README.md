# Farm

> [!IMPORTANT]
>
> - Please read &ensp;_Project_description.pdf_&ensp; to learn about the project.<br>
> - This program works as intended only on UNIX 64-bit systems.

## Usage

- This will run pre-built tests

```
make
```

OR

- Compile and run

```
gcc -pthread -o farm farm.c
./farm <path-to-file(s)>
```

## Options

- `n` is the number of threads used (default is 4).
- `q` is the max requests that can be made by MasterWorker at the same time, or max files that can be processed simultaneously (default is 8).
- `t` is the delay between each request made by MasterWorker (in milliseconds, default is 0).
