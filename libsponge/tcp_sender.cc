#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <iostream>
#include <random>

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , rto{retx_timeout}
    , _stream(capacity)
    , outstanding_segments()
    , l_edge(0)
    , r_edge(1)
    , outstanding(0)
    , t()
    , consecutive(0)
    , windowSize(1) {}

// the number of bytes_in_flight is equal to the number of bytes stored in our
// list of outstanding segments
uint64_t TCPSender::bytes_in_flight() const { return outstanding; }

/*
 * Function Name: addToOutstanding
 * Args: TCPSegment seg
 * Description: This function takes in a TCPSegment seg and adds it to the sorted list of
 * outstanding segments that have been sent to the TCP Receiver but not yet
 * acknowledged.
 */
void TCPSender::addToOutstanding(TCPSegment seg) {
    // keep track of how many bytes are outstanding or "in flight"
    outstanding += seg.length_in_sequence_space();

    // go through each element of outstanding_segments
    // insert seg into list in sorted fashion
    for (auto it = outstanding_segments.begin(); it != outstanding_segments.end(); it++) {
        TCPSegment next = *it;
        if (seg.header().seqno.raw_value() < next.header().seqno.raw_value()) {
            outstanding_segments.insert(it, seg);
            return;
        }
    }

    // if outstanding_segments is empty, or seg's seqno is larger than all outstanding segments
    // push seg to back of list
    outstanding_segments.push_back(seg);
}

/*
 * Function Name: construct_TCPSegment
 * Args: int length
 * Description: This function takes in the desired length of a TCP Segment
 * and constructs a TCP Segment with the next sequence number needed.
 */
TCPSegment TCPSender::construct_TCPSegment(int length) {
    // initialize a TCP segment
    TCPSegment seg;

    // set the sequence number to be the next sequence number
    seg.header().seqno = wrap(_next_seqno, _isn);

    // set SYN to true if _next_seqno = 0 and decrement length
    if (_next_seqno == 0) {
        seg.header().syn = true;
        length--;
    }

    // read up to MAX_PAYLOAD_SIZE bytes from the Byte Stream
    // set payload of seg to be bytes read
    if (length > 0 && !_stream.buffer_empty()) {
        seg.payload() = Buffer(_stream.read(min(static_cast<size_t>(length), TCPConfig::MAX_PAYLOAD_SIZE)));
        length -= seg.payload().copy().length();
    }

    // set EOF Byte Stream has reached EOF
    if (_stream.eof() && length > 0) {
        seg.header().fin = true;
    }

    return seg;
}

/*
 * Function Name: fill_window()
 * Description: If there is space left in our window, this function
 * constructs TCP segments and sends them to the TCP Receiver.
 */
void TCPSender::fill_window() {
    // if we do not have space left in our window, return
    if (l_edge.raw_value() == r_edge.raw_value())
        return;

    // if windowSize is equal to 0, act as if we have windowSize of 1
    // push one byte to _segments_out
    if (windowSize == 0) {
        // construct seg and add to _segments_out and our list of outstanding segments
        TCPSegment seg = construct_TCPSegment(1);
        safe_push_segment(seg);

        // reset timer if needed
        if (t.expired()) {
            t.start(rto);
        }

    } else {
        // while we have space in our window, construct TCP segments and send them
        // to the TCP receiver
        while (l_edge != r_edge) {
            size_t spaceLeft = r_edge.raw_value() - l_edge.raw_value();

            // return if we have already reached FIN
            if (_next_seqno == stream_in().bytes_written() + 2) {
                return;
            }
            TCPSegment seg = construct_TCPSegment(min(TCPConfig::MAX_PAYLOAD_SIZE + 2, spaceLeft));

            // return if we can only construct an empty segment (payload empty, no SYN, no FIN)
            if (seg.length_in_sequence_space() == 0) {
                return;
            }

            // add segment to outstanding list and push to TCP Reiver
            safe_push_segment(seg);

            // restart timer if necessary
            if (!t.running()) {
                t.start(rto);
            }
        }
    }
}

// this function pushes TCP Segments to _segments_out, and, if they occupy length
// in sequence space, then it adds them to the list of outstanding segments
// and increments _next_seqno and l_edge
void TCPSender::safe_push_segment(TCPSegment seg) {
    _segments_out.push(seg);

    // if the segment has a payload or SYN/FIN, add it to outstanding and increment
    // _next_seqno and l_edge
    if (seg.length_in_sequence_space() > 0) {
        addToOutstanding(seg);

        // increment _next_seqno and l_edge
        _next_seqno += seg.length_in_sequence_space();
        l_edge = l_edge + static_cast<uint32_t>(seg.length_in_sequence_space());
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    // if the ackno is greater than next seqno, return
    if (ackno.raw_value() > wrap(_next_seqno, _isn).raw_value())
        return;

    // if the ackno is smaller than next seqno, return
    if (ackno.raw_value() + window_size < wrap(_next_seqno, _isn).raw_value())
        return;

    // set l_edge and r_edge
    l_edge = wrap(_next_seqno, _isn);
    r_edge = ackno + window_size;

    // record windowSize
    windowSize = window_size;

    // if windowSize = 0, act as if it is equal to 1
    if (windowSize == 0) {
        r_edge = r_edge + 1;
    }

    // initialize bool to record whether any new data is removed from our outstanding list
    bool dataAcked = false;

    // iterate over outstanding list
    // if a segment has been acknowledged, remove it from the list and set dataAcked to true
    auto it = outstanding_segments.begin();
    while (it != outstanding_segments.end()) {
        TCPSegment seg = *it;
        if (seg.header().seqno.raw_value() + seg.length_in_sequence_space() <= ackno.raw_value()) {
            dataAcked = true;
            it = outstanding_segments.erase(it);
            outstanding -= seg.length_in_sequence_space();
        } else {
            break;
        }
    }

    // if we have acknoledged new data, reset RTO and restart timer if there
    // is still outstanding datat
    if (dataAcked) {
        rto = _initial_retransmission_timeout;
        consecutive = 0;
        if (!outstanding_segments.empty()) {
            t.start(rto);
        } else {
            t.stop();
        }
    }
}

/*
 * Function Name: start
 * Args: size_t time
 * Description: This function starts the timer running with time "time."
 */
void Timer::start(size_t time) {
    _expired = false;
    _running = true;
    timeLeft = time;
}

/*
 * Function Name: timePass
 * Args: size_t time
 * Description: This function tells the timer that time "time" has passed
 * so that the timer can decrement its time remaining.
 */
void Timer::timePass(size_t time) {
    if (time >= timeLeft) {
        _expired = true;
    } else {
        timeLeft -= time;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    // tell the timer that time has passed
    t.timePass(ms_since_last_tick);

    // if our timer has expired
    if (t.expired()) {
        // resend oldest segment
        if (!outstanding_segments.empty()) {
            _segments_out.push(outstanding_segments.front());
            // if we still have space in our window, increment consecutive and double RTO
            if (windowSize > 0) {
                consecutive++;
                rto = rto * 2;
            }
        }
        // restart timer
        t.start(rto);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return consecutive; }

// this function sends an empty segment
void TCPSender::send_empty_segment() {
    TCPSegment seg = construct_TCPSegment(0);
    safe_push_segment(seg);
}
