#include <logging/log.h>
#include <zephyr.h>
#include <zeta.h>

LOG_MODULE_DECLARE(zeta, CONFIG_ZETA_LOG_LEVEL);

K_MSGQ_DEFINE(CORE_msgq, sizeof(zt_channel_e), 20, 4);

u8_t running_flag = 0;

void bug0(u8_t pos_x_initial, u8_t pos_y_initial, u8_t pos_x_final, u8_t pos_y_final,
          u8_t obstacle)
{
    static enum { GOING_TO_INITIAL_SHELF, GOING_TO_FINAL_SHELF, FINISH } state = FINISH;

    switch (state) {
    case FINISH: {
        state = GOING_TO_INITIAL_SHELF;
        LOG_DBG("Going to the initial shelf...");
    } break;
    case GOING_TO_INITIAL_SHELF: {
        if (obstacle) {
            LOG_DBG("Wall following...");
        } else {
            LOG_DBG("Going to the point...");
        }
        zt_data_t *wheel_data      = ZT_DATA_BYTES(2, 0);
        wheel_data->bytes.value[0] = sys_rand32_get() % 3;
        wheel_data->bytes.value[1] = sys_rand32_get() % 3;
        zt_chan_pub(ZT_WHEEL_DATA_CHANNEL, wheel_data);
        if (!obstacle) {
            if ((sys_rand32_get() % 100) < 10) {
                LOG_DBG("Reached to the initial shelf");
                LOG_DBG("Going to the final shelf");
                state = GOING_TO_FINAL_SHELF;
            }
        }
    } break;
    case GOING_TO_FINAL_SHELF: {
        if (obstacle) {
            LOG_DBG("Wall following...");
        } else {
            LOG_DBG("Going to the point...");
        }
        zt_data_t *wheel_data      = ZT_DATA_BYTES(2, 0);
        wheel_data->bytes.value[0] = sys_rand32_get() % 3;
        wheel_data->bytes.value[1] = sys_rand32_get() % 3;
        zt_chan_pub(ZT_WHEEL_DATA_CHANNEL, wheel_data);
        if (!obstacle) {
            if ((sys_rand32_get() % 100) < 10) {
                LOG_DBG("Reached to the final shelf...");
                LOG_DBG("Robot job has been finished...");
                running_flag            = 0;
                zt_data_t *running_data = ZT_DATA_U8(running_flag);
                zt_chan_pub(ZT_RUNNING_CHANNEL, running_data);
                state = FINISH;
            }
        }
        break;
    }
    }
}

/**
 * @brief This is the function used by Zeta to tell the CORE that one(s) of the
 * channels which it is subscribed has changed. This callback will be called passing
 * the channel's id in it.
 *
 * @param id
 */
void CORE_service_callback(zt_channel_e id)
{
    k_msgq_put(&CORE_msgq, &id, K_NO_WAIT);
}

/**
 * @brief This is the task loop responsible to run the CORE thread
 * functionality.
 */
void CORE_task()
{
    LOG_DBG("CORE Service has started...[OK]");
    zt_data_t *request_data    = ZT_DATA_BYTES(4, 0);
    zt_data_t *obstacle_sensor = ZT_DATA_U8(0);
    zt_data_t *battery         = ZT_DATA_U8(0);
    u8_t obstacle              = 0;
    zt_channel_e id;
    while (1) {
        while (!k_msgq_get(&CORE_msgq, &id, K_MSEC(50))) {
            switch (id) {
            case ZT_SHELF_REQUEST_CHANNEL: {
                LOG_DBG("Robot job will start...");
                running_flag            = 1;
                zt_data_t *running_data = ZT_DATA_U8(running_flag);
                zt_chan_pub(ZT_RUNNING_CHANNEL, running_data);
            } break;
            case ZT_BATTERY_DATA_CHANNEL: {
                zt_chan_read(ZT_BATTERY_DATA_CHANNEL, battery);
                if (battery->u8.value < 30) {
                    LOG_DBG("Critical battery level %d", battery->u8.value);
                    zt_data_t *charge_request = ZT_DATA_U8(1);
                    zt_chan_pub(ZT_CHARGE_REQUEST_CHANNEL, charge_request);
                }
            } break;
            case ZT_OBSTACLE_DATA_CHANNEL: {
                zt_chan_read(ZT_OBSTACLE_DATA_CHANNEL, obstacle_sensor);
                if (obstacle_sensor->u8.value < 10) {
                    LOG_DBG("Obstacle detected");
                    obstacle = 1;
                }
            } break;
            default:
                break;
            }
        }

        if (running_flag) {
            bug0(request_data->bytes.value[0], request_data->bytes.value[1],
                 request_data->bytes.value[2], request_data->bytes.value[3], obstacle);
            obstacle = 0;
        }
    }
}


ZT_SERVICE_INIT(CORE, CORE_task, CORE_service_callback);
