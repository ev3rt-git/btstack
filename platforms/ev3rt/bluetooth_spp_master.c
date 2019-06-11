#include "sdp_query_rfcomm.h"
#include "rfcomm.h"
#include "sdp.h"

#define SPP_MASTER_TIMER_PERIOD_MS (100)

#define SPP_MASTER_TEST_CONNECTION ((void*)0xdeadbeef)
static uint16_t spp_master_test_channel_id = 0;
static uint16_t spp_master_test_channel_mtu = 0;

typedef enum {
//    SPP_MASTER_TEST,
    SPP_MASTER_IDLE,
    SPP_MASTER_OPEN,
    SPP_MASTER_SDP,
    SPP_MASTER_DONE,
} spp_master_state_t;

static btstack_bool_t     spp_master_in_reset = 0;
static spp_master_state_t spp_master_state = SPP_MASTER_IDLE;
static bd_addr_t          spp_master_remote;
static char               spp_master_pincode[BTSTACK_PIN_MAX_LEN + 1];
static char               spp_service_name_prefix[BTSTACK_SERVICE_MAX_LEN + 1];

typedef enum {
    W4_SDP_RESULT,
    W4_SDP_COMPLETE,
    W4_RFCOMM_CHANNEL,
    DONE
} state_t;
static state_t state = W4_SDP_RESULT;
static uint8_t  channel_nr = 0;
//static bd_addr_t remote = {0xD0, 0x07, 0x90, 0x49, 0xCB, 0x93};

static void handle_found_service(char * name, uint8_t port){
    log_error("APP: Service name: '%s', RFCOMM port %u\n", name, port);

    if (strncmp(name, spp_service_name_prefix, strlen(spp_service_name_prefix)) != 0) return;

    log_error("APP: matches requested SPP Service Name\n");
    channel_nr = port;
    state = W4_SDP_COMPLETE;
}

static void handle_query_rfcomm_event(sdp_query_event_t * event, void * context){
    sdp_query_rfcomm_service_event_t * ve;

    switch (event->type){
        case SDP_QUERY_RFCOMM_SERVICE:
            ve = (sdp_query_rfcomm_service_event_t*) event;
            handle_found_service((char*) ve->service_name, ve->channel_nr);
            break;
        case SDP_QUERY_COMPLETE:
            if (state != W4_SDP_COMPLETE){
                log_error("Requested SPP Service %s not found \n", spp_service_name_prefix);
                spp_master_state = SPP_MASTER_OPEN;
                break;
            }
            // connect
            log_error("Requested SPP Service found, creating RFCOMM channel\n");
            state = W4_RFCOMM_CHANNEL;
            rfcomm_create_channel_internal(SPP_MASTER_TEST_CONNECTION, spp_master_remote, channel_nr);
            break;
        default:
            break;
    }
}

static timer_source_t spp_master_timer;

#if BD_ADDR_LEN != 6 || BD_ADDR_LEN != BTSTACK_BD_ADDR_LEN
#error BD_ADDR_LEN must be 6.
#endif

void btstack_spp_master_reset() {
    btstack_spp_master_lock();
    spp_master_in_reset = 1;

    while (spp_master_state != SPP_MASTER_IDLE) {
        btstack_spp_master_unlock();
        btstack_runloop_sleep(100U);
        btstack_spp_master_lock();
    }

    spp_master_in_reset = 0;
    btstack_spp_master_unlock();
}

btstack_bool_t btstack_spp_master_connect(const uint8_t *addr, const char *pin, const char *service) {
    btstack_bool_t ret = 0;
    btstack_spp_master_lock();

    if (spp_master_state == SPP_MASTER_IDLE) {
        memcpy(spp_master_remote, addr, BD_ADDR_LEN);
        strncpy(spp_master_pincode, pin, sizeof(spp_master_pincode));
        spp_master_pincode[sizeof(spp_master_pincode) - 1] = '\0';
        strncpy(spp_service_name_prefix, service, sizeof(spp_service_name_prefix));
        spp_service_name_prefix[sizeof(spp_service_name_prefix) - 1] = '\0';
        spp_master_test_channel_id = 0;
        // FIXME: store pin code
        spp_master_state = SPP_MASTER_OPEN;
        ret = 1;
    }

    btstack_spp_master_unlock();
    return ret;
}

static void spp_master_timer_handler(timer_source_t *ts) {
    btstack_spp_master_lock();
    if (spp_master_in_reset && spp_master_state == SPP_MASTER_OPEN) {
        spp_master_state = SPP_MASTER_IDLE;
        btstack_spp_master_unlock();
    } else if (spp_master_in_reset && spp_master_state == SPP_MASTER_DONE) {
        if (spp_master_test_channel_id == 0) log_error("SPP_MASTER_DONE is already closed\n");
        btstack_spp_master_unlock();
        rfcomm_disconnect_internal(spp_master_test_channel_id);
    } else if (spp_master_state == SPP_MASTER_OPEN) {
        log_error("SPP_MASTER_SDP start");
        spp_master_state = SPP_MASTER_SDP;
        btstack_spp_master_unlock();
        sdp_query_rfcomm_channel_and_name_for_uuid(spp_master_remote, SDP_PublicBrowseGroup);
    } else {
        btstack_spp_master_unlock();
    }

	run_loop_set_timer(ts, SPP_MASTER_TIMER_PERIOD_MS);
	run_loop_add_timer(&spp_master_timer);
}

static void spp_master_setup() {
    sdp_query_rfcomm_register_callback(handle_query_rfcomm_event, NULL);
    spp_master_timer.process = spp_master_timer_handler;
    run_loop_set_timer(&spp_master_timer, SPP_MASTER_TIMER_PERIOD_MS);
    run_loop_add_timer(&spp_master_timer);
}
