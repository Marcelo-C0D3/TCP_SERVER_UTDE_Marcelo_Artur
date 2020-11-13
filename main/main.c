/*
	Autor: Melhor aluno do Prof. Vagner Rodrigues ->> Marcelo Loch e Arturo Dozão
	Objetivo: programa com ESP32 em modo servidor Socket TCP com funcionalidades.
	Disciplina: IoT Aplicada
	Curso: Engenharia da Computação
*/

/* BSD Socket API Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* Inclusão das Bibliotecas */
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "../libs/ultrasonic/ultrasonic.h"
#include "../libs/dht/dht.h"
#include "driver/gpio.h"

/* Definições e Constantes */
#define TRUE 1
#define FALSE 0
#define DEBUG TRUE
#define LED_BIT BIT0

#define LED_B GPIO_NUM_14 //led pin
#define MAX_DISTANCE_CM 100 // 5m max
#define TRIGGER_GPIO GPIO_NUM_2 //ultrasonic HC-SR04 pin
#define ECHO_GPIO GPIO_NUM_13   //ultrasonic HC-SR04 pin
#define DHT_GPIO GPIO_NUM_0     //dht11 pin
#define PORT CONFIG_EXAMPLE_PORT

/* Variáveis Globais */
static const char *TAG = "TCP_UTD";
static const dht_sensor_type_t sensor_type = DHT_TYPE_DHT11;
static const gpio_num_t dht_gpio = DHT_GPIO;
static EventGroupHandle_t LED_event_group; //Cria o objeto do grupo de eventos

QueueHandle_t bufferDistance;
QueueHandle_t bufferTemperatura;
QueueHandle_t bufferUmidade;

void sendMessages(int sock, uint32_t value, char inf[15]);

static void do_retransmit(const int sock)
{
    int len;
    char rx_buffer[128];
    uint32_t distance;
    uint16_t temp, umd;

    do
    {
        xQueueReceive(bufferDistance, &distance, pdMS_TO_TICKS(2000));
        xQueueReceive(bufferTemperatura, &temp, pdMS_TO_TICKS(2000));
        xQueueReceive(bufferUmidade, &umd, pdMS_TO_TICKS(2000));
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0)
        {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        }
        else if (len == 0)
        {
            ESP_LOGW(TAG, "Connection closed");
        }
        else
        {
            //Alguma mensagem foi recebida.
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);
            if (len >= 4)
            {
                if (strcmp(rx_buffer, "LEDB") == 0)
                {
                    xEventGroupSetBits(LED_event_group, LED_BIT);
                    //xQueueReceive();
                    //send(sock, temperatura).
                }
                else if (strcmp(rx_buffer, "DIST") == 0)
                {
                    ESP_LOGI(TAG, "distancia: %d", distance);
                    char rotule[15] = "\nDistancia:";
                    sendMessages(sock, distance, rotule);

                }else if (strcmp(rx_buffer, "TEMP") == 0)
                {
                    ESP_LOGI(TAG, "Temperatura: %d", temp);
                    char rotule[15] = "\nTemperatura:";
                    sendMessages(sock, temp, rotule);

                }else if (strcmp(rx_buffer, "UMID") == 0)
                {
                    ESP_LOGI(TAG, "Umidade: %d", umd);
                    char rotule[15] = "\nUmidade:";
                    sendMessages(sock, umd, rotule);

                }else{
                    ESP_LOGI(TAG, "O valor nao corresponde.");
                    char rotule[128] = "\nCodigo nao corresponde! \ntente:\n 'LEDB'\n'DIST'\n'TEMP'\n'UMID'";
                    umd = 0;
                    sendMessages(sock, umd, rotule);
                }
            }
        }
    } while (len > 0);
}
void sendMessages(int sock, uint32_t value, char inf[15])
{
    char res[64];
    sprintf(res, "%i", value);
    int to_write = strlen(strcat(inf, res));
    while (to_write > 0)
    {
        int written = send(sock, strcat(inf, res), to_write, 0);
        if (written < 0)
        {
            ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
        }
        to_write -= written;
    }
}
void task_toogleLED(void *pvParameter)
{
    bool estadoLed = 0;

    while (TRUE)
    {
        xEventGroupWaitBits(LED_event_group, LED_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        estadoLed = !estadoLed;
        gpio_set_level(LED_B, estadoLed);
        ESP_LOGI(TAG, "LED invertido!");
        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

static void tcp_server_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;

#ifdef CONFIG_EXAMPLE_IPV4
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
#else // IPV6
    struct sockaddr_in6 dest_addr;
    bzero(&dest_addr.sin6_addr.un, sizeof(dest_addr.sin6_addr.un));
    dest_addr.sin6_family = AF_INET6;
    dest_addr.sin6_port = htons(PORT);
    addr_family = AF_INET6;
    ip_protocol = IPPROTO_IPV6;
    inet6_ntoa_r(dest_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
#endif

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0)
    {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0)
    {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

    while (1)
    {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
        uint addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Convert ip address to string
        if (source_addr.sin6_family == PF_INET)
        {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
        }
        else if (source_addr.sin6_family == PF_INET6)
        {
            inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

        do_retransmit(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}

void task_ultrasonic(void *pvParamters)
{
    ultrasonic_sensor_t sensor = {
        .trigger_pin = TRIGGER_GPIO,
        .echo_pin = ECHO_GPIO};
    ultrasonic_init(&sensor);
    uint32_t distance;

    while (TRUE)
    {

        esp_err_t res = ultrasonic_measure_cm(&sensor, MAX_DISTANCE_CM, &distance);
        if (res != ESP_OK)
        {
            printf("Error: ");
            switch (res)
            {
            case ESP_ERR_ULTRASONIC_PING:
                printf("Cannot ping (device is in invalid state)\n");
                break;
            case ESP_ERR_ULTRASONIC_PING_TIMEOUT:
                printf("Ping timeout (no device found)\n");
                break;
            case ESP_ERR_ULTRASONIC_ECHO_TIMEOUT:
                printf("Echo timeout (i.e. distance too big)\n");
                break;
            default:
                printf("%d\n", res);
            }
        }
        else
            xQueueSend(bufferDistance, &distance, pdMS_TO_TICKS(0));

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void task_dht(void *pvParameters)
{
    int16_t temperature = 0;
    int16_t humidity = 0;
    gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);
    while (1)
    {
        if (dht_read_data(sensor_type, dht_gpio, &humidity, &temperature) == ESP_OK)
        {
            humidity = humidity / 10;       //Quem tem sensor
            temperature = temperature / 10; //Quem tem sensor
            xQueueSend(bufferTemperatura, &temperature, pdMS_TO_TICKS(0));
            xQueueSend(bufferUmidade, &humidity, pdMS_TO_TICKS(0));
            //umidade = esp_random()/1000000;
            //temperatura = esp_random()/1000000;
            //ESP_LOGI(TAG, "Umidade %d%% e Temperatura %dºC", umidade, temperatura);
        }
        else
        {
            printf("Could not read data from sensor\n");
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void app_main()
{
    LED_event_group = xEventGroupCreate(); //Cria o grupo de eventos
    bufferDistance = xQueueCreate(5, sizeof(uint32_t));
    bufferTemperatura = xQueueCreate(5, sizeof(uint16_t));
    bufferUmidade = xQueueCreate(5, sizeof(uint16_t));

    ESP_LOGI(TAG, "[APP] Inicio..");
    ESP_LOGI(TAG, "[APP] Memória Livre: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] Versão IDF: %s", esp_get_idf_version());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    gpio_pad_select_gpio(LED_B);
    gpio_set_direction(LED_B, GPIO_MODE_OUTPUT);

    xTaskCreate(tcp_server_task, "tcp_server", 4096, NULL, 5, NULL);
    xTaskCreate(task_toogleLED, "task_toogleLED", 4096, NULL, 5, NULL);
    xTaskCreate(task_dht, "task_dht", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    xTaskCreate(task_ultrasonic, "task_ultrasonic", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);
    ESP_LOGI(TAG, "[APP] Memória Livre Após Criação das Tasks: %d bytes", esp_get_free_heap_size());
}
