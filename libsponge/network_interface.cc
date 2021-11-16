#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

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
    EthernetFrame frame;
    if (opcode == OPCODE_REPLY) {
        arp_message.target_ethernet_address = IP_to_Ethernet[next_hop];
        frame = create_frame(arp.serialize(), IP_to_Ethernet[next_hop]);
    } else {
        frame = create_frame(arp.serialize(), ETHERNET_BROADCAST);
    }
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
            send_ARP_request(next_hop, OPCODE_REQUEST);
        } else if (dgrams_to_send[next_hop].second > 5 * 1000) {
            dgrams_to_send[next_hop].second = 0;
            send_ARP_request(next_hop, OPCODE_REQUEST);
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

void NetworkInterface::send_queued_datagrams(Address addr) {
    // queue of datagrams that need to be sent to IP Address addr
    queue<InternetDatagram> to_send = dgrams_to_send[addr].first;

    // ethernet address associated with IP address addr
    EthernetAddress eth = IP_to_Ethernet[addr].first;

    // send all dgrams in queue
    while (!to_send.empty()) {
        EthernetFrame frame = create_frame(to_send.front().serialize(), eth);
        _frames_out(frame);
        to_send.pop();
    }

    // remove entry from map
    dgrams_to_send.erase(addr);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {

    // ignore any frames not destined for the network interface
    if (frame.dst != _ethernet_address) return;

    // if the frame contains an IPv4 datagram in its payload, parse it and return the datagram
    if (frame.type = TYPE_IPv4) {
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) == ParseResult::NoError) {
            return dgram;
        }
    }

    // if the frame contains an ARP message in its payload, parse it, 
    // cache sender's IP address and Ethernet address, send any queued datagrams
    // reply if needed
    if (frame.type = TYPE_ARP) {
        ARPMessage arp;
        if (arp.parse(frame.payload() != ParseResult::NoError)) {
            return;
        }
        // map sender IP address to sender Ethernet address with TTL 30 seconds
        IP_to_Ethernet[arp.sender_ip_address] = (arp.sender_ethernet_address, 30 * 1000);

        // send any datagrams that were waiting to learn the ethernet address
        send_queued_datagrams(arp.sender_ip_address);

        // reply if necessary
        if (arp.opcode == OPCODE_REQUEST) {
            send_ARP_message(arp.sender_ip_address, OPCODE_REPLY);
        } 
    }

    // if frame is IPv4
        // parse as IPv4
        // return InternetDatagram

    // if frame is ARP
        // parse as ARP Message
        // remember mapping between the sender's IP address and Ethernet address for 30 seconds
        // if ARP request asks for our IP address:
            // send ARP reply

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 

    //queue<Address> to_remove; --> IF HAVING TROUBLE (LIKE SEG FAULTS), USE TO_REMOVE QUEUE
    // INSTEAD OF REMOVING INSIDE OF FOR LOOP

    // if IP Address to Ethernet address has been cached longer than 30 seconds, remove it
    // otherwise decrement TTL
    for (auto it = IP_to_Ethernet.begin(); it != IP_to_Ethernet.end(); it++) {
        if (it->second.second <= ms_since_last_tick) {
            //to_remove.push(it->first);
            IP_to_Ethernet.erase(it);
        } else {
            it->second.second -= ms_since_last_tick;
        }
    }

    // keep track of ms since ARP request sent for each IP address
    for (auto it = dgrams_to_send.begin(); it != dgrams_to_send.end(); it++) {
        if (it->second.second < 5) {
            it->second.second += ms_since_last_tick;
        }
    }
}
