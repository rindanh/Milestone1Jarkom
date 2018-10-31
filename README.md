# Milestone1Jarkom
Program yang akan dibuat terdiri dari dua file, yaitu sender dan receiver. Implementasi diwajibkan menggunakan bahasa C/C++ dengan protokol UDP. Program sender akan membaca suatu file dan mengirimnya ke receiver dengan menggunakan Sliding Window Protocol. Program receiver akan menerima data yang dikirim dan menuliskan file tersebut ke file system.

Berikut ini adalah format dari paket (frame) yang dikirim.

SOH
(0x1)
Sequence Number
Data Length
Data
Checksum
1 byte
4 byte
4 byte
Max 1024 byte
1 byte

Berikut ini adalah format dari ACK yang dikirim.
ACK


Next Sequence Number
Checksum
1 byte
4 byte
1 byte

Untuk mensimulasikan packet loss, anda dapat menggunakan tools pumba yang detailnya dapat dilihat pada https://github.com/alexei-led/pumba.

Untuk memudahkan penilaian, silahkan gunakan format berikut untuk menjalankan program.
./sendfile <filename> <windowsize> <buffersize> <destination_ip> <destination_port>
./recvfile <filename> <windowsize> <buffersize> <port>

