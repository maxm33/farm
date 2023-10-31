# Farm

**_Please read "Project_description.pdf" to learn more_**

This is the project I was given as final exam for the Laboratory 2 course.

The main focus of this project is to implement a concurrent multithreaded system and manage possible race conditions
with the help of Mutexes, Atomicity and Condition Variables.

_farm_ is a program in which there are 2 processes: [MasterWorker](#masterworker) (main) and [Collector](#collector) (child process).

### MasterWorker

MasterWorker is a multithreaded process that has n threads (4 by default, [see the possible options below](#options)).

Each of these threads (named Workers from now on) get one task from a queue managed by the MasterWorker.

The task is a regular binary file, and Workers do some operations with the content of the files.

Whenever a file has been successfully processed, the Worker sends his results to the Collector through a socket connection established between the Collector and the MasterWorker.

If there are more files in the queue, the Workers that are finished with the previous files will now get another one from the queue and so on until the queue is empty.

MasterWorker also handles the signals. When a signal (SIGHUP, SIGINT, SIGQUIT, SIGTERM) is received, the Workers stop accepting new requests from the MasterWorker, but files that were planned to be processed already are still being processed.

Eventually, Workers are shut down, Collector process is killed and the program is forced to end.

### Collector

Collector just waits to receive data from Workers and prints it on the console.

## How to use

There is a makefile that compiles _farm.c_, _generafile.c_ and then runs _test.sh_.

The test uses _generafile_ to generate the binary files to process and then proceeds to run some tests for _farm_.

Simply entering

```
make
```

in the command prompt, while in the main folder, should do the trick.

## Options

- -n < integer > : number of threads used (default is 4).
- -q < integer > : max number of requests that can be made by MasterWorker at the same time, or max number of files that can be processed simultaneously (default is 8).
- -t < integer > : the delay between each request made by MasterWorker (in milliseconds, default is 0).

**Note:** This program only works on UNIX 64-bit systems.
