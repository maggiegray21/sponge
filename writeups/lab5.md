Lab 5 Writeup
=============

My name: Maggie Gray

My SUNet ID: mgray21

I collaborated with: N/A

I would like to thank/reward these classmates for their help: N/A

This lab took me about 5 hours to do. I did not attend the lab session.

Program Structure and Design of the NetworkInterface:

Data Structures:
    My network interface relies on two maps. One map (dgrams_to_send) maps IP addresses to pairs. Each pair 
    contains a queue of InternetDatagrams and a size_t which serves as a timer. Whenever the 
    network interface tries to send a datagram to an IP address that it does not know the 
    Ethernet address for, the datagram is added to the queue associated with the IP address
    in the map. Whenever an ARP request is sent asking about the IP address in the map, the
    size_t "timer" is reset to 0, to ensure that the network interface waits at least 5 seconds
    before sending a new ARP request asking about that IP address. Whenever a new ARP
    message is received by the interface, the interface learns the ethernet address associated
    with a given IP address and all datagrams waiting to be sent to that IP address are
    sent. I used a map make it easy to insert and delete map entries and to make it easy to
    look up map entries. I used a pair so that I could keep track of the amount of time since
    an ARP request was sent and all datagrams waiting without relying on a separate data
    structure. I used a queue because queues also allow for easy/fast insertion/deletion.

    My second map (IP_to_Ethernet) also maps IP addresses to pairs. The pairs contain an EthernetAddress
    and a size_t which is used as a timer. The map keeps track of all EthernetAddresses that
    we know of and which IP addresses they are connected to. The timer ensures that the 
    entries are only cached for 30 seconds. After 30 seconds the entries are deleted.
    Again, I used a map to make it easy to insert and delete map entries and to make
    it easy to look up map entries.

Global Variables
    My network interface relies on four global variables. Two of those variables, OPCODE_REPLY
    and OPCODE_REQUEST store the opcode values of ARP message replies and requests (2 and
    1 respectively). I also store the time to live of cache entries in the variable 
    CACHE_TTL (30 seconds or 30000 ms), and the amount of time before another ARP request may
    be sent about the same IP address (5 seconds or 5000 ms) in the variable ARP_REQUEST_TIMEOUT.

send_datagram:
    The send_datagram function takes in an InternetDatagram dgram and an IP Address
    next_hop. If we already know the ethernet address associated with next_hope (which would
    be stored in one of the maps described above), then we create an ethernet frame with dgram
    as the payload and send the frame to the next hop. If we do not know the ethernet
    address associated with next_hop, then we send an ARP request asking about the ethernet
    address associated with next_hop (if we have not sent one in the past 5 seconds), and we
    add dgram to the queue of datagrams waiting to be sent to next_hop.

tick:
    tick denotes the passage of time. Whenever time passes, tick deletes any elements in 
    our map IP_to_Ethernet that have been in the map at least 30 seconds. It also increments
    the amount of time since an ARP message asking about a specific IP address was sent.

Helper Functions:
    send_queued_datagrams takes in an IP address and, after an ARP message is received, sends
    all datagrams in our map dgrams_to_send that are waiting to be sent to the IP address addr.

    send_ARP_message takes in the IP address of our next hop and an opcode and sends an ARP
    message (either replying to next_hop or requesting next_hop's ethernet address).

    create_frame takes in a payload, an ethernet address addr, and a type and creates an EthernetFrame
    with that payload and type with dst set to addr.

recv_frame:
    recv_frame takes in an EthernetFrame frame. If frame is an IPv4 frame, then we return the
    datagram stored in its payload. If it is an ARP message, then we store the ethernet address
    and IP address of the message in our map IP_to_Ethernet. If the ARP message received
    is a request, then we respond to it (if it is requesting our EthernetAddress).


Implementation Challenges:
I had some trouble figuring out how to delete elements from a map while iterating over the
map without getting a SEGFAULT (I ultimately just saved all the elements I needed to delete
in a queue and deleted them after I was done iterating over the map, which isn't super
efficient).

Remaining Bugs:
None (that I know of).

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
