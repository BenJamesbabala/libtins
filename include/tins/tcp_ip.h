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

#ifndef TINS_TCP_IP_H
#define TINS_TCP_IP_H

#include "cxxstd.h"

// This classes use C++11 features
#if TINS_IS_CXX11

#include <vector>
#include <array>
#include <map>
#include <functional>
#include <stdint.h>
#include "macros.h"

namespace Tins {

class PDU;
class TCP;
class IPv4Address;
class IPv6Address;

namespace TCPIP {

class TINS_API Flow {
public:
    enum State {
        UNKNOWN,
        SYN_SENT,
        ESTABLISHED,
        FIN_SENT,
        RST_SENT
    };

    typedef std::vector<uint8_t> payload_type;
    typedef std::map<uint32_t, payload_type> buffered_payload_type;
    typedef std::function<void(Flow&)> event_callback;

    Flow(const IPv4Address& dest_address, uint16_t dest_port,
         uint32_t sequence_number);

    Flow(const IPv6Address& dest_address, uint16_t dest_port,
         uint32_t sequence_number);

    void data_callback(const event_callback& callback);
    void buffering_callback(const event_callback& callback);

    void process_packet(PDU& pdu);

    bool is_v6() const;
    bool is_finished() const;
    bool packet_belongs(const PDU& packet) const;

    IPv4Address dst_addr_v4() const;
    IPv6Address dst_addr_v6() const;
    uint16_t dport() const;
    const payload_type& payload() const;
    payload_type& payload();
    State state() const;
    uint32_t sequence_number() const;
    const buffered_payload_type& buffered_payload() const;
    buffered_payload_type& buffered_payload();

    void state(State new_state);
private:
    void store_payload(uint32_t seq, const payload_type& payload);
    buffered_payload_type::iterator erase_iterator(buffered_payload_type::iterator iter);
    void update_state(const TCP& tcp);

    payload_type payload_;
    buffered_payload_type buffered_payload_;
    uint32_t seq_number_;
    std::array<uint8_t, 16> dest_address_;
    uint16_t dest_port_;
    event_callback on_data_callback_;
    event_callback on_buffering_callback_;
    bool is_v6_;
    State state_;
};

class TINS_API Stream {
public:
    enum State {
        SYN_SENT,
        SYN_RCVD,
        ESTABLISHED,
        CLOSE_WAIT,
        FIN_WAIT_1,
        FIN_WAIT_2,
        TIME_WAIT,
        CLOSED
    };

    typedef std::function<void(Stream&)> stream_callback;
    typedef Flow::payload_type payload_type;

    Stream(const PDU& initial_packet);
    Stream(const Flow& client_flow, const Flow& server_flow);

    void process_packet(PDU& packet);

    Flow& client_flow();
    const Flow& client_flow() const;
    Flow& server_flow();
    const Flow& server_flow() const;

    bool is_finished() const;
    bool is_v6() const;

    IPv4Address client_addr_v4() const;
    IPv6Address client_addr_v6() const;
    IPv4Address server_addr_v4() const;
    IPv6Address server_addr_v6() const;
    uint16_t client_port() const;
    uint16_t server_port() const;
    const payload_type& client_payload() const;
    payload_type& client_payload();
    const payload_type& server_payload() const;
    payload_type& server_payload();

    void stream_closed_callback(const stream_callback& callback);
    void client_data_callback(const stream_callback& callback);
    void server_data_callback(const stream_callback& callback);
    void client_buffering_callback(const stream_callback& callback);
    void server_buffering_callback(const stream_callback& callback);

    void setup_flows_callbacks();
private:
    static Flow extract_client_flow(const PDU& packet);
    static Flow extract_server_flow(const PDU& packet);

    void on_client_flow_data(const Flow& flow);
    void on_server_flow_data(const Flow& flow);
    void on_client_buffering(const Flow& flow);
    void on_server_buffering(const Flow& flow);

    Flow client_flow_;
    Flow server_flow_;
    stream_callback on_stream_closed_;
    stream_callback on_client_data_callback_;
    stream_callback on_server_data_callback_;
    stream_callback on_client_buffering_callback_;
    stream_callback on_server_buffering_callback_;
    State state_;
};

class TINS_API StreamFollower {
public:
    typedef Stream::stream_callback stream_callback;

    StreamFollower();

    bool process_packet(PDU& packet);
    void new_stream_callback(const stream_callback& callback);

    Stream& find_stream(IPv4Address client_addr, uint16_t client_port,
                           IPv4Address server_addr, uint16_t server_port);
private:
    static const size_t DEFAULT_MAX_BUFFERED_CHUNKS;
    typedef std::array<uint8_t, 16> address_type;

    struct stream_id {
        stream_id(const address_type& client_addr, uint16_t client_port,
                  const address_type& server_addr, uint16_t server_port);

        address_type min_address;
        address_type max_address;
        uint16_t min_address_port;
        uint16_t max_address_port;

        bool operator<(const stream_id& rhs) const;

        static size_t hash(const stream_id& id);
    };

    typedef std::map<stream_id, Stream> streams_type;

    stream_id make_stream_id(const PDU& packet);
    static address_type serialize(IPv4Address address);
    static address_type serialize(const IPv6Address& address);

    streams_type streams_;
    stream_callback on_new_connection_;
    size_t max_buffered_chunks_;
    bool attach_to_flows_;
};

} // TCPIP
} // Tins

#endif // TINS_IS_CXX11 

#endif // TINS_TCP_IP_H
