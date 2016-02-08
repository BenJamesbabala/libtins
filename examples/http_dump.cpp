/*
 * Copyright (c) 2016, Matias Fontanini
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above
 *   copyright notice, this list of conditions and the following disclaimer
 *   in the documentation and/or other materials provided with the
 *   distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <iostream>
#include <sstream>
#include "tins/tcp_ip.h"
#include "tins/sniffer.h"
#include "tins/ip_address.h"
#include "tins/ipv6_address.h"

using std::cout;
using std::cerr;
using std::endl;
using std::bind;
using std::string;
using std::ostringstream;
using std::exception;

using Tins::Sniffer;
using Tins::SnifferConfiguration;
using Tins::TCPIP::StreamFollower;
using Tins::TCPIP::Stream;

// Convert the client endpoint to a readable string
string client_endpoint(const Stream& stream) {
    ostringstream output;
    // Use the IPv4 or IPv6 address depending on which protocol the 
    // connection uses
    if (stream.is_v6()) {
        output << stream.client_addr_v6();
    }
    else {
        output << stream.client_addr_v4();
    }
    output << ":" << stream.client_port();
    return output.str();
}

// Convert the server endpoint to a readable string
string server_endpoint(const Stream& stream) {
    ostringstream output;
    if (stream.is_v6()) {
        output << stream.server_addr_v6();
    }
    else {
        output << stream.server_addr_v4();
    }
    output << ":" << stream.server_port();
    return output.str();
}

// Concat both endpoints to get a readable stream identifier
string stream_identifier(const Stream& stream) {
    ostringstream output;
    output << client_endpoint(stream) << " - " << server_endpoint(stream);
    return output.str();
}

// Whenever there's new client data on the stream, this callback is executed.
void on_client_data(Stream& stream) {
    // Construct a string out of the contents of the client's payload
    string data(stream.client_payload().begin(), stream.client_payload().end());
    // Now print it, prepending some information about the stream
    cout << client_endpoint(stream) << " >> " 
         << server_endpoint(stream) << ": " << endl << data << endl;
    // Now erase the stored data, as we've already processed it. This is important,
    // since if we don't do this, the connection will keep buffering data until
    // the stream is closed
    stream.client_payload().clear();
}

// Whenever there's new server data on the stream, this callback is executed.
// This does the same thing as on_client_data
void on_server_data(Stream& stream) {
    string data(stream.server_payload().begin(), stream.server_payload().end());
    cout << server_endpoint(stream) << " >> " 
         << client_endpoint(stream) << ": " << endl << data << endl;
    stream.server_payload().clear();
}

// When a connection is closed, this callback is executed.
void on_connection_closed(Stream& stream) {
    cout << "[+] Connection closed: " << stream_identifier(stream) << endl;
}

// When a new connection is captured, this callback will be executed.
void on_new_connection(Stream& stream) {
    // Print some information about the new connection
    cout << "[+] New connection " << stream_identifier(stream) << endl;
    // Now configure the callbacks on it.
    // First, we want on_client_data to be called every time there's new client data
    stream.client_data_callback(&on_client_data);
    // Same thing for server data, but calling on_server_data
    stream.server_data_callback(&on_server_data);
    // When the connection is closed, call on_connection_closed
    stream.stream_closed_callback(&on_connection_closed);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <interface>" << endl;
        return 1;
    }
    using std::placeholders::_1;

    try {
        // Construct the sniffer configuration object
        SnifferConfiguration config;
        // Only capture TCP traffic sent from/to port 80
        config.set_filter("tcp port 80");
        // Construct the sniffer we'll use
        Sniffer sniffer(argv[1], config);

        cout << "Starting capture on interface " << argv[1] << endl;

        // Now construct the stream follower
        StreamFollower follower;
        // We just need to specify the callback to be executed when a new 
        // stream is captured. In this stream, you should define which callbacks
        // will be executed whenever new data is sent on that stream 
        // (see on_new_connection)
        follower.new_stream_callback(&on_new_connection);
        // Now start capturing. Every time there's a new packet, call 
        // follower.process_packet
        sniffer.sniff_loop(bind(&StreamFollower::process_packet, &follower, _1));
    }
    catch (exception& ex) {
        cerr << "Error: " << ex.what() << endl;
        return 1;
    }
}
