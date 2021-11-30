#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    Route r(get_prefix(route_prefix, prefix_length), prefix_length, next_hop, interface_num);
    /*
    r.prefix = get_prefix(route_prefix, prefix_length);
    r.prefix_length = prefix_length;
    r.next_hop = next_hop;
    r.interface_num = interface_num;
    */
    routing_table.push_back(r);
}

uint32_t Router::get_prefix(uint32_t route_prefix, uint8_t prefix_length) {
    if (prefix_length == 32) return route_prefix;
    if (prefix_length == 0) return 0;
    route_prefix = route_prefix >> (32 - prefix_length);
    route_prefix = route_prefix << (32 - prefix_length);
    return route_prefix;
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // return (drop dgram) if dgram has no time left to live
    if (dgram.header().ttl == 0 || dgram.header().ttl - 1 == 0) return;
    uint32_t destination = dgram.header().dst;
    Route best_route(0, 0, {}, 0);
    bool route_found = false;

    // go through routing_table to find the best route for the dgram
    for (Route r : routing_table) {
        if (get_prefix(destination, r._prefix_length) == r._prefix) {
            route_found = true;
            if (r._prefix_length >= best_route._prefix_length) {
                best_route = r;
            }
        }
    }
    
    // if no route found, drop dgram
    if (route_found == false) return;

    // decrement dgram's TTL
    dgram.header().ttl -= 1;

    // sends dgram to associated interface
    if (best_route._next_hop.has_value()) {
        //Address addr = best_route._next_hop;
        interface(best_route._interface_num).send_datagram(dgram, best_route._next_hop.value());
    } else {
        interface(best_route._interface_num).send_datagram(dgram, Address::from_ipv4_numeric(destination));
    }
    
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
