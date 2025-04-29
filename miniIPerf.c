#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_BUFFER_SIZE 1000
#define FIN_MESSAGE "FIN"
#define ACK_MESSAGE "ACK"

void error(const char *msg) {
    printf("Error: %s\n", msg);
}

void validatePortNum(const char *port) {
    int port_num = atoi(port);
    if (port_num < 1024 || port_num > 65535) {
        error("port number must be in the range of [1024, 65535]");
        exit(0);
    }
}

void validateTime(const char *time) {
    int time_num = atoi(time);
    if (time_num <= 0) {
        error("time argument must be greater than 0");
        exit(0);
    }
}

void validateFlag(const char *flag, const char* expected) {
    if (strcmp(flag, expected) != 0) {
        error("missing or extra arguments");
        exit(0);
    }
}

void serverStat(double x, double y) {
    printf("Received=%.0f KB, Rate=%.3f Mbps\n", x, y);
}

void clientStat(size_t x, double y) {
    printf("Sent=%zu KB, Rate=%.3f Mbps\n", x, y);
}

double to_mbps(size_t kiloBytes, double duration) {
    return (double)kiloBytes * 8 * 0.001 / duration;
}

double get_time_diff(struct timeval *start, struct timeval *end) {
    return (double)(end->tv_sec - start->tv_sec) + 
           (double)(end->tv_usec - start->tv_usec) * 1e-6;
}

int server(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return -1;
    }

    int optval = 1000000;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
                   &optval, sizeof(optval)) == -1) {
        close(sockfd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 1000) == -1) {
        close(sockfd);
        return -1;
    }

    int newsockfd = accept(sockfd, NULL, NULL);
    if (newsockfd == -1) {
        close(sockfd);
        return -1;
    }

    struct timeval start, end;
    gettimeofday(&start, NULL);

    char buffer[MAX_BUFFER_SIZE + 1];
    memset(buffer, '\0', MAX_BUFFER_SIZE + 1);

    ssize_t bytes_received;
    size_t total_bytes = 0;

    while ((bytes_received = recv(newsockfd, buffer, MAX_BUFFER_SIZE, 0)) > 0) {
        total_bytes += bytes_received;
        if (strstr(buffer, FIN_MESSAGE) != NULL) {
            send(newsockfd, ACK_MESSAGE, 4, 0);
        }
        memset(buffer, '\0', MAX_BUFFER_SIZE);
    }

    gettimeofday(&end, NULL);

    if (bytes_received == -1) {
        close(newsockfd);
        close(sockfd);
        return -1;
    } 

    double time_elapsed = get_time_diff(&start, &end);
    double kb_received = total_bytes / 1000.0;

    serverStat(kb_received, to_mbps(kb_received, time_elapsed));

    close(newsockfd);
    close(sockfd);
    return 0;
}

int client(const char *hostname, int port, int duration) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        return -1;
    }

    struct hostent *host = gethostbyname(hostname);
    if (!host) {
        close(sockfd);
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, host->h_addr, host->h_length);

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(sockfd);
        return -1;
    }

    char buffer[MAX_BUFFER_SIZE + 1];
    memset(buffer, '0', MAX_BUFFER_SIZE);

    struct timeval start, current;
    gettimeofday(&start, NULL);
    
    size_t total_kb_sent = 0;
    ssize_t bytes_sent;

    do {
        gettimeofday(&current, NULL);
        double elapsed = get_time_diff(&start, &current);
        
        if (elapsed >= duration) {
            bytes_sent = send(sockfd, FIN_MESSAGE, 4, 0);
            total_kb_sent += bytes_sent / 1000;
            char ack_buffer[4];
            if (recv(sockfd, ack_buffer, sizeof(ack_buffer), 0) > 0 && strstr(ack_buffer, ACK_MESSAGE) != NULL) {
                if (shutdown(sockfd, SHUT_WR) == -1) {
                    close(sockfd);
                    return -1;
                }
                gettimeofday(&current, NULL);
                break;
            };
        }

        bytes_sent = send(sockfd, buffer, MAX_BUFFER_SIZE, 0);
        if (bytes_sent == -1) {
            close(sockfd);
            return -1;
        }
        total_kb_sent += bytes_sent / 1000;
        
    } while (1);

    double time_elapsed = get_time_diff(&start, &current);
    clientStat(total_kb_sent, to_mbps(total_kb_sent, time_elapsed));
    close(sockfd);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 4 && argc != 8) {
        error("missing or extra arguments");
        return 0;
    }

    if (strcmp(argv[1], "-s") == 0) {
        if (argc != 4) {
            error("missing or extra arguments");
            return 0;
        }
        validateFlag(argv[2], "-p");
        validatePortNum(argv[3]);
        return server(atoi(argv[3]));
    } 
    
    if (strcmp(argv[1], "-c") == 0) {
        if (argc != 8) {
            error("missing or extra arguments");
            return 0;
        }
        validateFlag(argv[2], "-h");
        validateFlag(argv[4], "-p");
        validateFlag(argv[6], "-t");
        validatePortNum(argv[5]);
        validateTime(argv[7]);
        return client(argv[3], atoi(argv[5]), atoi(argv[7]));
    }

    error("missing or extra arguments");
    return 0;
}