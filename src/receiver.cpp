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
	int receiver_seq_num;
	bool frame_error;
	bool eot;

	while (true) {
		frame_size = recvfrom(sock_fd, (char *) frame, MAX_FRAME_SIZE, MSG_WAITALL, (struct sockaddr *) &client_address, &cliend_address_size);
		frame_error = read_frame(&receiver_seq_num, data, &data_size, &eot, frame);
		create_ack(receiver_seq_num, ack, frame_error);
		sendto(sock_fd, ack, ACK_SIZE, 0, (const struct sockaddr *) &client_address, client_address_size);
	}

}

int main(int argc, char *argv[]) {
	char *fname;
	int window_size;
	int max_buffer_size;
	int port;

	if (argc==5) {
		fname = argv[1];
		window_size = (int) atoi(argv[2]);
		max_buffer_size = MAX_DATA_SIZE * (int) atoi(argv[3]);
		port = atoi(argv[4]);
	} else {
		cerr << "use ./recvfile <filename> <window_size> <buffer_size> <port>" << endl;
		return 1;
	}

	server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; 
    server_addr.sin_port = htons(port);

    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("cannot create socket\n");
		return 0;
    }

    memset(&server_address, 0, sizeof(server_address));

    if (bind(socket_fd, (const struct sockaddr *)&server_addr, 
            sizeof(server_addr)) < 0) { 
        perror("bind failed");
		return 0;
    }

    FILE *file = fopen(fname, "wb");
    char buffer[max_buffer_size];
    int actual_buffer_size;

    char ack[ACK_SIZE];
    char frame[MAX_FRAME_SIZE];
    char data[MAX_DATA_SIZE];
    int actual_frame_size;
    int actual_data_size;
    int laf,lfr;
    int receiver_seq_num;
    bool eot;
    bool frame_error;

    memset(&client_addr, 0, sizeof(client_addr)); 

    bool receive_done = false;
    int buffer_num = 0;
    while (!receive_done) {
    	actual_buffer_size = max_buffer_size;
    	memset(buffer,0,actual_buffer_size);

    	int receiver_seq_count = (int) max_buffer_size / MAX_DATA_SIZE;
    	bool window_receiver_mask[window_size];
    	for (int i=0;i<window_size;i++){
    		window_receiver_mask[i] = false;
    	}
    	lfr = -1;
    	laf = lfr + window_size;

    	while (true) {
    		socklen_t cliend_address_size;
    		frame_size = recvfrom(sock_fd, (char *) frame, MAX_FRAME_SIZE, MSG_WAITALL, (struct sockaddr *) &client_address, &cliend_address_size);
			frame_error = read_frame(&receiver_seq_num, data, &data_size, &eot, frame);
			create_ack(receiver_seq_num, ack, frame_error);
			sendto(sock_fd, ack, ACK_SIZE, 0, (const struct sockaddr *) &client_address, client_address_size);

			if (receiver_seq_num <= laf) {
				if (!frame_error) {
					int buffer_shift = receiver_seq_num * MAX_DATA_SIZE;
					if (receiver_seq_num==lfr+1) {
						memcpy(buffer + buffer_shift, data, actual_data_size);

						int shift = 1;
						for (int i = 1; i < window_size; i++) {
                            if (!window_receiver_mask[i]) break;
                            shift += 1;
                        }
                        for (int i = 0; i < window_size - shift; i++) {
                            window_receiver_mask[i] = window_receiver_mask[i + shift];
                        }
                        for (int i = window_size - shift; i < window_size; i++) {
                            window_receiver_mask[i] = false;
                        }
                        lfr += shift;
                        laf = lfr + window_size;
					} else if (receiver_seq_num>lfr+1) {
						if (!window_receiver_mask[receiver_seq_num - (lfr + 1)]) {
                            memcpy(buffer + buffer_shift, data, actual_data_size);
                            window_receiver_mask[recv_seq_num - (lfr + 1)] = true;
                        }
					}

					if (eot) {
						buffer_size = buffer_shift + actual_data_size;
                        receiver_seq_count = receiver_seq_num + 1;
                        receive_done = true;
					}
				}
			}

			if (lfr >= receiver_seq_count - 1)
				break;
    	}
    	cout << "\r" << "[RECEIVED " << (unsigned long long) buffer_num * (unsigned long long) 
                max_buffer_size + (unsigned long long) actual_buffer_size << " BYTES]" << flush;
        fwrite(buffer, 1, actual_buffer_size, file);
        buffer_num += 1;

    }
    fclose(file);

    /* Start thread to keep sending requested ack to sender for 3 seconds */
    thread stdby_thread(send_ack);
    time_stamp start_time = current_time();
    while (elapsed_time(current_time(), start_time) < STANDBY_TIME) {
        cout << "\r" << "[STANDBY TO SEND ACK FOR 3 SECONDS | ]" << flush;
        sleep_for(100);
        cout << "\r" << "[STANDBY TO SEND ACK FOR 3 SECONDS / ]" << flush;
        sleep_for(100);
        cout << "\r" << "[STANDBY TO SEND ACK FOR 3 SECONDS - ]" << flush;
        sleep_for(100);
        cout << "\r" << "[STANDBY TO SEND ACK FOR 3 SECONDS \\ ]" << flush;
        sleep_for(100);
    }
    stdby_thread.detach();

    cout << "\nDONE" << endl;
    return 0;
}
