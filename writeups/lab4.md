Lab 4 Writeup
=============

My name: Maggie Gray

My SUNet ID: mgray21

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [15] hours to do. I [did] attend the lab session.

Program Structure and Design of the TCPConnection:
segment_received:
    The segment_received funciton takes in a TCPSegment and sends the segment to the _receiver.
    It resets time_since_segment_received to 0 each time it receives a segment, and it keeps
    track of whether or not the connection needs to linger after both streams finish at the end.
    If the segment received has the ACK flag set, it sends the ackno and window size to the
    TCP Sender. It ensures that segments are sent in response to every segment received.

active():
    active() returns true if the TCP Connection is still active and false otherwise. We know 
    that the TCP Connection is still active if:
       1) the inbound stream is not ended and the _receiver still has unassembled bytes
       2) the outbound stream is not at EOF or the FIN segment has not yet been sent
       3) there are still bytes in flight
    If the three prerequisites are false, then active() will return false after lingering
    for 10 times the initial retransmission timer. If the TCP Connection does not need
    to linger (the inbound stream ended before the _sender sent FIN) and the three 
    prerequisites are false, then active() will return false.

create_segment()
    create_segment() pops a TCP segment off the _sender's segments_out queue. It sets the window
    size of the segment, and if SYN has been received, it sets the ACK and ackno of the segment
    (all of which it gets from the _receiver). create_segment() returns the created segment.

send_rst()
    send_rst() creates and sends a segement with the RST flag set. It also sets both byte streams
    to error.

send_segments()
    send_segments() sends all segments on the _sender's segments_out queue to the connection's
    _segments_out queue. It sends a segment with the RST flag set if there have been too many
    consecutive retransmissions of a segment.

tick()
    tick() calls the _sender's tick function and sends any segments that the _sender may have
    retransmitted. It also increments the time_since_segment_received.

Implementation Challenges:
I had some trouble figuring out how to handle a window size of 0. I had to ensure that when a 
window size of 0 was advertised, EXACTLY 1 segment would be sent by the sender (no more,
no less). 

Remaining Bugs:
None that I know of

- Optional: I had unexpected difficulty with: I had a lot of trouble with my VM 
during this assignment. The assignment would have taken me MUCH less time, but I 
spent like 6 hours trying to get my VM to work. First, the memory filled up, and 
it took me a while to figure out how to free up memory. Then, my VM stopped 
connecting to SSH. I hadn't saved my code to a private GitHub repo or a shared 
folder (a mistake I will not make again), and I couldn't copy and paste from my
VM (I hadn't set up shared clipboard, and needed a network connection on the VM to
set it up, which I did not have), so I ended up having to retype my entire TCP 
Connection file and remake a new VM. I definitely learned some lessons from that
experience and will always make sure to have a GitHub repo with my code in the 
future. 

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
