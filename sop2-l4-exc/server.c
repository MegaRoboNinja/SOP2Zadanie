#include "l4-common.h"
#include <string.h>
#include <sys/epoll.h>

#define BACKLOG_SIZE 10
#define MAX_CLIENT_COUNT 4
#define MAX_EVENTS 10

#define NAME_OFFSET 0
#define NAME_SIZE 64
#define MESSAGE_OFFSET NAME_SIZE
#define MESSAGE_SIZE 448
#define BUFF_SIZE 20

void usage(char *program_name) {
    fprintf(stderr, "USAGE: %s port key\n", program_name);
    exit(EXIT_FAILURE);
}

volatile int do_work = 1;

int set_nonblock(int);
void doServer(int serverSocket);
void manageClients(int serverSocket, struct epoll_event* ev, int epoll_fd, int* maxPIDsum);
void ReceiveMsg(char buffer[BUFF_SIZE], int clientSocket, int epoll_fd);
int DigitSum(char* buffer);

void sigint_handler() { do_work = 0; }

int main(int argc, char **argv) {
    // GET INPUT:
    char *program_name = argv[0];
    if (argc != 2) {
        usage(program_name);
    }

    uint16_t port = atoi(argv[1]);
    if (port == 0){
        usage(argv[0]);
    }

    // Create the server socket
    int serverSocket = bind_tcp_socket(port, BACKLOG_SIZE);

    if (sethandler(sigint_handler, SIGINT))
        ERR("Seting SIGINT:");

    int new_flags = fcntl(serverSocket, F_GETFL) | O_NONBLOCK;
    fcntl(serverSocket, F_SETFL, new_flags);

    doServer(serverSocket);

    // Close the socket
    close(serverSocket);
    fprintf(stderr, "Server has terminated.\n");
    return EXIT_SUCCESS;
}

void doServer(int serverSocket)
{
    int maxPIDsum = -1;

    // Create an epoll instance 
    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) < 0)
        ERR("epoll_create:");
    // Create an event that occurs when sb tries to connect to the server
    struct epoll_event ev;
    ev.data.fd = serverSocket;
    ev.events = EPOLLIN; // enables reading
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, serverSocket, &ev) < 0)
    {
        perror("epoll_ctl: listen_sock");
        exit(EXIT_FAILURE);
    }

    // each field in the array holds information about:
    //      .event - type of event as a bitmask
    //      .data  - ie fd associated with the event
    struct epoll_event events[MAX_EVENTS];

    sigset_t mask, oldmask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

    while(do_work)
    {
        printf("Calling epoll_wait()!\n");
        // returns how many events occured and puts them in events array
        int evNumber = epoll_pwait(epoll_fd, events, MAX_EVENTS, -1, &oldmask);
        if (evNumber < 0) 
            ERR("epoll_pwait");

        for (int i = 0; i < evNumber; ++i) { // for each event
            int fd = events[i].data.fd; // fd to socket the event is associated with
            // this event is associated with the server
            // so it means that sb is trying to connect
            manageClients(fd, &ev, epoll_fd, &maxPIDsum);
        }
    }

    printf("Max sum of digits in PID: %d\n", maxPIDsum);
}

void manageClients(int serverSocket, struct epoll_event* ev, int epoll_fd, int* maxPIDsum)
{
    // accept the new client
    int clientSocket = add_new_client(serverSocket);
    printf("Client accepted\n");
    if (clientSocket < 0)
        ERR("accept()");

    // Get the message
    char buffer[BUFF_SIZE];
    ReceiveMsg(buffer, clientSocket, epoll_fd);
    printf("Received message: %s\n", buffer);

    // Update max digit sum if needed
    int16_t s = DigitSum(buffer);
    if (s > (*maxPIDsum))
        (*maxPIDsum) = s;
    printf("Max sum of digits in PID: %d\n", *maxPIDsum);

    // Send back the sum of the digits
    if(bulk_write(clientSocket, (char*)&s, sizeof(int16_t)) < 0)
        ERR("write:");
    printf("sending back the sum...\n");
}

void ReceiveMsg(char buffer[BUFF_SIZE], int clientSocket, int epoll_fd)
{
    // Receive the authentication message
    ssize_t size = bulk_read(clientSocket, buffer, BUFF_SIZE);
    if (size == 0 || (size == -1 && errno == ECONNRESET)) {
        //client disconnected
        printf("Clietn disconnected\n");
        if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clientSocket, NULL) < 0)
            ERR("epoll_ctl");
        close(clientSocket);
    }
}

int DigitSum(char* buffer)
{
    int i = 0, sum = 0;
    while(buffer[i] != '\0')
    {
        sum += buffer[i++] - '0';
    }
    return sum;
}

int set_nonblock(int desc) {
  int oldflags = fcntl(desc, F_GETFL, 0);
  if (oldflags == -1)
    return -1;
  oldflags |= O_NONBLOCK;
  return fcntl(desc, F_SETFL, oldflags);
}