/*
 * Filename: stream_reassembler.hh
 * Author: Maggie Gray
 * Description: This file contains the header for a StreamReassembler which reassembles
 * unassembled data chunks to send to a ByteStream.
 */

#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <list>
#include <string>

using namespace std;

// This struct represents unassembled chunks of data. It includes the string of the data
// and the string's start and end index
struct Unassembled {
  public:
    size_t startIndex;
    size_t endIndex;
    string str;
    Unassembled() : startIndex{0}, endIndex{0}, str{""} {};
};

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    ByteStream _output;   //!< The reassembled in-order byte stream
    size_t _capacity;     //!< The maximum number of bytes
    size_t currentIndex;  //!< The next index the Reassembler needs to write to the ByteStream

    // a list of unassembled chunks of data
    list<Unassembled> unassembledList;

    // adds an Unassembled to the list of Unassembled chunks of data
    void addToList(Unassembled current);

    // checks to see if two Unassembled chunks of data overlap
    bool checkOverlaps(Unassembled u1, Unassembled u2);

    // constructs a new Unassembled chunk by merging the data of two
    // overlapping Unassembled chunks
    Unassembled constructOverlapping(Unassembled u1, Unassembled u2);

    // writes a string to the ByteStream
    void write_substring(Unassembled current);
    void protected_write(Unassembled current);

    // creates an Unassembled struct that stores the string data and its
    // start and end index
    Unassembled createUnassembled(const string &data, const size_t index);

    // stores the number of bytes of data currently unassembled and stored in
    // the Unassembled list
    size_t bytesInList;

    // records the maximum end index our list of Unassembleds can store
    size_t maxEndIndex;

    // records whether EOF has been pushed
    bool eofPushed;

    // records the index of EOF
    size_t eofIndex;

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity)
        : _output(capacity)
        , _capacity(capacity)
        , currentIndex(0)
        , unassembledList()
        , bytesInList(0)
        , maxEndIndex(0)
        , eofPushed(0)
        , eofIndex(0){};

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
