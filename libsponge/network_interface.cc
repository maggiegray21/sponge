#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"
#include "ethernet_header.hh"

#include <iostream>

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

EthernetFrame NetworkInterface::create_frame(BufferList payload, EthernetAddress addr, uint16_t type) {
    EthernetFrame frame;
    frame.header().type = type;
    frame.header().dst = addr; 
    frame.header().src = _ethernet_address;
    frame.payload() = payload;
    //cerr << "Created frame. DST = " << addr[0] << " SRC = " << _ethernet_address[0] << "\n";
    return frame;
}

void NetworkInterface::send_ARP_message(uint32_t next_hop, uint16_t opcode) {
    // create ARP message
    ARPMessage arp;
    arp.opcode = opcode;
    arp.sender_ethernet_address = _ethernet_address;
    arp.sender_ip_address = _ip_address.ipv4_numeric();
    arp.target_ip_address = next_hop;
    EthernetFrame frame;
    if (opcode == OPCODE_REPLY) {
        cerr << "About to try to access map elements\n";
        arp.target_ethernet_address = IP_to_Ethernet[next_hop].first;
        frame = create_frame(arp.serialize(), IP_to_Ethernet[next_hop].first, EthernetHeader::TYPE_ARP);
        cerr << "Reply ARP message created, map elements accessed\n";
    } else {
        frame = create_frame(arp.serialize(), ETHERNET_BROADCAST, EthernetHeader::TYPE_ARP);
    }
    _frames_out.push(frame);
    cerr << "ARP message pushed\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();

    // if we already know the Ethernet address of the next hop
    auto it1 = IP_to_Ethernet.find(next_hop_ip);
    if (it1 != IP_to_Ethernet.end()) {
        EthernetFrame frame = create_frame(dgram.serialize(), IP_to_Ethernet[next_hop_ip].first, EthernetHeader::TYPE_IPv4);
        _frames_out.push(frame);
    } else {
        queue<InternetDatagram> empty;
        // if ARP request asking about next_hop has not been sent in past 5 seconds
        // reset the number of seconds since ARP request sent to 0 and send ARP request
        auto it2 = dgrams_to_send.find(next_hop_ip);
        if (it2 == dgrams_to_send.end()) {
            cerr << "About to create map entry for dgrams_to_send\n";
            dgrams_to_send[next_hop_ip] = pair<queue<InternetDatagram>, size_t>(empty, 0);
            cerr << "Created map entry for dgrams_to_send\n";
            send_ARP_message(next_hop_ip, OPCODE_REQUEST);
        } else if (dgrams_to_send[next_hop_ip].second >= ARP_REQUEST_TIMEOUT) {
            dgrams_to_send[next_hop_ip].second = 0;
            send_ARP_message(next_hop_ip, OPCODE_REQUEST);
        }
        // add dgram to list of d_grams_to_send to IP Address next_hop
        dgrams_to_send[next_hop_ip].first.push(dgram);
        cerr << "Pushed dgram to map queue for IP address " << next_hop_ip << "\n";
        auto it3 = dgrams_to_send.find(next_hop_ip);
        if (it3 == dgrams_to_send.end()) {
            cerr << "Error: Dgram was not pushed\n";
        }
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

void NetworkInterface::send_queued_datagrams(uint32_t addr) {
    cerr << "About to send queued datagrams\n";
    // queue of datagrams that need to be sent to IP Address addr
    queue<InternetDatagram> to_send = dgrams_to_send[addr].first;

    // ethernet address associated with IP address addr
    EthernetAddress eth = IP_to_Ethernet[addr].first;

    // send all dgrams in queue
    while (!to_send.empty()) {
        EthernetFrame frame = create_frame(to_send.front().serialize(), eth, EthernetHeader::TYPE_IPv4);
        _frames_out.push(frame);
        to_send.pop();
    }

    // remove entry from map
    dgrams_to_send.erase(addr);
    cerr << "map entry deleted\n";
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    cerr << "Frame received\n";

    // ignore any frames not destined for the network interface
    if (frame.header().dst != _ethernet_address && 
        frame.header().dst != ETHERNET_BROADCAST) return {};

    // if the frame contains an IPv4 datagram in its payload, parse it and return the datagram
    if (frame.header().type == EthernetHeader::TYPE_IPv4) {
        cerr << "Frame of type IPV4\n";
        InternetDatagram dgram;
        if (dgram.parse(frame.payload()) == ParseResult::NoError) {
            return dgram;
        }
    }

    // if the frame contains an ARP message in its payload, parse it, 
    // cache sender's IP address and Ethernet address, send any queued datagrams
    // reply if needed
    if (frame.header().type == EthernetHeader::TYPE_ARP) {
        cerr << "ARP received\n";
        ARPMessage arp;
        if (arp.parse(frame.payload()) != ParseResult::NoError) {
            cerr << "Parse failed, about to return\n";
            return {};
        }
        // map sender IP address to sender Ethernet address with TTL 30 seconds
        //pair<EthernetAddress, size_t> pr = make_pair(arp.sender_ethernet_address, CACHE_TTL);
        IP_to_Ethernet[arp.sender_ip_address] = pair<EthernetAddress, size_t>(arp.sender_ethernet_address, CACHE_TTL);

        // send any datagrams that were waiting to learn the ethernet address
        send_queued_datagrams(arp.sender_ip_address);

        // reply if necessary
        if (arp.opcode == OPCODE_REQUEST && arp.target_ip_address == _ip_address.ipv4_numeric()) {
            cerr << "About to send reply\n";
            send_ARP_message(arp.sender_ip_address, OPCODE_REPLY);
        } 
    }

    return {};

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

    queue<uint32_t> to_remove; //--> IF HAVING TROUBLE (LIKE SEG FAULTS), USE TO_REMOVE QUEUE
    // INSTEAD OF REMOVING INSIDE OF FOR LOOP

    cerr << "About to try deleting things from IP_to_Ethernet\n";

    // if IP Address to Ethernet address has been cached longer than 30 seconds, remove it
    // otherwise decrement TTL
    for (auto it = IP_to_Ethernet.begin(); it != IP_to_Ethernet.end(); it++) {
        cerr << "entered for loop\n";
        if (it == IP_to_Ethernet.end()) {
            cerr << "Entered for loop even though we are at end 1\n";
        } 
        if (it->second.second <= ms_since_last_tick) {
            cerr << "About to try deleting an entry from IP_to_Ethernet\n";
            if (it == IP_to_Ethernet.end()) {
                cerr << "Entered for loop even though we are at end\n";
            } 
            to_remove.push(it->first);
            //IP_to_Ethernet.erase(it);
            if (it == IP_to_Ethernet.end()) {
                cerr << "deleted last element of IP_to_Ethernet, about to break\n";
                break;
            } 
        } else {
            cerr << "About to decrement it->second.second\n";
            it->second.second -= ms_since_last_tick;
            cerr << "Decremented it->second.second\n";
        }
    }

    while (!to_remove.empty()) {
        IP_to_Ethernet.erase(to_remove.front());
        to_remove.pop();
    }

    cerr << "Successfully deleted everything from IP_to_Ethernet\n";

    // keep track of ms since ARP request sent for each IP address
    for (auto it = dgrams_to_send.begin(); it != dgrams_to_send.end(); it++) {
        if (it->second.second < ARP_REQUEST_TIMEOUT) {
            it->second.second += ms_since_last_tick;
        }
    }

    cerr << "Successfully iterated through all of dgrams_to_send\n";
}
