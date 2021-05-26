#include <logging/log.h>
#include <zephyr.h>
#include <zeta.h>

LOG_MODULE_DECLARE(zeta, CONFIG_ZETA_LOG_LEVEL);

K_SEM_DEFINE(TWO_WHEEL_callback_sem, 0, 1);

void motors_action(u8_t left_wheel, u8_t right_wheel)
{
    if (left_wheel < right_wheel) {
        LOG_DBG("Turning left...");
    } else if (left_wheel > right_wheel) {
        LOG_DBG("Turning right...");
    } else if (left_wheel != 0) {
        LOG_DBG("Fowarding...");
    } else {
        LOG_DBG("Stoping...");
    }
}


/**
 * @brief This is the function used by Zeta to tell the TWO_WHEEL that one(s) of the
 * channels which it is subscribed has changed. This callback will be called passing the
 * channel's id in it.
 *
 * @param id
 */
void TWO_WHEEL_service_callback(zt_channel_e id)
{
    k_sem_give(&TWO_WHEEL_callback_sem);
}

/**
 * @brief This is the task loop responsible to run the TWO_WHEEL thread
 * functionality.
 */
void TWO_WHEEL_task()
{
    LOG_DBG("TWO_WHEEL Service has started...[OK]");
    zt_data_t *wheel_data = ZT_DATA_U16(0);
    while (1) {
        k_sem_take(&TWO_WHEEL_callback_sem, K_FOREVER);
        zt_chan_read(ZT_WHEEL_DATA_CHANNEL, wheel_data);
        motors_action(wheel_data->bytes.value[0], wheel_data->bytes.value[1]);
    }
}

ZT_SERVICE_INIT(TWO_WHEEL, TWO_WHEEL_task, TWO_WHEEL_service_callback);
