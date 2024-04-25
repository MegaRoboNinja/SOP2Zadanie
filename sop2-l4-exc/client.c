#include "l4-common.h"
#include <string.h>
#include <sys/epoll.h>

void usage(char *program_name) {
    fprintf(stderr, "USAGE: %s host port \n", program_name);
    exit(EXIT_FAILURE);
}
int main(int argc, char **argv) {
    int fd;
    int16_t sum;
    if (argc != 3)
    {
        usage(argv[0]);
        return EXIT_FAILURE;
    }
    fd = connect_tcp_socket(argv[1], argv[2]);

    pid_t pid = getpid(); // get the process ID
    char pid_str[20]; // buffer to hold the PID string

    if(sprintf(pid_str, "%d", pid)<0) // convert PID to string
        ERR("sprintf:");

    printf("Process %s has connected to server\n", pid_str);

    if (bulk_write(fd, pid_str, 20) < 0)
        ERR("write:");

    printf("Process %d: Sent PID to server\n", pid);
    if (bulk_read(fd, &sum, sizeof(int16_t)) < 0)
        ERR("sum read:");
    
    printf("Process %d: Received sum of the digits: %d\n", pid, sum);

    if (TEMP_FAILURE_RETRY(close(fd)) < 0)
        ERR("close");
    return EXIT_SUCCESS;
}