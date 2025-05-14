#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

#include "webserver.h"
#include "setup.h"


int webserver_init(void) {
    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server) {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    // Vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK) {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }
    
    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);
    
    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");
}


static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}


void user_request(char **request) {
    static int pattern = 0;
    if (strstr(*request, "GET /blue_on") != NULL) {
        set_pattern(pio, sm, pattern, "branco");
        pattern = (pattern + 1) % 3; // Varia entre 0, 1, 2
    }
    else if (strstr(*request, "GET /blue_off") != NULL) {
        clear_matrix(pio, sm);
    }
    else if (strstr(*request, "GET /update") != NULL) {
        temperatura = 30 + rand() % 51; // 30 a 80
        umidade = 30 + rand() % 61;     // 30 a 90
        oxigenio = 10 + rand() % 11;    // 10 a 20
    }
};


static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (!p) {
        tcp_close(tpcb);
        tcp_recv(tpcb, NULL);
        return ERR_OK;
    }

    // Alocação do request na memória dinámica
    char *request = (char *)malloc(p->len + 1);
    memcpy(request, p->payload, p->len);
    request[p->len] = '\0';

    printf("Request: %s\n", request);

    // Tratamento de request - Controle dos LEDs
    user_request(&request);
    
    // Leitura da temperatura interna
    //float temperature = temp_read();
    float temperature = temperatura;;
    float umidade_web = umidade;
    float oxigenio_web = oxigenio;

    // Cria a resposta HTML
    char html[1024];

    // Instruções html do webserver
    snprintf(html, sizeof(html), // Formatar uma string e armazená-la em um buffer de caracteres
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "\r\n"
             "<!DOCTYPE html>\n"
             "<html>\n"
             "<head>\n"
             "<title> Monitoramento da Composteira </title>\n"
             "<style>\n"
             "body { background-color: #f0f0f0; font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }\n"
             "h1 { font-size: 64px; margin-bottom: 30px; }\n"
             "button { background-color:rgb(187, 255, 170); font-size: 36px; margin: 10px; padding: 20px 40px; border-radius: 10px; }\n"
             ".temperature { font-size: 48px; margin-top: 30px; color: #333; }\n"
             "</style>\n"
             "</head>\n"

             "<body>\n"
             "<h1>EmbarcaTech: Monitoramento da Composteira</h1>\n"
             "<form action=\"./blue_on\"><button>Ativar aeracao</button></form>\n"
             "<form action=\"./blue_off\"><button>Desativar aeracao</button></form>\n"
             "<form action=\"./update\"><button>Atualizar dados</button></form>\n"
             "<p class=\"temperature\">Temperatura: %.2f &deg;C</p>\n"
             "<p class=\"temperature\">Umidade: %.2f %</p>\n"
             "<p class=\"temperature\">Oxigenio: %.2f %</p>\n"
             "</body>\n"
             "</html>\n",
             temperature, umidade_web, oxigenio_web);

    // Escreve dados para envio (mas não os envia imediatamente).
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem
    tcp_output(tpcb);

    // Libera memória alocada dinamicamente
    free(request);
    
    // Libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK;
}
