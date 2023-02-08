#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NUM_CHILD 5 // number of child processes to create

unsigned int cpidc = 0; // number of child processes created
pid_t cpidv[NUM_CHILD]; // array of child process ids

int kint = 0; // SIGINT occurence

void term_handler()
{ // SIGTERM signal handler
    printf("child[%d]: received SIGTERM signal, terminating\n", getpid());
    exit(0);
}

void kint_handler()
{ // SIGINT signal handler
    kint = 1;
    printf("parent[%d]: received SIGINT signal\n", getpid());
}

void child_process()
{ // child process
#ifdef WITH_SIGNALS
  // set custom handler for SIGTERM and ignore SIGINT
    signal(SIGTERM, term_handler);
    signal(SIGINT, SIG_IGN);
#endif
    pid_t pid = getpid();
    printf("child[%d]: ppid -> %d\n", pid, getppid());
    sleep(10);
    printf("child[%d]: execution complete\n", pid);
    exit(0);
}

void send_children(int sig)
{ // send signal to all created child processes
    pid_t pid = getpid();
    for (int i = 0; i < cpidc; i++)
    {
        printf("parent[%d]: sending %s signal to child[%d]\n", pid, strsignal(sig), cpidv[i]);
        kill(cpidv[i], sig);
    }
}

void set_signals(void *handler)
{
    for (int i = 0; i < NSIG; i++)
        signal(i, handler);
}

void _fork()
{ // extended fork with error handling
    pid_t pid = getpid();
    pid_t cpid = fork();
    if (cpid == 0)
        child_process();
    else if (cpid > 0)
    {
        printf("parent[%d]: created child[%d]\n", pid, cpid);
        cpidv[cpidc++] = cpid;
    }
    else
    {
        printf("parent[%d]: could not create child, fork returned %d\n", pid, cpid);
        send_children(SIGTERM);
        exit(1);
    }
}

int main()
{ // main(parent) process
    if (NUM_CHILD > 0)
    {
#ifdef WITH_SIGNALS
        // ignore all signals, restore default handler for SIGCHLD and set custom handler for SIGINT
        set_signals(SIG_IGN);
        signal(SIGCHLD, SIG_DFL);
        signal(SIGINT, kint_handler);
#endif
        pid_t pid = getpid(), cpid;
        for (int i = 0; i < NUM_CHILD; i++)
        {
            if (kint)
                break;
            _fork();
            sleep(1);
        }
        printf("parent[%d]: created %d child processes\n", pid, cpidc);
        unsigned int ec = 0;
        int status;

        while ((cpid = wait(&status)) > 0)
        {
            printf("parent[%d]: child[%d] exited with code %d\n", pid, cpid, status);
            ec++;
        }
        printf("parent[%d]: received %d exit codes\n", pid, ec);
        printf("parent[%d]: %s", pid, ec == cpidc ? "No more child processes" : "Not all created child processes exited");

#ifdef WITH_SIGNALS
        // restore default handlers for all signals
        set_signals(SIG_DFL);
#endif
        printf("\nparent[%d]: execution complete\n", pid);
    }

    return 0;
}