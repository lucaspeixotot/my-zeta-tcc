#include <logging/log.h>
#include <zephyr.h>
#include <zeta.h>
#include <random/rand32.h>

LOG_MODULE_DECLARE(zeta, CONFIG_ZETA_LOG_LEVEL);

K_SEM_DEFINE(SENSOR_callback_sem, 0, 1);


u8_t obstacle_sensor_fetch(void)
{
    return sys_rand32_get() % 100;
}

u8_t battery_fetch(void)
{
    return sys_rand32_get() % 100;
}


/**
 * @brief This is the function used by Zeta to tell the SENSOR that one(s) of the
 * channels which it is subscribed has changed. This callback will be called passing the
 * channel's id in it.
 *
 * @param id
 */
void SENSOR_service_callback(zt_channel_e id)
{
    k_sem_give(&SENSOR_callback_sem);
}

/**
 * @brief This is the task loop responsible to run the SENSOR thread
 * functionality.
 */
void SENSOR_task()
{
    LOG_DBG("SENSOR Service has started...[OK]");
    zt_data_t* running_flag    = ZT_DATA_U8(0);
    zt_data_t* obstacle_sensor = ZT_DATA_U8(0);
    zt_data_t* battery         = ZT_DATA_U8(0);
    while (1) {
        if (!k_sem_take(&SENSOR_callback_sem, K_MSEC(50))) {
            zt_chan_read(ZT_RUNNING_CHANNEL, running_flag);
        }
        if (running_flag->u8.value) {
            obstacle_sensor->u8.value = obstacle_sensor_fetch();
            LOG_DBG("Sending obstacle sensor data...");
            zt_chan_pub(ZT_OBSTACLE_DATA_CHANNEL, obstacle_sensor);
        } else {
            battery->u8.value = battery_fetch();
            LOG_DBG("Sending battery data...");
            zt_chan_pub(ZT_BATTERY_DATA_CHANNEL, battery);
        }
    }
}

ZT_SERVICE_INIT(SENSOR, SENSOR_task, SENSOR_service_callback);
