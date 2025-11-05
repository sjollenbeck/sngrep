# Guia da API para Desenvolvedores - sngrep

## Visão Geral

Este documento descreve a API interna e os pontos de extensão do sngrep para desenvolvedores que desejam adicionar novas funcionalidades ou modificar o comportamento existente.

## Arquitetura de Plugins (Atributos)

### Adicionando Novos Atributos SIP

#### 1. Definir o Enum (sip_attr.h)
```c
enum sip_attr_id {
    // ... atributos existentes ...
    SIP_ATTR_MEU_NOVO_ATRIBUTO,  // Adicione antes de SIP_ATTR_COUNT
    SIP_ATTR_COUNT
};
```

#### 2. Registrar Metadados (sip_attr.c)
```c
static sip_attr_hdr_t attrs[SIP_ATTR_COUNT] = {
    // ... atributos existentes ...
    { 
        SIP_ATTR_MEU_NOVO_ATRIBUTO,    // ID
        "meuatributo",                  // Nome interno
        "Meu Attr",                     // Título curto
        "Meu Novo Atributo",           // Descrição
        15,                             // Largura padrão
        minha_funcao_cor               // Função de coloração (opcional)
    }
};
```

#### 3. Implementar Getter (sip_call.c ou sip_msg.c)
```c
case SIP_ATTR_MEU_NOVO_ATRIBUTO:
    // Lógica para obter o valor
    if (call->meu_campo) {
        sprintf(value, "%s", call->meu_campo);
    }
    break;
```

### Exemplo Completo: Adicionando "User Agent"

```c
// 1. Em sip_attr.h
SIP_ATTR_USER_AGENT,

// 2. Em sip_attr.c
{ SIP_ATTR_USER_AGENT, "useragent", "UA", "User Agent", 30 },

// 3. Em sip_msg.h - adicionar campo
struct sip_msg {
    // ...
    char *user_agent;
};

// 4. Em sip.c - capturar o header
if (regexec(&regex_useragent, payload, 2, pmatch, 0) == 0) {
    msg->user_agent = strndup(
        payload + pmatch[1].rm_so,
        pmatch[1].rm_eo - pmatch[1].rm_so
    );
}

// 5. Em sip_msg.c - retornar valor
case SIP_ATTR_USER_AGENT:
    if (msg->user_agent)
        sprintf(value, "%s", msg->user_agent);
    break;
```

## API de Captura

### Interface de Captura

```c
// Estrutura principal de captura
typedef struct capture_manager {
    pcap_t *handle;           // Handle do libpcap
    int running;              // Estado da captura
    int paused;               // Captura pausada
    pthread_t thread;         // Thread de captura
} capture_manager_t;

// Funções principais
int capture_init();           // Inicializar sistema de captura
int capture_start();          // Iniciar captura
void capture_stop();          // Parar captura
void capture_pause();         // Pausar/resumir
packet_t* capture_packet();  // Obter próximo pacote
```

### Adicionando Novo Protocolo de Transporte

```c
// 1. Definir tipo em capture.h
enum packet_type {
    PACKET_TCP,
    PACKET_UDP,
    PACKET_TLS,
    PACKET_MEU_PROTOCOLO  // Novo
};

// 2. Implementar parser em capture.c
int capture_parse_meu_protocolo(packet_t *packet, u_char *data, int len) {
    // Validar protocolo
    if (!is_meu_protocolo(data))
        return 0;
    
    // Extrair payload SIP
    packet->payload = extract_sip_payload(data, len);
    packet->type = PACKET_MEU_PROTOCOLO;
    return 1;
}

// 3. Adicionar ao pipeline
if (capture_parse_meu_protocolo(packet, data, len)) {
    return packet;
}
```

## API de Interface (ncurses)

### Criando Novo Painel

```c
// 1. Definir tipo em ui_manager.h
enum panel_types {
    // ...
    PANEL_MEU_PAINEL
};

// 2. Criar arquivos
// src/curses/ui_meu_painel.h
// src/curses/ui_meu_painel.c

// 3. Estrutura básica
typedef struct meu_painel_info {
    WINDOW *win;
    // Dados específicos do painel
} meu_painel_info_t;

// 4. Implementar interface
ui_t ui_meu_painel = {
    .type = PANEL_MEU_PAINEL,
    .create = meu_painel_create,
    .destroy = meu_painel_destroy,
    .draw = meu_painel_draw,
    .handle_key = meu_painel_handle_key,
    .help = meu_painel_help
};

// 5. Registrar em ui_manager.c
ui_t* ui_create(enum panel_types type) {
    switch(type) {
        case PANEL_MEU_PAINEL:
            return &ui_meu_painel;
    }
}
```

### Exemplo: Painel de Estatísticas Avançadas

```c
// ui_advanced_stats.c
void advanced_stats_create(ui_t *ui) {
    // Criar janela
    ui->win = newwin(LINES-2, COLS, 1, 0);
    ui->panel = new_panel(ui->win);
    
    // Inicializar dados
    advanced_stats_info_t *info = malloc(sizeof(advanced_stats_info_t));
    set_panel_userptr(ui->panel, info);
    
    // Configurar título
    mvwprintw(ui->win, 0, 0, "Advanced Statistics");
}

int advanced_stats_handle_key(ui_t *ui, int key) {
    switch(key) {
        case KEY_UP:
            // Navegar para cima
            break;
        case 'r':
            // Refresh estatísticas
            break;
        case 'q':
        case KEY_ESC:
            return KEY_PROPAGATED;  // Voltar ao painel anterior
    }
    return KEY_HANDLED;
}
```

## API de Processamento SIP

### Parser de Headers Customizados

```c
// Adicionar regex em sip.c
regex_t regex_meu_header;
regcomp(&regex_meu_header, "^X-Meu-Header:[ ]*(.+)\r$", REG_EXTENDED);

// Parser function
void sip_parse_meu_header(sip_msg_t *msg, const u_char *payload) {
    regmatch_t pmatch[2];
    if (regexec(&regex_meu_header, payload, 2, pmatch, 0) == 0) {
        msg->meu_header = strndup(
            payload + pmatch[1].rm_so,
            pmatch[1].rm_eo - pmatch[1].rm_so
        );
    }
}
```

### Detectores de Estado Customizados

```c
// Adicionar novo estado em sip_call.h
enum call_state {
    // ...
    SIP_CALLSTATE_MEU_ESTADO = 100
};

// Implementar detector em call_update_state()
if (is_meu_condicao(msg)) {
    call->state = SIP_CALLSTATE_MEU_ESTADO;
    // Lógica adicional
}

// Adicionar string representation
case SIP_CALLSTATE_MEU_ESTADO:
    return "MEU ESTADO";
```

## API de Filtragem

### Criando Filtro Customizado

```c
// 1. Definir função de filtro
int filter_meu_criterio(sip_call_t *call, const char *expr) {
    // Retornar 1 se deve mostrar, 0 se deve ocultar
    return (call->meu_campo && strstr(call->meu_campo, expr));
}

// 2. Registrar em filter.c
filter_t filters[] = {
    // ...
    { "meu_filtro", filter_meu_criterio }
};

// 3. Usar via configuração
// set filter.meu_filtro valor_busca
```

## API de Persistência

### Salvando Dados Customizados

```c
// Formato PCAP customizado
void save_with_metadata(FILE *file, sip_call_t *call) {
    // Escrever header customizado
    fprintf(file, "X-Custom-Data: %s\n", call->custom_data);
    
    // Escrever pacotes
    vector_iter_t it = vector_iterator(call->msgs);
    while ((msg = vector_iterator_next(&it))) {
        pcap_dump(file, msg->packet->header, msg->packet->data);
    }
}

// Formato texto customizado
void export_custom_format(FILE *file, sip_call_t *call) {
    fprintf(file, "Call-ID: %s\n", call->callid);
    fprintf(file, "Custom-Field: %s\n", call->custom_field);
    // ...
}
```

## Hooks e Callbacks

### Sistema de Eventos

```c
// Definir tipos de eventos
enum event_type {
    EVENT_CALL_CREATED,
    EVENT_CALL_DESTROYED,
    EVENT_MSG_RECEIVED,
    EVENT_STATE_CHANGED
};

// Estrutura de callback
typedef void (*event_callback_t)(int event, void *data);

// Registrar callback
void register_event_callback(int event, event_callback_t callback) {
    callbacks[event] = callback;
}

// Disparar evento
void trigger_event(int event, void *data) {
    if (callbacks[event]) {
        callbacks[event](data);
    }
}

// Exemplo de uso
void on_call_completed(int event, void *data) {
    sip_call_t *call = (sip_call_t*)data;
    log_call_completion(call);
    send_notification(call);
}

register_event_callback(EVENT_STATE_CHANGED, on_call_completed);
```

## Integração com Bibliotecas Externas

### Exemplo: Integração com Redis

```c
// 1. Adicionar dependência em configure.ac
AC_CHECK_LIB([hiredis], [redisConnect])

// 2. Criar módulo
// src/storage_redis.c
#include <hiredis/hiredis.h>

redisContext *redis;

void storage_redis_init(const char *host, int port) {
    redis = redisConnect(host, port);
}

void storage_redis_save_call(sip_call_t *call) {
    redisCommand(redis, "SET call:%s %s", 
                 call->callid, 
                 serialize_call(call));
}

sip_call_t* storage_redis_load_call(const char *callid) {
    redisReply *reply = redisCommand(redis, "GET call:%s", callid);
    return deserialize_call(reply->str);
}
```

## Testes Unitários

### Framework de Testes

```c
// tests/test_minha_funcionalidade.c
#include <assert.h>
#include "sip.h"

void test_meu_atributo() {
    // Setup
    sip_call_t *call = call_create("test@callid", NULL);
    call->meu_campo = strdup("valor_teste");
    
    // Test
    char value[256];
    call_get_attribute(call, SIP_ATTR_MEU_ATRIBUTO, value);
    
    // Assert
    assert(strcmp(value, "valor_teste") == 0);
    
    // Cleanup
    call_destroy(call);
}

int main() {
    test_meu_atributo();
    printf("All tests passed!\n");
    return 0;
}
```

## Debug e Profiling

### Macros de Debug

```c
// Em config.h
#ifdef DEBUG
#define DPRINT(fmt, ...) \
    fprintf(stderr, "[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DPRINT(fmt, ...)
#endif

// Uso
DPRINT("Processing call %s", call->callid);
```

### Instrumentação

```c
// Timing
#include <time.h>

clock_t start = clock();
// Código a medir
process_heavy_operation();
clock_t end = clock();

double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
DPRINT("Operation took %f seconds", cpu_time);
```

## Melhores Práticas

### Gerenciamento de Memória

```c
// SEMPRE verificar alocações
char *buffer = malloc(size);
if (!buffer) {
    return ENOMEM;
}

// SEMPRE liberar memória
free(buffer);
buffer = NULL;  // Evitar use-after-free

// Usar wrappers seguros
#define SAFE_FREE(ptr) do { \
    if (ptr) { \
        free(ptr); \
        ptr = NULL; \
    } \
} while(0)
```

### Thread Safety

```c
// Usar mutexes para dados compartilhados
pthread_mutex_t calls_mutex = PTHREAD_MUTEX_INITIALIZER;

void add_call_thread_safe(sip_call_t *call) {
    pthread_mutex_lock(&calls_mutex);
    vector_append(calls_list, call);
    pthread_mutex_unlock(&calls_mutex);
}
```

### Error Handling

```c
// Retornar códigos de erro consistentes
enum error_codes {
    ERR_SUCCESS = 0,
    ERR_MEMORY = -1,
    ERR_INVALID = -2,
    ERR_NOT_FOUND = -3
};

// Propagar erros adequadamente
int process_call(sip_call_t *call) {
    int ret;
    
    if ((ret = validate_call(call)) != ERR_SUCCESS) {
        return ret;
    }
    
    if ((ret = save_call(call)) != ERR_SUCCESS) {
        rollback_call(call);
        return ret;
    }
    
    return ERR_SUCCESS;
}
```

## Contribuindo

### Checklist para Pull Requests

- [ ] Código compila sem warnings
- [ ] Testes passam
- [ ] Documentação atualizada
- [ ] Sem memory leaks (valgrind)
- [ ] Segue style guide
- [ ] Changelog atualizado

### Style Guide

```c
// Indentação: 4 espaços
// Chaves: Estilo K&R
if (condition) {
    // código
} else {
    // código
}

// Nomes: snake_case
int minha_funcao(char *meu_parametro);

// Constantes: UPPER_CASE
#define MAX_BUFFER_SIZE 1024

// Comentários: Estilo C
/* Comentário multi-linha
 * continua aqui
 */
```

## Recursos

- [Documentação Doxygen](../html/index.html)
- [Wiki do Projeto](https://github.com/irontec/sngrep/wiki)
- [Issue Tracker](https://github.com/irontec/sngrep/issues)
- [Mailing List](mailto:sngrep-dev@lists.irontec.com)

---

*Versão API: 2.0 | Última atualização: Novembro 2024*
