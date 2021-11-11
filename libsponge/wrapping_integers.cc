#include "wrapping_integers.hh"

#include <cmath>
#include <iostream>
#include <math.h>

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    uint32_t wrapped = isn.raw_value() + n;
    return WrappingInt32(wrapped);
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    // calculate the zero-indexed 32-bit sequence number
    uint32_t offset = n.raw_value() - isn.raw_value();

    // if checkpoint is less than 2^31, then just return the zero-indexed 32-bit sequence number
    if (checkpoint < pow(2, 31)) {
        return offset;
    }

    // set 32 least significant bits to 0
    uint64_t unwrapped = (checkpoint >> 32) << 32;

    // add 32-bit sequence number to unwrapped
    unwrapped += offset;
    uint64_t power = pow(2, 31);

    // if the difference between unwrapped and checkpoint is more than 2^31
    // add or subtract 2^32
    if (unwrapped > checkpoint) {
        if (unwrapped - checkpoint > power) {
            unwrapped -= static_cast<uint64_t>(1) << 32;
        }
    } else {
        if (checkpoint - unwrapped > power) {
            unwrapped += static_cast<uint64_t>(1) << 32;
        }
    }

    return unwrapped;
}
