#include <logging/log.h>
#include <zephyr.h>
#include <zeta.h>

LOG_MODULE_DECLARE(zeta, CONFIG_ZETA_LOG_LEVEL);

K_MSGQ_DEFINE(NET_msgq, sizeof(zt_channel_e), 50, 4);

u32_t request_fetch(void)
{
    return sys_rand32_get() % 100;
}

/**
 * @brief This is the function used by Zeta to tell the NET that one(s) of the
 * channels which it is subscribed has changed. This callback will be called passing the
 * channel's id in it.
 *
 * @param id
 */
void NET_service_callback(zt_channel_e id)
{
    k_msgq_put(&NET_msgq, &id, K_NO_WAIT);
}

/**
 * @brief This is the task loop responsible to run the NET thread
 * functionality.
 */
void NET_task()
{
    LOG_DBG("NET Service has started...[OK]");
    zt_data_t *running_flag = ZT_DATA_U8(0);
    zt_data_t *request_data = ZT_DATA_BYTES(4, 0);
    zt_channel_e id;
    while (1) {
        while (!k_msgq_get(&NET_msgq, &id, K_MSEC(50))) {
            switch (id) {
            case ZT_CHARGE_REQUEST_CHANNEL: {
                LOG_DBG("Sending packet \"The robot need to be charged\"");
            } break;
            case ZT_RUNNING_CHANNEL: {
                zt_chan_read(ZT_RUNNING_CHANNEL, running_flag);
                if (!running_flag->u8.value) {
                    LOG_DBG("Sending packet \"The SHELF_REQUEST has been executed...\"");
                }
            } break;
            default:
                break;
            }
        }
        if (!running_flag->u8.value) {
            request_data->u32.value = request_fetch();
            if (request_data) {
                LOG_DBG("SHELF_REQUEST has been received...");
                zt_chan_pub(ZT_SHELF_REQUEST_CHANNEL, request_data);
            }
        }
    }
}

ZT_SERVICE_INIT(NET, NET_task, NET_service_callback);
