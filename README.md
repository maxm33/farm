# Farm

This was the final assignment for Laboratory 2 course.

<br>

> [!IMPORTANT]
>
> - Please read &ensp;_Project_description.pdf_&ensp; to learn about the assignment.<br>
> - This program works as intended only on Unix-like 64-bit systems.

<br>

## Usage

- Run pre-built tests

```
make
```

OR

- Compile and run (check the available options) 

```
gcc -pthread -o farm farm.c
./farm <option(s)> <path-to-file(s)>
```

<br>

## Options

- `-n <number>` to set the number of threads used (default is 4);
- `-q <number>` to set a limit to requests that can be made by MasterWorker at once (default is 8);
- `-t <number>` to set a delay (in milliseconds) in between each request made by MasterWorker (default is 0).
