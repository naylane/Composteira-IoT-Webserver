#ifndef WEBSERVER_H
#define WEBSERVER_H

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

// Inicializa o servidor web
void webserver_init(void);

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Tratamento do request do usuário - digite aqui
void user_request(char **request);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);


#endif