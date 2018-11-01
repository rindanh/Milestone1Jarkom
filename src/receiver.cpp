#include <stdio.h>
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <thread>

#include "helpers.h"

#define STANDBY_TIME 3000

using namespace std;

int sock_fd;
struct sockaddr_in server_address, client_address;

void send_ack() {
	char ack[ACK_SIZE], frame[MAX_FRAME_SIZE], data[MAX_DATA_SIZE];
	int actual_frame_size;
	int actual_data_size;
	socklen_t cliend_address_size;

}