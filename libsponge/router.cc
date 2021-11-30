#include "router.hh"

#include <iostream>

using namespace std;

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

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
    routing_table.push_back(r);
}

/*
 * Function Name: get_prefix
 * Args: uint32_t route_prefix, uint8_t prefix_length (number of significant bits)
 * Description: This function takes in a route_prefix and a prefix_length and returns a prefix
 * with all non-significant bits set to 0.
 */
uint32_t Router::get_prefix(uint32_t route_prefix, uint8_t prefix_length) {
    // if all 32 bits are significant, return route_prefix
    if (prefix_length == 32)
        return route_prefix;

    // if no bits are significant, return 0
    if (prefix_length == 0)
        return 0;

    // set all non-significant bits to 0
    route_prefix = route_prefix >> (32 - prefix_length);
    route_prefix = route_prefix << (32 - prefix_length);
    return route_prefix;
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // return (drop dgram) if dgram has no time left to live
    if (dgram.header().ttl == 0 || dgram.header().ttl - 1 == 0)
        return;

    Route best_route(0, 0, {}, 0);
    bool route_found = false;

    // go through routing_table to find the best route for the dgram
    for (Route r : routing_table) {
        // if dgram's dst matches the prefix of a route in our table, set route_found to true
        if (get_prefix(dgram.header().dst, r._prefix_length) == r._prefix) {
            route_found = true;
            // if the route found has a larger prefix length then the best route previously found
            // set best_route to the new route found
            if (r._prefix_length >= best_route._prefix_length) {
                best_route = r;
            }
        }
    }

    // if no route found, drop dgram
    if (route_found == false)
        return;

    // decrement dgram's TTL
    dgram.header().ttl -= 1;

    // send dgram to interface specified by route found
    if (best_route._next_hop.has_value()) {
        interface(best_route._interface_num).send_datagram(dgram, best_route._next_hop.value());
    } else {
        interface(best_route._interface_num).send_datagram(dgram, Address::from_ipv4_numeric(dgram.header().dst));
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
