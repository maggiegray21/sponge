#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

EthernetFrame NetworkInterface::create_frame(BufferList payload, EthernetAddress addr) {
    EthernetFrame frame;
    frame.header().type =  EthernetHeader::TYPE_IPv4;
    frame.header().dst = addr; 
    frame.header().src = _ethernet_address;
    frame.payload() = payload;
    return frame;
}

void NetworkInterface::send_ARP_message(const Address next_hop, uint16_t opcode) {
    // create ARP message
    ARPMessage arp;
    arp.opcode = opcode;
    arp.sender_ethernet_address = _ethernet_address;
    arp.sender_ip_address = _ip_address;
    arp.target_ip_address = next_hop.ipv4_numeric;
    /*
    // fill in arp_message.target_ethernet_address if it is a reply
    if (opcode == 2) {

    }
    */

    // put ARP message into ethernet frame and send it
    EthernetFrame frame = create_frame(arp.serialize(), ETHERNET_BROADCAST);
    _frames_out.push(frame);
    
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    // if we already know the Ethernet address of the next hop
    auto it = IP_to_Ethernet.find(next_hop));
    if (it != IP_to_Ethernet.end()) {
        EthernetFrame frame = create_frame(dgram.serialize(), IP_to_Ethernet[next_hop].first);
        _frames_out.push(frame);
    } else {
        // if ARP request asking about next_hop has not been sent in past 5 seconds
        // reset the number of seconds since ARP request sent to 0 and send ARP request
        auto it = dgrams_to_send.find(next_hop);
        if (it == dgrams_to_send.end()) {
            dgrams_to_send[next_hop].second = 0;
            send_ARP_request(next_hop, 1);
        } else if (dgrams_to_send[next_hop].second > 5) {
            dgrams_to_send[next_hop].second = 0;
            send_ARP_request(next_hop, 1);
        }
        // add dgram to list of d_grams_to_send to IP Address next_hop
        dgrams_to_send[next_hop].first.push(dgram);
    }

    // if Ethernet address already known
        // create ethernet frame
        // set payload to be equal to the IP Datagram
        // set type = EthernetHeader::TYPE_IPv4
        // set dest and source addresses

    // if Ethernet address is unknown
        // queue IP datagram in MAP OF QUEUES!! map <IP address, queue>
        // if a request for this address has not been sent in the past 5 seconds
            // broadcast ARP request for next hop

}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {

    // ignore any frames not destined for the network interface

    // if frame is IPv4
        // parse as IPv4
        // return InternetDatagram

    // if frame is ARP
        // parse as ARP Message
        // remember mapping between the sender's IP address and Ethernet address for 30 seconds
        // if ARP request asks for our IP address:
            // send ARP reply

    DUMMY_CODE(frame);
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 

    // check if anything in mapping has been remembered for 30 seconds
    // update requests in past 5 seconds

}
