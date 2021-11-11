#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // if FIN has been received and everything has been reassembled, return
    if (FIN_RECV && _reassembler.empty())
        return;

    // if we receive SYN, set SYN_RECV to true
    // and set isn to the seqno of SYN
    if (seg.header().syn) {
        SYN_RECV = true;
        isn = seg.header().seqno;
    }

    // if we have received SYN, then push the payload of the segment to the reassembler
    if (SYN_RECV) {
        // if we receive FIN, set FIN_RECV to true
        if (seg.header().fin) {
            FIN_RECV = true;
        }

        string toPush = seg.payload().copy();
        uint64_t index;

        // if the segment includes SYN, then the index of the first byte of the payload is
        // one more than the segment's seqno
        if (seg.header().syn) {
            index = unwrap(seg.header().seqno + 1, isn, _reassembler.stream_out().bytes_written()) - 1;
        } else {
            index = unwrap(seg.header().seqno, isn, _reassembler.stream_out().bytes_written()) - 1;
        }

        // push substring to the reassembler
        _reassembler.push_substring(toPush, index, seg.header().fin);
    }

    return;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    // if we have not received SYN, return empty
    if (!SYN_RECV) {
        return {};
    }

    // ackno is the number of bytes written by the byte stream plus one for the SYN
    uint64_t n = _reassembler.stream_out().bytes_written() + 1;

    // if we have received FIN and all bytes have been reassembled, then add one for the FIN
    if (FIN_RECV && _reassembler.empty()) {
        n++;
    }

    return wrap(n, isn);
}

size_t TCPReceiver::window_size() const { return _reassembler.stream_out().remaining_capacity(); }
