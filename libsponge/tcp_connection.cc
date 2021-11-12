#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return time_since_segment_received; }

void TCPConnection::segment_received(const TCPSegment &seg) {

    // if SYN has not been sent and this segment does not have SYN set, return
    if (_sender.next_seqno_absolute() == 0 && !seg.header().syn) return;

    // reset time_since_segment_received
    time_since_segment_received = 0;

    // if RST flag is set, set errors on both byte streams
    if (seg.header().rst) {
        _receiver.stream_out().set_error();
        _sender.stream_in().set_error();
        reset = true;
    }

    // sends segment to receiver
    _receiver.segment_received(seg);

    if (_receiver.stream_out().input_ended() && !_sender.stream_in().eof()) {
        _linger_after_streams_finish = false;
    }

    // if ACK is set, send ackno and window_size to _sender
    if (seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
        if ((seg.header().ackno == seg.header().seqno) && seg.header().win == 0) return; 
        safe_fill_window();
    }

    // handles "keep alives" 
    if (_receiver.ackno().has_value() && (seg.length_in_sequence_space() == 0)
    && seg.header().seqno == _receiver.ackno().value() - 1) {
        _sender.send_empty_segment();
    }

    // if segment has length in sequence space, ensure a segment is sent
    if (seg.length_in_sequence_space() > 0 && _sender.segments_out().empty()) {
        _sender.send_empty_segment();
    }

    send_segments();
}

bool TCPConnection::active() const {

    // if a segment with RST flag has been sent or received, return false
    if (reset) return false;

    // if inbound stream is not ended or there are still unassembled bytes, return true
    if (!_receiver.stream_out().input_ended() || _receiver.unassembled_bytes() != 0) return true;

    // if outbound stream is not at EOF or FIN has not been sent, return true
    if (!_sender.stream_in().eof() ||
        _sender.next_seqno_absolute() != _sender.stream_in().bytes_written() + 2) return true;

    // if there are still bytes in flight, return true
    if (_sender.bytes_in_flight() != 0) return true;

    // if the three above conditions are true, and we do not need to linger, return false
    if (!_linger_after_streams_finish) return false;

    // if it has been 10 times the initial retransmission timeout, return false
    if (time_since_segment_received >= (10 * _cfg.rt_timeout)) return false;

    return true;

}

TCPSegment TCPConnection::create_segment() {
    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();

    // if SYN has been received, set ackno and ACK
    if (_receiver.ackno().has_value()) {
        seg.header().ackno = _receiver.ackno().value();
        seg.header().ack = true;
    }

    // set window size
    seg.header().win = min(_receiver.window_size(), static_cast<size_t>(UINT16_MAX));
    return seg;
}

void TCPConnection::send_rst() {
    // create an empty segment with RST = true if there are no segments waiting to be sent
    if (_sender.segments_out().empty()) _sender.send_empty_segment();
    TCPSegment seg = create_segment();
    seg.header().rst = true;
    _segments_out.push(seg);

    // set both byte streams to error
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();

    // set reset to true
    reset = true;
}

void TCPConnection::send_segments() {
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        send_rst();
    }

    while (!_sender.segments_out().empty()) {
        TCPSegment seg = create_segment();
        _segments_out.push(seg);
    }
}

void TCPConnection::safe_fill_window() {
    _sender.fill_window();
    send_segments();
}


size_t TCPConnection::write(const string &data) {
    // send the _sender data to put into TCP segments
    size_t numWritten = _sender.stream_in().write(data);
    safe_fill_window();
    return numWritten;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    // tells _sender that time has passed
    _sender.tick(ms_since_last_tick);
    send_segments();

    // increment time_since_segment_received
    time_since_segment_received += ms_since_last_tick;
}

// ends outbound byte stream
void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    safe_fill_window();
}

void TCPConnection::connect() {
    // _sender's window size is initialized to 1, so calling fill_window() will
    // cause _sender to send an empty segment with SYN flag set
    safe_fill_window();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            send_rst();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
