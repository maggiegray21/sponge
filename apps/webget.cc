/*
 *
 * Filename: webget.cc
 * Author: Maggie Gray
 * Description: This file implements the get_URL function which allows the
 * user to fetch Web pages over the Internet.
 *
 * */

//#include "socket.hh"
#include "tcp_sponge_socket.hh"
#include "util.hh"

#include <cstdlib>
#include <iostream>

using namespace std;

/*
 *
 * Function Name: get_URL
 * Args: const string &host, const string &path
 * Description: This function takes a host server and a path and
 * connects the user to that server and requests the URL path from
 * that server using HTTP. It then prints out the server's response.
 *
 * */
void get_URL(const string &host, const string &path) {
    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).

    // create a socket
    //TCPSocket sock;
    CS144TCPSocket sock;

    // connect that socket to the host using HTTP
    sock.connect(Address(host, "http"));

    // request the URL path
    sock.write("GET ");
    sock.write(path);
    sock.write(" HTTP/1.1\r\nHost: ");
    sock.write(host);
    sock.write("\r\nConnection: close\r\n\r\n");

    // read and print the server's response until EOF
    while (!sock.eof()) {
        auto recvd = sock.read();
        cout << recvd;
    }

    sock.wait_until_closed();
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
