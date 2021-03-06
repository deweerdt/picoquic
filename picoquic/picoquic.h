/*
* Author: Christian Huitema
* Copyright (c) 2017, Private Octopus, Inc.
* All rights reserved.
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Private Octopus, Inc. BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef PICOQUIC_H
#define PICOQUIC_H

#include <stdint.h>
#ifdef WIN32
#include <winsock2.h>
#include <Ws2def.h>
#include <WS2tcpip.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifdef  __cplusplus
extern "C" {
#endif

#define PICOQUIC_ERROR_CLASS 0x400
#define PICOQUIC_ERROR_DUPLICATE (PICOQUIC_ERROR_CLASS  + 1)
#define PICOQUIC_ERROR_FNV1A_CHECK (PICOQUIC_ERROR_CLASS  + 2)
#define PICOQUIC_ERROR_AEAD_CHECK (PICOQUIC_ERROR_CLASS  + 3)
#define PICOQUIC_ERROR_UNEXPECTED_PACKET (PICOQUIC_ERROR_CLASS  + 4)
#define PICOQUIC_ERROR_MEMORY (PICOQUIC_ERROR_CLASS  + 5)
#define PICOQUIC_ERROR_SPURIOUS_REPEAT (PICOQUIC_ERROR_CLASS  + 6)
#define PICOQUIC_ERROR_CNXID_CHECK (PICOQUIC_ERROR_CLASS  + 7)
#define PICOQUIC_ERROR_INITIAL_TOO_SHORT (PICOQUIC_ERROR_CLASS  + 8)
#define PICOQUIC_ERROR_VERSION_NEGOTIATION_SPOOFED (PICOQUIC_ERROR_CLASS  + 9)
#define PICOQUIC_ERROR_MALFORMED_TRANSPORT_EXTENSION (PICOQUIC_ERROR_CLASS  + 10)
#define PICOQUIC_ERROR_EXTENSION_BUFFER_TOO_SMALL (PICOQUIC_ERROR_CLASS  + 11)
#define PICOQUIC_ERROR_ILLEGAL_TRANSPORT_EXTENSION (PICOQUIC_ERROR_CLASS  + 12)
#define PICOQUIC_ERROR_CANNOT_RESET_STREAM_ZERO (PICOQUIC_ERROR_CLASS  + 13)
#define PICOQUIC_ERROR_INVALID_STREAM_ID (PICOQUIC_ERROR_CLASS  + 14)
#define PICOQUIC_ERROR_STREAM_ALREADY_CLOSED (PICOQUIC_ERROR_CLASS  + 15)
#define PICOQUIC_ERROR_FRAME_BUFFER_TOO_SMALL (PICOQUIC_ERROR_CLASS  + 16)
#define PICOQUIC_ERROR_INVALID_FRAME (PICOQUIC_ERROR_CLASS  + 17)
#define PICOQUIC_ERROR_CANNOT_CONTROL_STREAM_ZERO (PICOQUIC_ERROR_CLASS  + 18)
#define PICOQUIC_ERROR_HRR (PICOQUIC_ERROR_CLASS  + 19)
#define PICOQUIC_ERROR_DISCONNECTED (PICOQUIC_ERROR_CLASS  + 20)

#define PICOQUIC_TRANSPORT_ERROR_NO_ERROR (0x80000000)
#define PICOQUIC_TRANSPORT_ERROR_INTERNAL (0x80000001)
#define PICOQUIC_TRANSPORT_ERROR_CANCELLED (0x80000002) /* RST STREAM only */
#define PICOQUIC_TRANSPORT_ERROR_FLOW_CONTROL_ERROR (0x80000003)
#define PICOQUIC_TRANSPORT_ERROR_STREAM_ID_ERROR (0x80000004)
#define PICOQUIC_TRANSPORT_ERROR_STREAM_STATE_ERROR (0x80000005)
#define PICOQUIC_TRANSPORT_ERROR_FINAL_OFFSET_ERROR (0x80000006)
#define PICOQUIC_TRANSPORT_ERROR_FRAME_FORMAT_ERROR (0x80000007)
#define PICOQUIC_TRANSPORT_ERROR_TRANSPORT_PARAMETER_ERROR (0x80000008)
#define PICOQUIC_TRANSPORT_ERROR_VERSION_NEGOTIATION_ERROR (0x80000009)
#define PICOQUIC_TRANSPORT_ERROR_PROTOCOL_VIOLATION (0x8000000A) :
#define PICOQUIC_TRANSPORT_ERROR_QUIC_RECEIVED_RST (0x80000035) 
#define PICOQUIC_TRANSPORT_ERROR_FRAME_ERROR_MIN (0x80000100)
#define PICOQUIC_TRANSPORT_ERROR_FRAME_ERROR_MAX (0x800001FF)
#define PICOQUIC_TRANSPORT_ERROR_FRAME_ERROR(frame_id) (0x80000100|(frame_id)) /* XX is replaced by actual frame type */

#define PICOQUIC_MAX_PACKET_SIZE 1536

	/*
	 * Connection states, useful to expose the state to the application.
	 */
    typedef enum
    {
        picoquic_state_client_init,
        picoquic_state_client_init_sent,
        picoquic_state_client_renegotiate,
        picoquic_state_client_hrr_received,
        picoquic_state_client_init_resent,
        picoquic_state_server_init,
        picoquic_state_client_handshake_start,
        picoquic_state_client_handshake_progress,
        picoquic_state_client_almost_ready,
        picoquic_state_client_ready,
        picoquic_state_server_almost_ready,
        picoquic_state_server_ready,
        picoquic_state_disconnecting,
        picoquic_state_disconnected,
        picoquic_state_server_send_hrr
	} picoquic_state_enum;

	/*
	 * The stateless packet structure is used to temporarily store
	 * stateless packets before they can be sent by servers.
	 */

	typedef struct st_picoquic_stateless_packet_t {
		struct st_picoquic_stateless_packet_t * next_packet;
		struct sockaddr_storage addr_to;
		size_t length;

		uint8_t bytes[PICOQUIC_MAX_PACKET_SIZE];
	} picoquic_stateless_packet_t;

	/*
	 * The simple packet structure is used to store packets that
	 * have been sent but are not yet acknowledged.
	 * Packets are stored in unencrypted format.
	 * The checksum length is the difference between encrypted and unencrypted.
	 */
	typedef struct _picoquic_packet {
		struct _picoquic_packet * previous_packet;
		struct _picoquic_packet * next_packet;

		uint64_t sequence_number;
		uint64_t send_time;
		size_t length;
		size_t checksum_overhead;

		uint8_t bytes[PICOQUIC_MAX_PACKET_SIZE];
	} picoquic_packet;

	typedef struct st_picoquic_quic_t picoquic_quic_t;
	typedef struct st_picoquic_cnx_t picoquic_cnx_t;

	typedef enum {
		picoquic_callback_no_event = 0,
		picoquic_callback_stream_fin,
		picoquic_callback_stream_reset,
        picoquic_callback_close
	} picoquic_call_back_event_t;

	/* Callback function for providing stream data to the application */
	typedef void(*picoquic_stream_data_cb_fn) (picoquic_cnx_t * cnx,
		uint32_t stream_id, uint8_t * bytes, size_t length, 
		picoquic_call_back_event_t fin_or_event, void * callback_ctx);


	/* QUIC context create and dispose */
	picoquic_quic_t * picoquic_create(uint32_t nb_connections,
		char const * cert_file_name, char const * key_file_name,
		char const * default_alpn,
		picoquic_stream_data_cb_fn default_callback_fn,
		void * default_callback_ctx);

	void picoquic_free(picoquic_quic_t * quic);

    /* Set cookie mode on QUIC context when under stress */
    void picoquic_set_cookie_mode(picoquic_quic_t * quic, int cookie_mode);

	/* Connection context creation and registration */
	picoquic_cnx_t * picoquic_create_cnx(picoquic_quic_t * quic,
		uint64_t cnx_id, struct sockaddr * addr, uint64_t start_time, uint32_t preferred_version,
		char const * sni, char const * alpn);

	picoquic_cnx_t * picoquic_create_client_cnx(picoquic_quic_t * quic, 
		struct sockaddr * addr, uint64_t start_time, uint32_t preferred_version,
		char const * sni, char const * alpn, 
		picoquic_stream_data_cb_fn callback_fn, void * callback_ctx);

	void picoquic_delete_cnx(picoquic_cnx_t * cnx);

	int picoquic_close(picoquic_cnx_t * cnx);

	picoquic_cnx_t * picoquic_get_first_cnx(picoquic_quic_t * quic);
    picoquic_cnx_t * picoquic_get_next_cnx(picoquic_cnx_t * cnx);
    int64_t picoquic_get_next_wake_delay(picoquic_quic_t * quic, 
        uint64_t current_time,
        int64_t delay_max);

	picoquic_state_enum picoquic_get_cnx_state(picoquic_cnx_t * cnx);

    void picoquic_get_peer_addr(picoquic_cnx_t * cnx, struct sockaddr ** addr, int * addr_len);

    uint64_t picoquic_get_cnxid(picoquic_cnx_t * cnx);
    uint64_t picoquic_get_initial_cnxid(picoquic_cnx_t * cnx);
    uint64_t picoquic_get_cnx_start_time(picoquic_cnx_t * cnx);

    int picoquic_is_cnx_backlog_empty(picoquic_cnx_t * cnx);

    void picoquic_set_callback(picoquic_cnx_t * cnx,
        picoquic_stream_data_cb_fn callback_fn, void * callback_ctx);

	/* Send and receive network packets */

	picoquic_stateless_packet_t * picoquic_dequeue_stateless_packet(picoquic_quic_t * quic);
	void picoquic_delete_stateless_packet(picoquic_stateless_packet_t * sp);

	int picoquic_incoming_packet(
		picoquic_quic_t * quic,
		uint8_t * bytes,
		uint32_t length,
		struct sockaddr * addr_from,
		uint64_t current_time);

	picoquic_packet * picoquic_create_packet();

	int picoquic_prepare_packet(picoquic_cnx_t * cnx, picoquic_packet * packet,
		uint64_t current_time, uint8_t * send_buffer, size_t send_buffer_max, size_t * send_length);

	/* send and receive data on streams */
	int picoquic_add_to_stream(picoquic_cnx_t * cnx,
		uint32_t stream_id, const uint8_t * data, size_t length, int set_fin);

	int picoquic_reset_stream(picoquic_cnx_t * cnx,
		uint32_t stream_id);


	/* Congestion algorithm definition */
	typedef enum {
		picoquic_congestion_notification_acknowledgement,
		picoquic_congestion_notification_repeat,
		picoquic_congestion_notification_timeout,
		picoquic_congestion_notification_spurious_repeat,
		picoquic_congestion_notification_rtt_measurement
	} picoquic_congestion_notification_t;

	typedef void(*picoquic_congestion_algorithm_init) (picoquic_cnx_t * cnx);
	typedef void(*picoquic_congestion_algorithm_notify)(picoquic_cnx_t * cnx,
		picoquic_congestion_notification_t notification,
		uint64_t rtt_measurement,
		uint64_t nb_bytes_acknowledged,
		uint64_t lost_packet_number,
		uint64_t current_time);
	typedef void(*picoquic_congestion_algorithm_delete) (picoquic_cnx_t * cnx);

	typedef struct st_picoquic_congestion_algorithm_t {
		uint32_t congestion_algorithm_id;
		picoquic_congestion_algorithm_init alg_init;
		picoquic_congestion_algorithm_notify alg_notify;
		picoquic_congestion_algorithm_delete alg_delete;
	} picoquic_congestion_algorithm_t;

	void picoquic_set_default_congestion_algorithm(picoquic_quic_t * quic, picoquic_congestion_algorithm_t const * algo);

	void picoquic_set_congestion_algorithm(picoquic_cnx_t * cnx, picoquic_congestion_algorithm_t const * algo);

    /* For building a basic HTTP 0.9 test server */
    int http0dot9_get(uint8_t * command, size_t command_length,
        uint8_t * response, size_t response_max, size_t *response_length);

#ifdef  __cplusplus
}
#endif

#endif /* PICOQUIC_H */
