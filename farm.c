#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#define UNIX_PATH_MAX 108
#define MAX_STRING_SIZE 255
#define SOCKNAME "./farm.sck"

typedef struct
{
  long result;
  char filename[MAX_STRING_SIZE];
} Data;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t isFull = PTHREAD_COND_INITIALIZER; // condition variable

struct sockaddr_un server;
int numThread = 4, queueLength = 8, activeWorkers = 0, numFiles = 0, fd_skt, delay = 0, j = 0;
char **files;

// threads function
void *elaborate(void *arg);

// when signal is received, execution of the program is forced to end
void handler(int signal)
{
  pthread_mutex_lock(&mutex1);
  numFiles = 0;
  pthread_mutex_unlock(&mutex1);
}

void checkFailure(int n)
{
  if (n == -1)
  {
    perror("Operation failed");
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char *argv[])
{

  if (argc == 1)
  { // no args, program terminated
    printf("Must specify a list of files\n");
    exit(EXIT_FAILURE);
  }

  // install SIGHUP, SIGINT, SIGQUIT, SIGTERM handlers
  sigset_t set;
  struct sigaction s;
  struct stat *buffer;

  checkFailure(sigfillset(&set));
  // blocking all signals
  checkFailure(pthread_sigmask(SIG_SETMASK, &set, NULL));
  checkFailure(sigemptyset(&set));
  checkFailure(sigaddset(&set, SIGINT));
  checkFailure(sigaddset(&set, SIGQUIT));
  checkFailure(sigaddset(&set, SIGTERM));
  checkFailure(sigaddset(&set, SIGHUP));
  s.sa_mask = set;
  s.sa_handler = &handler;
  s.sa_flags = SA_RESTART;
  checkFailure(sigaction(SIGINT, &s, NULL));
  checkFailure(sigaction(SIGQUIT, &s, NULL));
  checkFailure(sigaction(SIGHUP, &s, NULL));
  checkFailure(sigaction(SIGTERM, &s, NULL));
  checkFailure(sigemptyset(&set));
  // no signal is blocked now
  checkFailure(pthread_sigmask(SIG_SETMASK, &set, NULL));

  files = (char **)malloc(sizeof(char *));
  buffer = (struct stat *)malloc(sizeof(struct stat));

  // program menu options: -n, -q, -t
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-n") == 0)
    {
      numThread = atoi(argv[i + 1]); // sets number of threads, default is 4
      i++;
    }
    else if (strcmp(argv[i], "-q") == 0)
    {
      queueLength = atoi(argv[i + 1]); // sets queue length, default is 8
      i++;
    }
    else if (strcmp(argv[i], "-t") == 0)
    {
      delay = atoi(argv[i + 1]); // sets delay, default is 0
      i++;
    }
    else
    {
      if (strlen(argv[i]) > MAX_STRING_SIZE)
      { // skips if filename length > 255
        // printf("File '%s' exceeded the max filename length\n", argv[i]);
        continue;
      }
      checkFailure(stat(argv[i], buffer));
      if (!S_ISREG(buffer->st_mode))
      { // skips file if not regular
        // printf("File '%s' is not regular\n", argv[i]);
        continue;
      }
      // reallocs memory for another string in the list of files (multi-D array)
      files = (char **)realloc(files, (j + 1) * sizeof(char *));
      // adjusts the allocated string size;
      files[j] = (char *)malloc(strlen(argv[i]) * sizeof(char) + 1);
      strcpy(files[j], argv[i]);
      j++;
    }
  }
  numFiles = j;
  free(buffer);

  strncpy(server.sun_path, SOCKNAME, UNIX_PATH_MAX);
  server.sun_family = AF_UNIX;

  // ------ START OF CHILD PROCESS ------
  pid_t Collector = fork();
  if (Collector == 0)
  {
    Data collected;
    char prevbadoutput[MAX_STRING_SIZE] = "";

    // blocking SIGHUP, SIGINT, SIGQUIT, SIGTERM
    checkFailure(sigaddset(&set, SIGINT));
    checkFailure(sigaddset(&set, SIGQUIT));
    checkFailure(sigaddset(&set, SIGTERM));
    checkFailure(sigaddset(&set, SIGHUP));
    checkFailure(pthread_sigmask(SIG_BLOCK, &set, NULL));

    int s_sck = socket(AF_UNIX, SOCK_STREAM, 0);
    bind(s_sck, (struct sockaddr *)&server, sizeof(server));
    listen(s_sck, SOMAXCONN);
    int fd_c = accept(s_sck, NULL, 0);

    while (j > 0)
    {
      checkFailure(read(fd_c, &collected, sizeof(Data)));
      /*
        When a signal is received by masterworker, if there are threads that are
        already in queue (usually it's the case), those threads finish to work
        on the files and send results to this process (collector), while the
        remaining threads are terminated. Some garbage data still remains on the
        socket, and it is filtered by this strcmp().
      */
      if (strcmp(collected.filename, prevbadoutput))
      {
        printf("%ld %s\n", collected.result, collected.filename);
      }
      strcpy(prevbadoutput, collected.filename);
      j--;
    }

    close(fd_c);
    close(s_sck);
    unlink(SOCKNAME);
    exit(EXIT_SUCCESS);
  }
  // ------ END OF CHILD PROCESS ------

  fd_skt = socket(AF_UNIX, SOCK_STREAM, 0);

  while (connect(fd_skt, (struct sockaddr *)&server, sizeof(server)) == -1)
  {
    if (errno == ENOENT)
    {
      sleep(1); // listen() not called yet, wait 1 sec and re-try till connected
    }
    else
    {
      perror("connect() failed");
      unlink(SOCKNAME); // connect failed so it deletes the socket file
      exit(EXIT_FAILURE);
    }
  }
  // masterworker and collector are connected now

  // creating and waiting all the thread workers
  pthread_t tid[numThread];

  for (int i = 0; i < numThread; i++)
  {
    pthread_create(&tid[i], NULL, elaborate, NULL);
  }

  for (int i = 0; i < numThread; i++)
  {
    pthread_join(tid[i], NULL);
  }

  close(fd_skt);
  unlink(SOCKNAME); // deletes the socket file

  // frees multidimensional array
  for (int i = 0; i < j; i++)
  {
    free(files[i]);
  }
  free(files);

  return 0;
}

void *elaborate(void *arg)
{
  Data data = {}; 
  int fileSize = 0;
  long n = 0, result = 0;
  FILE *fd;
  char *processedFile;
  struct stat *buffer;

  while (1)
  {
    pthread_mutex_lock(&mutex1);
    if (numFiles <= 0)
    { // checks if there are any files left in the array...
      pthread_mutex_unlock(&mutex1);
      pthread_exit(NULL); // if not, thread is terminated
    }

    processedFile = (char *)malloc(strlen(files[numFiles - 1]) * sizeof(char) + 1);
    strcpy(processedFile, files[numFiles - 1]); // gets filename
    numFiles--;

    while (activeWorkers >= queueLength)
    {                                      // if queue is full...
      pthread_cond_wait(&isFull, &mutex1); // ...thread goes in waiting
    }
    activeWorkers++; // thread is actually working now
    pthread_mutex_unlock(&mutex1);

    pthread_mutex_lock(&mutex2); // lock so each thread waits the delay one by one.
    usleep(delay * 1000);        // worker waits delay time (in milliseconds)
    pthread_mutex_unlock(&mutex2);

    buffer = (struct stat *)calloc(1, sizeof(struct stat));

    checkFailure(stat(processedFile, buffer));
    fileSize = buffer->st_size;

    free(buffer); // free dynamic memory used

    fd = fopen(processedFile, "rb"); // opens binary file
    if (!fd)
    {
      perror("fopen() failed");
      exit(EXIT_FAILURE);
    }
    /*
    gets one long at a time from the file, which is multiplied by i and summed
    to the final result
    */
    fseek(fd, 0, SEEK_SET);
    for (int i = 0; i < fileSize / sizeof(long); i++)
    {
      fread(&n, sizeof(long), 1, fd);
      result = result + (i * n);
    }
    fclose(fd);

    // copying results to custom structure
    strcpy(data.filename, processedFile);
    data.result = result;

    checkFailure(write(fd_skt, &data, sizeof(data))); // sends results to collector

    free(processedFile); // free dynamic memory used
    result = 0;

    /*
    thread gets lock and, as it finishes working by sending the result to
    collector, signals to unlock the first thread in the waiting queue, then
    lock is released
    */
    pthread_mutex_lock(&mutex3);
    activeWorkers--;
    pthread_cond_signal(&isFull);
    pthread_mutex_unlock(&mutex3);
  }
}
