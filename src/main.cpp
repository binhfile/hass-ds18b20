//
// Created by developer on 22/12/2019.
//

#include <ds18b20.h>
#include <fmt/format.h>
#include <getopt.h>
#include <mosquitto.h>
#include <atomic>
#include <chrono>
#include <csignal>
#include <string>
#include <thread>
using namespace std::chrono_literals;

std::atomic_bool s_term{false};
static void int_handler(int sig) {
    (void)sig;
    s_term.store(true, std::memory_order_relaxed);
}

static void mosq_log_callback(struct mosquitto *mosq, void *userdata, int level,
                              const char *str) {
    /* Pring all log messages regardless of level. */

    switch (level) {
        // case MOSQ_LOG_DEBUG:
        // case MOSQ_LOG_INFO:
        // case MOSQ_LOG_NOTICE:
        case MOSQ_LOG_WARNING:
        case MOSQ_LOG_ERR: {
            printf("%i:%s\n", level, str);
        }
    }
}

/**
 * --gpio_num=<gpio pin's number>
 * --mqtt_ip=<mqtt broker ip>
 * --mqtt_port=<mqtt broker port>
 * --mqtt_topic<mqtt topic to publish> ex: /home/outdoor/temperature/state
 *
 * data format:
 * {
 *   "temperature": <C, float format>,
 *   "humidity": <RH, float format>
 * }
 */
int main(int argc, char **argv) {
    std::signal(SIGINT, int_handler);
    // read config from command line
    int gpio_num = 0;
    std::string mqtt_ip, mqtt_topic;
    int mqtt_port = 0;
    struct option long_options[] = {{"gpio_num", required_argument, 0, 'g'},
                                    {"mqtt_ip", required_argument, 0, 'i'},
                                    {"mqtt_port", required_argument, 0, 'p'},
                                    {"mqtt_topic", required_argument, 0, 't'},
                                    {0, 0, 0, 0}};
    while (true) {
        int option_index = 0;
        int c =
            getopt_long(argc, argv, "g:i:p:t:?", long_options, &option_index);
        if (c == -1) break;
        switch (c) {
            case 'g':
                gpio_num = atoi(optarg);
                break;
            case 'i':
                mqtt_ip = std::string(optarg);
                break;
            case 'p':
                mqtt_port = atoi(optarg);
                break;
            case 't':
                mqtt_topic = std::string(optarg);
                break;
            default:
                printf("Invalid arguments");
                return -1;
        }
    }

    printf("GPIO: %d\r\n", gpio_num);
    printf("MQTT: %s:%d\r\n", mqtt_ip.c_str(), mqtt_port);
    printf("Topic: %s\r\n", mqtt_topic.c_str());

    mosquitto_lib_init();
    struct mosquitto *mqtt = mosquitto_new(NULL, true, NULL);
    if (!mqtt) {
        fprintf(stderr, "Error: Out of memory.\r\n");
        mosquitto_lib_cleanup();
        exit(1);
    }
    mosquitto_log_callback_set(mqtt, mosq_log_callback);
    int keepalive = 60;
    if (mosquitto_connect(mqtt, mqtt_ip.c_str(), mqtt_port, keepalive)) {
        fprintf(stderr, "Unable to connect.\r\n");
        mosquitto_destroy(mqtt);
        mosquitto_lib_cleanup();
        exit(1);
    }
    int loop = mosquitto_loop_start(mqtt);
    if (loop != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "Unable to start loop: %i\r\n", loop);
        mosquitto_destroy(mqtt);
        mosquitto_lib_cleanup();
        exit(1);
    }

    ds18b20 drv;
    drv.pin_no = gpio_num;
    int err = ds18b20_initialize(&drv);
    if (err) {
        fprintf(stderr, "Unable to initialize driver.\r\n");
        mosquitto_loop_stop(mqtt, true);
        mosquitto_destroy(mqtt);
        mosquitto_lib_cleanup();
        exit(1);
    }

    int temperature = 0;
    ds18b20_resolution resolution = ds18b20_resolution_9bit;
    while (!s_term.load(std::memory_order_relaxed)) {
        err = ds18b20_read_one_device(&drv, &temperature, &resolution);
        if (err) {
            fprintf(stderr, "Unable to read error = %d\r\n", err);
        } else {
            float temp = (float)temperature / 1000.0f;
            printf("    T=%.3f Bit=%d\r\n", temp, resolution);
            std::string msg = fmt::format("{{\"value\":{}}}", temp);
            err = mosquitto_publish(mqtt, NULL, mqtt_topic.c_str(),
                                    msg.length(), msg.c_str(), 0, 0);
            if (err != MOSQ_ERR_SUCCESS) {
                fprintf(stderr, "Unable to publish = %d\r\n", err);
            }
        }
        std::this_thread::sleep_for(5s);
    }

    ds18b20_destroy(&drv);
    mosquitto_loop_stop(mqtt, true);
    mosquitto_destroy(mqtt);
    mosquitto_lib_cleanup();
    printf("exit\r\n");
    return 0;
}