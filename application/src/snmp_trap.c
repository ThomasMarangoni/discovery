#include "snmp_trap.h"
#include "kcolor.h"
#include "lib/gll.h"

#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>

pthread_mutex_t ip_buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t run_loop_mutex = PTHREAD_MUTEX_INITIALIZER;

static ipv4_t *ip_buffer;
static int ip_buffer_pos = 0;
static sds argument_str_array[2];

static pthread_t thread;
static bool thread_used = false;

static volatile bool run_loop_mutexed = true;

static void signal_handler(int signo) {
    if(signo == SIGINT || signo == SIGTERM)
    {
        pthread_mutex_lock(&run_loop_mutex);
        run_loop_mutexed = false;
        pthread_mutex_unlock(&run_loop_mutex);
    }
}


int snmp_trap_read_data(ipv4_t **ip_buffer_out, int* ip_buffer_pos_out)
{
    int mutex_error = pthread_mutex_trylock(&ip_buffer_mutex);
    if(mutex_error)
    {
        return EXIT_FAILURE;
    }

    **ip_buffer_out = *ip_buffer;
    *ip_buffer_pos_out = ip_buffer_pos;

    ip_buffer_pos = 0;
    memset(ip_buffer, 0, IP_BUFFER_SIZE * sizeof(ipv4_t));

    pthread_mutex_unlock(&ip_buffer_mutex);

    return EXIT_SUCCESS;
}

void *snmp_trap_daemon_thread(void* param)
{
    int pipefd[2];
    if(pipe(pipefd)== -1)
    {
        printf(KRED "[ERROR] Pipe error\n" KNORMAL);
        return (void*)EXIT_FAILURE;
    }

    //pid_t parent = getpid();
    pid_t pid = fork();

    // Error:
    if (pid == -1)
    {
        return (void*)EXIT_FAILURE;
    }
    // Parent:
    else if (pid > 0)
    {
        close(pipefd[PIPE_WRITE_END]);

        int cstatus;

        FILE *stream;
        char *line = NULL;
        size_t len = 0;
        ssize_t nread;

        stream = fdopen(pipefd[PIPE_READ_END], "r");

        bool run_loop = true;
        while (run_loop)
        {
            nread = getline(&line, &len, stream);
            if(nread != -1)
            {
                sds line_str = sdsnewlen(line, nread);

                sds *tokens;
                int count;

                tokens = sdssplitlen(line_str, sdslen(line_str), " ", 1, &count);

                if(count == 2 && strstr(tokens[1],"snmptrapd") == NULL)
                {
                    sds timestamp_str = sdsnew(tokens[0]);
                    sds ip_str = sdsnew(tokens[1]);
                    ip_str = sdstrim(ip_str, "\n");

                    printf("IP: %s\n", ip_str);
                    ipv4_t ip = ipv4_from_str(&ip_str);

                    sdsfree(timestamp_str);
                    sdsfree(ip_str);

                    pthread_mutex_lock(&ip_buffer_mutex);

                    if(ip_buffer_pos >= TRAP_SLEEP_US)
                    {
                        printf(KRED "[ERROR] SNMP Trap buffer is full, dropping packages." KNORMAL);
                        printf(KNORMAL "[NOTE] Current buffer size: %u" KNORMAL, TRAP_SLEEP_US);
                    }
                    else
                    {
                        ip_buffer[ip_buffer_pos] = ip;
                        ip_buffer_pos++;
                    }
                    pthread_mutex_unlock(&ip_buffer_mutex);
                }

                sdsfree(line_str);

                sdsfreesplitres(tokens,count);
            }
            else
            {
                /// Sleep for amount of microseconds defined in TRAP_SLEEP_US, because nothing needs to be done 
                usleep(TRAP_SLEEP_US);
            }

            if(line != NULL)
            {
                //free(line);
            }

            int mutex_error = pthread_mutex_trylock(&run_loop_mutex);
            if(!mutex_error)
            {
                run_loop = run_loop_mutexed;
                pthread_mutex_unlock(&run_loop_mutex);
            }
        }
        
        fclose(stream);     

        close(pipefd[PIPE_READ_END]);

        sdsfree(argument_str_array[0]);
        sdsfree(argument_str_array[1]);

        pthread_mutex_lock(&ip_buffer_mutex);
        free(ip_buffer);
        pthread_mutex_unlock(&ip_buffer_mutex);

        /// Kill SNMP Trap Daemon
        kill(pid, SIGINT);

        printf("Waiting for snmptrapd to be closed.\n");
        waitpid(pid, &cstatus, 0);
        
        return EXIT_SUCCESS;
    }
    // Child:
    else 
    {
        FILE *fp = fopen("snmptrapd.conf", "w");

        fprintf(fp, "authCommunity log,execute,net %s", argument_str_array[1]);
        fclose(fp);

        sds binary_path_str = sdsnew("/usr/bin/snmptrapd");
        /*binary_path_str = sdscatsds(binary_path_str, argument_str_array[0]);
        binary_path_str = sdscat(binary_path_str, "external/snmptrapd");
        binary_path_str = sdsRemoveFreeSpace(binary_path_str);
        */

        close(pipefd[PIPE_READ_END]);

        if (dup2(pipefd[PIPE_WRITE_END], STDOUT_FILENO) == -1)
        {
            perror("dup2");
            return (void*)EXIT_FAILURE;
        }

        close(pipefd[PIPE_WRITE_END]);

        if(access(binary_path_str, X_OK) != 0)
        {
            printf(KRED "[ERROR] Can't run file: %s" KNORMAL, binary_path_str);
            return (void*)EXIT_FAILURE;
        }

        // sudo snmptrapd -C -c snmptrapd.conf -Lo -f -t -n -F "%#04y-%#02m-%#02lT%#02h:%#02j:%#02k+00:00 %a\n"
        sds format_str = sdsnew("\"%#04y-%#02m-%#02lT%#02h:%#02j:%#02k+00:00 %a\n\"");
        execl(binary_path_str, binary_path_str, "-C", "-c", "snmptrapd.conf", "-Lo", "-f", "-t", "-n", "-F", format_str , NULL);
        

        return EXIT_SUCCESS;
    }
}

void snmp_trap_wait_for_thread(void)
{
    pthread_mutex_lock(&run_loop_mutex);
    run_loop_mutexed = false;
    pthread_mutex_unlock(&run_loop_mutex);

    if(thread_used)
    {
        printf("Waiting for threads to be closed\n");
        pthread_join(thread, NULL);
    }
        
}

int snmp_trap_daemon_setup(sds* exec_path_str, sds* community_str)
{
    printf("Setting up SNMP Trap Daemon.\n");

    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        printf("[ERROR] snmp_trap_daemon_setup couldn't setup signal handling for SIGINT\n");
        return EXIT_FAILURE;
    }

    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        printf("[ERROR] snmp_trap_daemon_setup couldn't setup signal handling for SIGTERM\n");
        return EXIT_FAILURE;
    }

    ip_buffer = (ipv4_t *)calloc(IP_BUFFER_SIZE, sizeof(ipv4_t));

    argument_str_array[0] = sdsnew(*exec_path_str);
    argument_str_array[1] = sdsnew(*community_str);

    pthread_create(&thread, NULL, snmp_trap_daemon_thread, NULL);
    thread_used = true;

    printf("Listening to SNMP Traps.\n");

    return EXIT_SUCCESS;
}