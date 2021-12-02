Lab 6 Writeup
=============

My name: Maggie Gray

My SUNet ID: mgray21

I collaborated with: N/A

I would like to thank/reward these classmates for their help: N/A

This lab took me about 1 hours to do. I did attend the lab session.

Program Structure and Design of the Router:
class Route:
    I created a "Route" data structure to hold all relevant information needed to
    define a route in a Router. The "Route" structure contains a route's prefix, 
    prefix length, next hop IP address, and interface number.

routing_table:
    In order to store all known Routes, I used a vector of "Route" data structures.

get_prefix:
    get_prefix is a helper function that takes in a route_prefix and a prefix_length
    and returns a uint32_t with all non-significant numbers set to 0.

add_route:
    add_route creates a "Route" r and adds it to our routing_table vector.

route_one_datagram:
    route_one_datagram routes one datagram if the correct route is stored in our
    routing_table and if the datagram still has TTL > 0.

Implementation Challenges:
Nothing

Remaining Bugs:
None that I know of

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]
