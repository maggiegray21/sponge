/*
 *
 * Filename: byte_stream.cc
 * Author: Maggie Gray
 * Description: This file implements a bytestream that users can write into
 * and read from.
 *
 * */

#include "byte_stream.hh"

#include <iostream>

using namespace std;

/*
 *
 * Function Name: write
 * Args: const string &data
 * Return: size_t, number of bytes written
 * Description: This function takes a string of data
 * and stores it in the string "input" if the bytestream
 * has capacity left to store data.
 *
 * */
size_t ByteStream::write(const string &data) {
    size_t numWritten = 0;

    // add each character in the data string to the input string
    // if the bytestream has capacity left
    for (int i = 0; i < static_cast<int>(data.length()); i++) {
        if (capacityLeft > 0) {
            input += data[i];
            numWritten++;
            capacityLeft--;
        }
    }
    bytesWritten += numWritten;
    return numWritten;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    // check to see if there are len bytes left in input
    if (input.length() < len) {
        return input;
    }
    return input.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    // check to see if there are len bytes left in input
    if (input.length() < len) {
        input = "";
        bytesRead += input.length();
        capacityLeft += input.length();
    } else {
        input = input.substr(len, input.length() - len);
        bytesRead += len;
        capacityLeft += len;
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string output = peek_output(len);
    pop_output(len);
    return output;
}

void ByteStream::end_input() { inputEnded = true; }

bool ByteStream::input_ended() const { return inputEnded; }

size_t ByteStream::buffer_size() const { return input.length(); }

bool ByteStream::buffer_empty() const { return (buffer_size() == 0); }

bool ByteStream::eof() const { return (input_ended() && buffer_empty()); }

size_t ByteStream::bytes_written() const { return bytesWritten; }

size_t ByteStream::bytes_read() const { return bytesRead; }

size_t ByteStream::remaining_capacity() const { return capacityLeft; }
