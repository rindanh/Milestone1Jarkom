#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netdb.h>

#include "helpers.h"

#define TIMEOUT 10

using namespace std;

int sock_fd;
struct sockaddr_in server_addr, client_addr;
int lar, lfs;

int win_length;
bool *win_ack_recv;
time_stamp *win_sent_time;

time_stamp TCurr = current_time();
mutex win_mutex;

void getACK() {
	int ack_size;
	char ack[ACK_size];
	int ack_seqnum;
	bool ack_err;
	bool ack_neg;

	while (true) {
		socklen_t server_addr_size;
		ack_size = recvfrom(sock_fd, (char *)ack, ACK_SIZE, MSG_WAITALL, (struct sockaddr *) &server_addr, &server_addr_size);
		ack_err = read_ack(&ack_seqnum, &ack_neg, ack);

		win_mutex.lock();

		if (!ack_err && ack_seqnum > lar && ack_seqnum <= lfs) {
			if (!ack_neg) {
				win_ack_recv[ack_seqnum - (lar + 1)] = true;
			} else {
				win_sent_time[ack_seqnum - (lar + 1)] = TCurr;
			}
		}

		win_mutex.unlock();
	}
}

int main (char *arg[], int countp) {
	int port_dest;
	struct hostent *host_dest;
	char *ip_dest;
	int max_buffer_size;

	/* Parameter memenuhi */
	if (countp == 6) {
		file_name = arg[1];
		ip_dest = arg[2];
		port_dest = arg[3];
		win_length = atoi(arg[4]);
		max_buffer_size = MAX_DATA_SIZE * (int) atoi(arg[5]);
	} else {
		cerr << "Please enter ./sendfile <filename> <destination_ip> <destination_port> <window_length> <buffer_size>" << endl;
	}

	/* Mendapatkan nama host dari IP address tujuan */
	host_dest = gethostbyname(ip_dest);
	if (!host_dest) {
		cerr << "Host is unidentified : " << ip_dest << endl;
		return 1;
	}

	/* Inisialisasi address pada sender dan receiver */

	memset(&server_addr, 0, sizeof(server_addr));
	memset(&client_addr, 0, sizeof(client_addr));

	/* Mengisi sender address (server address) dengan struktur data */
	server_addr.sin_family = AF_INET;
	// Mengcopy dengan address pertama dari host_dest
	bcopy(host_dest->h_addr, (char *) &server_addr.sin_addr, host_dest->h_length);
	server_addr.sin_port = htons(port_dest);

	/* Mengisi address pada receiver dengan struktur data */
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = INADDR_ANY;
	client_addr.sin_port = htons(0);

	/* Membuat socket */
	if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		cerr << "Socket is failed to be created" << endl;
		return 1;
	}

	/* Bind socket ke address receiver */

	if (::bind(sock_fd, (const struct sockaddr *)&client_addr, sizeof(client_addr)) < 0) {
		cerr < "Socket binding failed" << endl;
		return 1;
	}

	/* Pengecekan nama file */
	if (access(file_name, F_OK) == -1) {
		cerr < "File is unidentified : " << file_name << endl;
	}

	/* Membuka file yang akan dikirim */
	FILE *file = fopen(file_name, "rb");
	char buffer[max_buffer_size];
	int buffer_size;
	char frame[MAX_FRAME_SIZE];
	int frame_size;
	char data[MAX_DATA_SIZE];
	int data_size;

	/* Mencoba untuk mendapatkan ACK */
	thread thread_received(getACK);

	/* Mengirim file */
	bool donereading = false;
	int buffer_num = 0;
	while (!donereading) {

		/* Membaca sebagian file ke dalam variabel buffer, mengetahui ukuran packet sekali buffer dalam bentuk byte*/
		buffer_size = fread(buffer, 1, max_buffer_size, file);
		if (buffer_size == max_buffer_size) {
			char temp[1];
			int next_buffer_size = fread(temp, 1, 1, file);
			if (next_buffer_size == 0) {
				donereading = true;
			}
			int error = fseek(file, -1, SEEK_CUR);
		} else if (buffer_size < max_buffer_size) {
			donereading = true;
		}

		win_mutex.lock();

		/* Menginisialisasi variabel untuk sliding window */
		int seq_count = buffer_size / MAX_DATA_SIZE + ((buffer_size % MAX_DATA_SIZE == 0) ? 0 : 1);
		int seq_num;
		win_sent_time = new time_stamp[win_length];
		win_ack_recv = new bool [win_length];
		bool win_sent_recv[win_length];
		for (int i = 0; i < win_length; i++) {
			win_ack_recv[i] = false;
			win_sent_recv[i] = false;
		}
		lar = -1;
		lfs = lar + win_length;

		win_mutex.unlock();

		/* Mengirim buffer yang sekarang dengan sliding window */
		bool donesending = false;
		while (!donesending) {
			win_mutex.lock();

			/* Mengecek apakah window tsb sudah menerima ack atau belum, jika sudah, geser window */
			if (win_ack_recv[0]) {
				int shift = 1;
				for (int i = 1; i < win_length; i++) {
					if (!win_ack_recv[i]) {
						break;
					}
					shift += 1;
				}
				for (int i = 0; i < win_length - shift; i++) {
					win_sent_recv[i] = win_sent_recv[i + shift];
					win_ack_recv[i] = win_ack_recv[i + shift];
					win_sent_time[i] = win_sent_time[i + shift];
				}
				/* Menginisialisasi false pada semua yang telah berubah */
				for (int i = win_length - shift; i < win_length; i++) {
					win_sent_recv[i] = false;
					win_ack_recv[i] = false;
				}
				lar += shift;
				lfs = lar + win_length;
			}

			win_mutex.unlock();

			/* Mengirim frame yang belum terkirim dan mengirim ulang yang sudah melewati time out */
			for (int i = 0; i < win_length; i++) {
				seq_num = lar + i + 1;

				if (seq_num < seq_count) {
					win_mutex.lock();

					if (!win_sent_recv[i] || (!win_ack_recv[i] && (elapsed_time(current_time(), win_sent_time[i]) > TIMEOUT))) {
						int buffer_shift = seq_num * MAX_DATA_SIZE;
						data_size = (buffer_size - buffer_size < MAX_DATA_SIZE) ? (buffer_size - buffer_shift) : MAX_DATA_SIZE;
						memcpy(data, buffer + buffer_shift, data_size);

						bool eot = (seq_num == seq_count - 1) && (donereading);
						frame_size = create_frame(seq_num, frame, data, data_size, eot);

						sendto(sock_fd, frame, frame_size, 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));
						win_sent_recv[i] = true;
						win_sent_time[i] = current_time();
					}

					win_mutex.unlock();
				}
			}

			/* Pindah ke buffer selanjutnya */
			if (lar >= seq_count - 1) {
				donesending = true;
			}
		}

		/* cout << "\r" << "[SENT " << (unsigned long long) buffer_num * (unsigned long long) 
        max_buffer_size + (unsigned long long) buffer_size << " BYTES]" << flush;
        buffer_num += 1;
        if (read_done) break; */

	}

	fclose(file);
	delete [] win_ack_recv;
	delete [] win_sent_time;
	thread_received.detach();

	cout << "\nPacket transfer done successfully" << endl;

	return 0;
}