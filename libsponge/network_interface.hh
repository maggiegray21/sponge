/*
* File Name: network_interface.hh
* Author: Maggie Gray (mgray21)
* Description: Header file for network_interface.cc which implements a network interface.
*/

#ifndef SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
#define SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH

#include "ethernet_frame.hh"
#include "tcp_over_ip.hh"
#include "tun.hh"
#include "arp_message.hh"
#include "ethernet_header.hh"

#include <optional>
#include <queue>
#include <map>
#include <utility>

//! \brief A "network interface" that connects IP (the internet layer, or network layer)
//! with Ethernet (the network access layer, or link layer).

//! This module is the lowest layer of a TCP/IP stack
//! (connecting IP with the lower-layer network protocol,
//! e.g. Ethernet). But the same module is also used repeatedly
//! as part of a router: a router generally has many network
//! interfaces, and the router's job is to route Internet datagrams
//! between the different interfaces.

//! The network interface translates datagrams (coming from the
//! "customer," e.g. a TCP/IP stack or router) into Ethernet
//! frames. To fill in the Ethernet destination address, it looks up
//! the Ethernet address of the next IP hop of each datagram, making
//! requests with the [Address Resolution Protocol](\ref rfc::rfc826).
//! In the opposite direction, the network interface accepts Ethernet
//! frames, checks if they are intended for it, and if so, processes
//! the the payload depending on its type. If it's an IPv4 datagram,
//! the network interface passes it up the stack. If it's an ARP
//! request or reply, the network interface processes the frame
//! and learns or replies as necessary.
class NetworkInterface {
  private:
    //! Ethernet (known as hardware, network-access-layer, or link-layer) address of the interface
    EthernetAddress _ethernet_address;

    //! IP (known as internet-layer or network-layer) address of the interface
    Address _ip_address;

    //! outbound queue of Ethernet frames that the NetworkInterface wants sent
    std::queue<EthernetFrame> _frames_out{};

    // map from IP addresses to queue of datagrams and amount of time since ARP request asking about that IP address
    std::map<uint32_t, std::pair<std::queue<InternetDatagram>, size_t>> dgrams_to_send{};

    // map from IP addresses to Ethernet addresses and amount of time the value has been cached
    std::map<uint32_t, std::pair<EthernetAddress, size_t>> IP_to_Ethernet{};

    // creates an EthernetFrame
    EthernetFrame create_frame(BufferList payload, EthernetAddress addr, uint16_t type);

    // sends an ARP message asking about next_hop
    void send_ARP_message(uint32_t next_hop, uint16_t opcode);

    // after learning the ethernet address connected to an IP address, 
    // sends all datagrams waiting to go to that IP address
    void send_queued_datagrams(uint32_t addr);

    // global variables that hold the values for ARP message opcodes
    uint16_t OPCODE_REPLY = 2;
    uint16_t OPCODE_REQUEST = 1;

    // global variables that hold "time out" times for the time to remember a cached Ethernet address
    // to IP mapping and the amount of time to wait to send a new ARP request
    size_t CACHE_TTL = 30000; // 30 seconds
    size_t ARP_REQUEST_TIMEOUT = 5000; // 5 seconds

  public:
    //! \brief Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer) addresses
    NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address);

    //! \brief Access queue of Ethernet frames awaiting transmission
    std::queue<EthernetFrame> &frames_out() { return _frames_out; }

    //! \brief Sends an IPv4 datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination address).

    //! Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address for the next hop
    //! ("Sending" is accomplished by pushing the frame onto the frames_out queue.)
    void send_datagram(const InternetDatagram &dgram, const Address &next_hop);

    //! \brief Receives an Ethernet frame and responds appropriately.

    //! If type is IPv4, returns the datagram.
    //! If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
    //! If type is ARP reply, learn a mapping from the "sender" fields.
    std::optional<InternetDatagram> recv_frame(const EthernetFrame &frame);

    //! \brief Called periodically when time elapses
    void tick(const size_t ms_since_last_tick);
};

#endif  // SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
