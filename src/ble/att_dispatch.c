/*
 * Copyright (C) 2014 BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. Any redistribution, use, or modification is done solely for
 *    personal benefit and not for any commercial purpose or for
 *    monetary gain.
 *
 * THIS SOFTWARE IS PROVIDED BY BLUEKITCHEN GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Please inquire about commercial licensing options at 
 * contact@bluekitchen-gmbh.com
 *
 */


/**
 * Dispatcher for independent implementation of ATT client and server
 */

#include "l2cap.h"

#include "att_dispatch.h"
#include "btstack_debug.h"

static void dummy_packet_handler(uint8_t packet_type, uint16_t handle, uint8_t *packet, uint16_t size);

static btstack_packet_handler_t att_client_handler = &dummy_packet_handler;
static btstack_packet_handler_t att_server_handler = &dummy_packet_handler;

static btstack_packet_callback_registration_t hci_event_callback_registration;

static int registered_for_hci_events = 0;

static void dummy_packet_handler(uint8_t packet_type, uint16_t handle, uint8_t *packet, uint16_t size){
}

static void att_hci_event_handler(uint8_t packet_type, uint8_t * packet, uint16_t size){
	att_server_handler(packet_type, 0, packet, size);
}

static void att_dispatch_register_for_hci_events(void){
	if (registered_for_hci_events) return;
	registered_for_hci_events = 1;
    hci_event_callback_registration.callback = &att_hci_event_handler;
    hci_add_event_handler(&hci_event_callback_registration);
}

static void att_packet_handler(uint8_t packet_type, uint16_t handle, uint8_t *packet, uint16_t size){
	if (packet_type != ATT_DATA_PACKET) return;

	// log_info("att_data_packet with opcode 0x%x", packet[0]);
	uint8_t att_pdu_odd = packet[0] & 1;
	if (att_pdu_odd){
		// odd PDUs are sent from server to client
		att_client_handler(packet_type, handle, packet, size);
	} else {
		// even PDUs are sent from client to server
		att_server_handler(packet_type, handle, packet, size);
	}
}

/**
 * @brief reset att dispatchter
 * @param packet_hander for ATT client packets
 */
void att_dispatch_register_client(btstack_packet_handler_t packet_handler){
	att_dispatch_register_for_hci_events();
	if (packet_handler == NULL){
		packet_handler = dummy_packet_handler;
	}
	att_client_handler = packet_handler;
	l2cap_register_fixed_channel(att_packet_handler, L2CAP_CID_ATTRIBUTE_PROTOCOL);
}

/**
 * @brief reset att dispatchter
 * @param packet_hander for ATT server packets
 */
void att_dispatch_register_server(btstack_packet_handler_t packet_handler){
	att_dispatch_register_for_hci_events();
	if (packet_handler == NULL){
		packet_handler = dummy_packet_handler;
	}
	att_server_handler = packet_handler;
	l2cap_register_fixed_channel(att_packet_handler, L2CAP_CID_ATTRIBUTE_PROTOCOL);
}
