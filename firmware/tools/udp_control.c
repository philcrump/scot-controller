
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>


static uint64_t timestamp_ms(void)
{
    struct timespec tp;

    if(clock_gettime(CLOCK_REALTIME, &tp) != 0)
    {
        return 0;
    }

    return (uint64_t) tp.tv_sec * 1000 + tp.tv_nsec / 1000000;
}

static void udp_client(uint16_t port, void (*_rx_callback)(uint64_t, uint8_t *, size_t))
{
    int n;

    /* Open UDP Socket */
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
    {
        /* Incoming socket failed to open */
        return;
    }

    /* Add REUSEADDR flag to allow multiple clients on the socket */
    int optval = 1;
    n = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
    if(n < 0)
    {
        /* Failed to set SO_REUSEADDR */
        close(sockfd);
        return;
    }

    /* Set the RX buffer length */
    optval = 256;
    n = setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &optval, sizeof(optval));
    if(n < 0)
    {
        /* Failed to set buffer size */
        close(sockfd);
        return;
    }

    /* Set up server address */
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    /* Bind Socket to server address */
    if (bind(sockfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
    { 
        /* Socket failed to bind */
        close(sockfd);
        return;
    }

    /* Allocate receive buffer */
    uint8_t *udp_client_buffer = (uint8_t*)malloc(256);
    if(udp_client_buffer == NULL)
    {
        /* buffer failed to be allocated */
        close(sockfd);
        return;
    }

    /* Infinite loop, blocks until incoming packet */
    uint64_t timestamp;
    while (1)
    {   
        /* Block here until we receive a packet */
        n = recv(sockfd, udp_client_buffer, 256, 0);
        if (n < 0)
        {
            /* Incoming recv failed */
            continue;
        }

        timestamp = timestamp_ms();

        _rx_callback(timestamp, udp_client_buffer, n);
    }
    
    free(udp_client_buffer);
    close(sockfd);
}

static void udp_localhost_send(uint16_t port, uint8_t *buffer, size_t buffer_size)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock < 0)
    {
        return;
    }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server.sin_port = htons(port);

    if(sendto(sock, buffer, buffer_size, 0, (const struct sockaddr *)&server, sizeof(struct sockaddr_in)) < 0)
    {
        fprintf(stderr, "udp_localhost_send, sendto error: %s\n", strerror(errno));
    }

    close(sock);
}

uint32_t counter=0;
void udp_rx_cb(uint64_t timestamp_ms, uint8_t *udp_buffer, size_t udp_buffer_size)
{
    //printf("[UDP] %ld: %ldB\n", timestamp_ms, udp_buffer_size);

    uint32_t tx_timestamp_ms;
    tx_timestamp_ms = *(uint32_t *)&(udp_buffer[0]);

    uint32_t hours = tx_timestamp_ms / (60*60*1000);
    uint32_t minutes = (tx_timestamp_ms - (hours * 60*60*1000)) / (60*1000);
    double seconds = (double)(tx_timestamp_ms - ((hours * 60*60*1000) + (minutes * 60*1000))) / (1000.0);
    //printf(" - %dH, %dM, %.3fs (%d)\n",
    //    hours,
    //    minutes,
    //    seconds,
    //    tx_timestamp_ms
    //);

    int64_t diff = ((int64_t)timestamp_ms-((timestamp_ms /100000)*100000)) - ((int64_t)tx_timestamp_ms-((tx_timestamp_ms /100000)*100000));

    uint32_t sid = *(uint32_t *)&(udp_buffer[4]);

    if(sid == 0x010)
    {
        /* Azimuth Resolver Position */
        uint16_t position_raw = *(uint16_t *)&(udp_buffer[8]);
        double position_degrees = (double)position_raw * (360.0 / 65536.0);
        //printf(" - %.2f° (%d)\n",
        //    position_degrees, position_raw
        //);

        printf("%d,%+ld,%.2f°,%d\n\n", counter++, diff,
             position_degrees, position_raw);
    }
}

int main(void)
{
    udp_client(11234, udp_rx_cb);

    return 0;
}
