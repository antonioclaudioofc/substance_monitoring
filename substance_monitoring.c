#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "hardware/uart.h"

#include "ws2818b.pio.h"

// Constantes para conexão com o display OLED
const uint I2C_SDA_PIN = 14;
const uint I2C_SCL_PIN = 15;
#define ANG_PIN 28

// Constantes para escrita dos NEOLEDS
#define LED_COUNT 25
#define LED_PIN 7
#define BRIGHTNESS 2

// Rede wifi para conexão
#define WIFI_SSID "Linux"
#define WIFI_PASS "00000000"

// Constante para conexão do arduino para a placa BitDogLab
#define UART_0_TX 16
#define UART_0_RX 17
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY UART_PARITY_NONE


// Criação do fórmulário para cadastro do pH de substâncias
#define HTTP_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"                                                                                 \
                      "<!DOCTYPE html>"                                                                                                                    \
                      "<html lang=\"pt-br\">"                                                                                                              \
                      "  <head>"                                                                                                                           \
                      "    <meta charset=\"UTF-8\" />"                                                                                                     \
                      "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />"                                                   \
                      "    <title>Monitoramento do pH de Substâncias</title>"                                                                              \
                      "    <script src=\"https://www.gstatic.com/firebasejs/10.9.0/firebase-app-compat.js\"></script>"                                     \
                      "    <script src=\"https://www.gstatic.com/firebasejs/10.9.0/firebase-firestore-compat.js\"></script>"                               \
                      "  </head>"                                                                                                                          \
                      "  <style>"                                                                                                                          \
                      "    .form {"                                                                                                                        \
                      "      max-width: 32rem;"                                                                                                            \
                      "      display: flex;"                                                                                                               \
                      "      flex-direction: column;"                                                                                                      \
                      "      gap: 1rem;"                                                                                                                   \
                      "      margin-block: 1rem;"                                                                                                          \
                      "      text-align: center;"                                                                                                          \
                      "    }"                                                                                                                              \
                      "  </style>"                                                                                                                         \
                      "  <body>"                                                                                                                           \
                      "    <div>"                                                                                                                          \
                      "      <form id=\"substanceForm\" class=\"form\">"                                                                                   \
                      "        <h2>Adicionar Substância</h2>"                                                                                              \
                      "        <input type=\"text\" id=\"name\" placeholder=\"Nome da Substância\" required />"                                            \
                      "        <input type=\"text\" id=\"location\" placeholder=\"Local\" required />"                                                     \
                      "        <input type=\"number\" step=\"0.1\" id=\"pHValue\" placeholder=\"Valor do pH\" required />"                                 \
                      "        <input type=\"text\" id=\"notes\" placeholder=\"Observações (opcional)\" />"                                                \
                      "        <select id=\"status\">"                                                                                                     \
                      "          <option value=\"Normal\">Normal</option>"                                                                                 \
                      "          <option value=\"Ácido\">Ácido</option>"                                                                                   \
                      "          <option value=\"Alcalino\">Alcalino</option>"                                                                             \
                      "        </select>"                                                                                                                  \
                      "        <button type=\"submit\">Adicionar Substância</button>"                                                                      \
                      "        <a href=\"https://monitoring-ph-substances.vercel.app/\" target=\"_blank\">VISUALIZAR TODAS AS SUBSTÂNCIAS CADASTRADAS</a>" \
                      "      </form>"                                                                                                                      \
                      "      <p id=\"isSend\"></p>"                                                                                                        \
                      "    </div>"                                                                                                                         \
                      "  </body>"                                                                                                                          \
                      "  <script>"                                                                                                                         \
                      "    const firebaseConfig = {"                                                                                                       \
                      "      apiKey: \"AIzaSyA2EuaDL8fI6z4MI-SHvJXuzKDHuOBNUCg\","                                                                         \
                      "      authDomain: \"embartatech.firebaseapp.com\","                                                                                 \
                      "      projectId: \"embartatech\","                                                                                                  \
                      "      storageBucket: \"embartatech.firebasestorage.app\","                                                                          \
                      "      messagingSenderId: \"395399776256\","                                                                                         \
                      "      appId: \"1:395399776256:web:e24f519acf39b99c8370e5\","                                                                        \
                      "    };"                                                                                                                             \
                      "    firebase.initializeApp(firebaseConfig);"                                                                                        \
                      "    const db = firebase.firestore();"                                                                                               \
                      "    document.getElementById(\"substanceForm\").onsubmit = function(event) {"                                                        \
                      "      event.preventDefault();"                                                                                                      \
                      "      const name = document.getElementById(\"name\").value;"                                                                        \
                      "      const location = document.getElementById(\"location\").value;"                                                                \
                      "      const pHValue = parseFloat(document.getElementById(\"pHValue\").value);"                                                      \
                      "      const notes = document.getElementById(\"notes\").value;"                                                                      \
                      "      const status = document.getElementById(\"status\").value;"                                                                    \
                      "      const newSubstance = {"                                                                                                       \
                      "        name,"                                                                                                                      \
                      "        location,"                                                                                                                  \
                      "        pHValue,"                                                                                                                   \
                      "        notes,"                                                                                                                     \
                      "        status,"                                                                                                                    \
                      "        date: new Date(),"                                                                                                          \
                      "      };"                                                                                                                           \
                      "      db.collection(\"substances\")"                                                                                                \
                      "        .add(newSubstance)"                                                                                                         \
                      "        .then(() => {"                                                                                                              \
                      "          document.getElementById(\"isSend\").innerText = \"Substância adicionada com sucesso!\";"                                  \
                      "          document.getElementById(\"substanceForm\").reset(); "                                                                     \
                      "        })"                                                                                                                         \
                      "        .catch((error) => {"                                                                                                        \
                      "          document.getElementById(\"isSend\").innerText = \"Erro ao adicionar substância!\";"                                       \
                      "          console.error(error);"                                                                                                    \
                      "        });"                                                                                                                        \
                      "    };"                                                                                                                             \
                      "  </script>"                                                                                                                        \
                      "</html>\r\n"

struct pixel_t
{
    uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t;

npLED_t leds[LED_COUNT];

PIO np_pio;
uint sm;

void npInit(uint pin)
{
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0;

    sm = pio_claim_unused_sm(np_pio, false);
    if (sm < 0)
    {
        np_pio = pio1;
        sm = pio_claim_unused_sm(np_pio, true);
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    for (int i = 0; i < LED_COUNT; i++)
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b)
{
    if (index == 12)
    {
        leds[index].R = r;
        leds[index].G = g;
        leds[index].B = b;
    }
}

void setMatrixColor(uint8_t r, uint8_t g, uint8_t b)
{
    for (uint i = 0; i < LED_COUNT; i++)
    {
        npSetLED(i, r, g, b);
    }
}

void npClear()
{
    for (uint i = 0; i < LED_COUNT; i++)
    {
        npSetLED(i, 0, 0, 0);
    }
}

void npWrite()
{
    for (uint i = 0; i < LED_COUNT; ++i)
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);
        pio_sm_put_blocking(np_pio, sm, leds[i].R);
        pio_sm_put_blocking(np_pio, sm, leds[i].B);
    }
    sleep_us(100);
}

const npLED_t colors_ph[15] = {
    {221, 31, 31},  // 0
    {224, 105, 1},  // 1
    {232, 201, 0},  // 2
    {232, 239, 1},  // 3
    {173, 214, 0},  // 4
    {128, 197, 28}, // 5
    {85, 182, 60},  // 6
    {63, 169, 69},  // 7
    {55, 182, 111}, // 8
    {44, 185, 202}, // 9
    {68, 142, 227}, // 10
    {51, 83, 186},  // 11
    {82, 81, 183},  // 12
    {91, 69, 180},  // 13
    {62, 43, 150},  // 14
};

static err_t http_callback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    if (p == NULL)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *request = (char *)p->payload;

    tcp_write(tpcb, HTTP_RESPONSE, strlen(HTTP_RESPONSE), TCP_WRITE_FLAG_COPY);

    pbuf_free(p);

    return ERR_OK;
}

static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, http_callback);
    return ERR_OK;
}

static void start_http_server(void)
{
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb)
    {
        printf("Erro ao criar PCB\n");
        return;
    }

    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }

    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);

    printf("Servidor HTTP running...\n");
}

void clear_uart_fifo(uart_inst_t *uart)
{
    while (uart_is_readable(uart))
    {
        uart_getc(uart);
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(10000);
    printf("Iniciando servidor HTTP\n");

    if (cyw43_arch_init())
    {
        printf("Erro ao inicializar o Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    printf("Conectando ao Wi-Fi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        return 1;
    }
    else
    {
        printf("Connected.\n");
        uint8_t *ip_address = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
        printf("Endereço IP http://%d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
    }

    start_http_server();

    npInit(LED_PIN);

    i2c_init(i2c1, SSD1306_I2C_CLK * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    uart_init(uart0, BAUD_RATE);
    gpio_set_function(UART_0_TX, GPIO_FUNC_UART);
    gpio_set_function(UART_0_RX, GPIO_FUNC_UART);

        

    uart_set_hw_flow(uart0, false, false);
    uart_set_format(uart0, DATA_BITS, STOP_BITS, PARITY);
    adc_init();

    adc_gpio_init(ANG_PIN);

    SSD1306_init();

    struct render_area frame_area = {
        start_col : 0,
        end_col : SSD1306_WIDTH - 1,
        start_page : 0,
        end_page : SSD1306_NUM_PAGES - 1
    };
    
    calc_render_area_buflen(&frame_area);

    uint8_t buf[SSD1306_BUF_LEN];
    memset(buf, 0, SSD1306_BUF_LEN);
    render(buf, &frame_area);

restart:

    SSD1306_scroll(true);
    sleep_ms(100);
    SSD1306_scroll(false);

    char text[4][20] = {
        "  ",
        "    pH: --.--  ",
        "   ",
        "  "};

    while (true)
    {
        cyw43_arch_poll();
        if (uart_is_readable(uart0))
        {
            char read_uart_1[10];
            int index = 0;

            while (uart_is_readable(uart0) && index < sizeof(read_uart_1) - 1)
            {
                read_uart_1[index] = uart_getc(uart0);
                index++;
            }
            read_uart_1[index] = '\0'; 

            for (int i = 0; i < index; i++)
            {
                if (read_uart_1[i] == ',')
                {
                    read_uart_1[i] = '.'; 
                    break;
                }
            }

            float ph = atof(read_uart_1);

            snprintf(text[1], 20, "   pH: %.2f", ph);

            int y = 0;
            for (uint i = 0; i < count_of(text); i++)
            {
                WriteString(buf, 5, y, text[i]);
                y += 8;
            }
            render(buf, &frame_area);

            setMatrixColor(colors_ph[(int)ph].R, colors_ph[(int)ph].G, colors_ph[(int)ph].B);
            npWrite();
        }

        sleep_ms(100);
    }
    return 0;
}