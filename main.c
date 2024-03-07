#include "driver/gpio.h"
#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "string.h"
#include "esp_log.h"
#include "dht.h"
#define CONFIG_DHT11_PIN GPIO_NUM_26
#define CONFIG_CONNECTION_TIMEOUT 5


static const char *TAG = "mqtt_client_example";

esp_mqtt_client_handle_t mqtt_client;

bool mqtt_connected = false;
bool data_ready = false;

char received_topic[80];
char user_data[256];

void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base,
                        int32_t event_id, void *event_data) {
  esp_mqtt_event_t *event = (esp_mqtt_event_t *)event_data;
  ESP_LOGD(TAG, "MQTT event = %ld", event_id);
  switch (event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT Connected");
    mqtt_connected = true;
    break;

  case MQTT_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "MQTT Disconnected");
    mqtt_connected = false;
    break;
  case MQTT_EVENT_PUBLISHED:
    ESP_LOGI(TAG, "MQTT Published: msg_id = %d", event->msg_id);
    break;
  case MQTT_EVENT_SUBSCRIBED:
    ESP_LOGI(TAG, "MQTT Subscribed: msg_id = %d", event->msg_id);
    break;
  case MQTT_EVENT_DATA:
    ESP_LOGI(TAG, "MQTT Data Received: msg_id = %d, topic = %.*s, data = %.*s",
             event->msg_id, event->topic_len, event->topic, event->data_len,
             event->data);
    memcpy(received_topic, event->topic, event->topic_len);
    received_topic[event->topic_len] = '\0';
    memcpy(user_data, event->data, event->data_len);
    user_data[event->data_len] = '\0';
    data_ready = true;
    break;
  default:
    break;
  }
}


void app_main() {
    dht11_t dht11_sensor;
    dht11_sensor.dht11_pin = CONFIG_DHT11_PIN;
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_err_t err = ESP_FAIL;
  while (err != ESP_OK) {
    err = example_connect();
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Unable to connect to WiFi.");
      vTaskDelay(pdMS_TO_TICKS(10000));
    }
  }

  esp_mqtt_client_config_t mqtt_client_config = {
      .broker.address.uri = "mqtt://demo.thingsboard.io",
      .broker.address.port = 1883,
      .credentials.username = "tjxbpdjn9OhIMf7PgxUa",
  };

  mqtt_client = esp_mqtt_client_init(&mqtt_client_config);
  esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY,
                                 mqtt_event_handler, NULL);

  esp_mqtt_client_start(mqtt_client);

  //esp_mqtt_client_subscribe(mqtt_client, "v1/devices/me/attributes", 1);

    // Read data
    while(1)
    {
      if(!dht11_read(&dht11_sensor, CONFIG_CONNECTION_TIMEOUT))
      {  
      printf("Temperature %.2f \n",dht11_sensor.temperature);
      printf("Humidity %.2f \n",dht11_sensor.humidity);
      }
      vTaskDelay(pdMS_TO_TICKS(50));
    if (!mqtt_connected) {
      esp_mqtt_client_reconnect(mqtt_client);
      continue;
    }
    //char *data = "{\"temperature\":%.2f"}dht11_sensor.temperature;
    char data[50];
    sprintf(data, "{\"temperature\": %.2f}", dht11_sensor.temperature);
    esp_mqtt_client_publish(mqtt_client, "v1/devices/me/telemetry", data, 0, 1,
                            0);
      vTaskDelay(50);
    } 
}
