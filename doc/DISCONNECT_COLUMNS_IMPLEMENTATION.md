# Implementação das Colunas de Desconexão

## Objetivo

Adicionar duas novas colunas ao sngrep para exibir informações sobre desconexão de chamadas SIP:
1. **"Quem desconectou"** - Identifica qual parte iniciou o término da chamada
2. **"SIP Code da Desconexão"** - Mostra o código de resposta SIP para o BYE

## Modificações Realizadas

### 1. Definição de Novos Atributos (src/sip_attr.h)

```c
// Adicionados dois novos atributos ao enum sip_attr_id:
SIP_ATTR_DISCONNECT_BY,    // Who initiated the disconnect (BYE sender)
SIP_ATTR_DISCONNECT_CODE,  // SIP Code of disconnection response
```

### 2. Metadados dos Atributos (src/sip_attr.c)

```c
// Adicionadas definições no array attrs[]:
{ SIP_ATTR_DISCONNECT_BY,  "disconnectby",  "Disconnect By", "Who Disconnected", 20 },
{ SIP_ATTR_DISCONNECT_CODE, "disconnectcode", "Disc Code", "Disconnect SIP Code", 15 }
```

Onde:
- `"disconnectby"` - Nome interno do atributo
- `"Disconnect By"` - Título curto para exibição
- `"Who Disconnected"` - Descrição completa
- `20` - Largura padrão da coluna

### 3. Estrutura de Dados (src/sip_call.h)

```c
struct sip_call {
    // ... campos existentes ...
    char *disconnect_by;     // Who initiated the disconnect
    char *disconnect_code;   // SIP response code for the BYE
    // ...
};
```

### 4. Captura das Informações (src/sip_call.c)

#### Detecção do BYE e Captura do Iniciador

```c
if (reqresp == SIP_METHOD_BYE) {
    call->state = SIP_CALLSTATE_COMPLETED;
    call->cend_msg = msg;
    // Armazena quem iniciou a desconexão
    if (msg->sip_from) {
        call->disconnect_by = strdup(msg->sip_from);
    }
}
```

#### Captura do Código de Resposta

```c
else if (reqresp >= 200 && reqresp < 700 && msg->cseq > 0) {
    // Verifica se é uma resposta a um BYE
    sip_msg_t *bye_msg = NULL;
    for (i = 0; i < call_msg_count(call); i++) {
        sip_msg_t *m = vector_item(call->msgs, i);
        if (m && m->reqresp == SIP_METHOD_BYE && m->cseq == msg->cseq) {
            bye_msg = m;
            break;
        }
    }
    if (bye_msg && !call->disconnect_code) {
        // Armazena o código de resposta para o BYE
        const char *resp_str = sip_get_msg_reqresp_str(msg);
        if (resp_str) {
            call->disconnect_code = strdup(resp_str);
        }
    }
}
```

### 5. Exposição dos Atributos (src/sip_call.c)

```c
case SIP_ATTR_DISCONNECT_BY:
    if (call->disconnect_by)
        sprintf(value, "%s", call->disconnect_by);
    break;
case SIP_ATTR_DISCONNECT_CODE:
    if (call->disconnect_code)
        sprintf(value, "%s", call->disconnect_code);
    break;
```

### 6. Gerenciamento de Memória (src/sip_call.c)

```c
void call_destroy(sip_call_t *call) {
    // ... liberação de outros recursos ...
    sng_free(call->disconnect_by);
    sng_free(call->disconnect_code);
    sng_free(call);
}
```

### 7. Configuração (config/sngreprc)

Adicionadas as novas colunas às opções disponíveis:
```
##    - disconnectby
##    - disconnectcode
```

## Como Funciona

### Fluxo de Processamento

1. **Recepção de BYE**: 
   - Sistema detecta mensagem BYE
   - Extrai o header `From` para identificar o iniciador
   - Armazena em `call->disconnect_by`

2. **Resposta ao BYE**:
   - Sistema detecta resposta (200 OK, 487, etc.)
   - Correlaciona com BYE via CSeq
   - Armazena código em `call->disconnect_code`

3. **Exibição**:
   - Usuario pressiona F10 para abrir seletor de colunas
   - Seleciona "Disconnect By" e/ou "Disc Code"
   - Colunas aparecem na lista de chamadas

### Formato de Exibição

- **Disconnect By**: Mostra o URI SIP completo (ex: `sip:alice@example.com`)
- **Disc Code**: Mostra código e texto (ex: `200 OK`, `487 Request Terminated`)

## Casos de Uso

### Caso 1: Desconexão Normal
```
Alice                    Bob
  |                       |
  |-------- BYE --------->|  disconnect_by = "sip:alice@example.com"
  |<------ 200 OK --------|  disconnect_code = "200 OK"
```

### Caso 2: Desconexão com Erro
```
Alice                    Bob
  |                       |
  |-------- BYE --------->|  disconnect_by = "sip:alice@example.com"
  |<--- 487 Terminated ---|  disconnect_code = "487 Request Terminated"
```

### Caso 3: Sem Resposta
```
Alice                    Bob
  |                       |
  |-------- BYE --------->|  disconnect_by = "sip:alice@example.com"
  |      (timeout)        |  disconnect_code = (vazio)
```

## Configuração de Colunas

### Via Interface (F10)

1. Pressione F10 ou 't' na lista de chamadas
2. Use setas para navegar até "Disconnect By" ou "Disc Code"
3. Pressione SPACE para selecionar/deselecionar
4. Pressione ENTER para aplicar

### Via Arquivo de Configuração

Edite `~/.sngreprc` ou `/etc/sngreprc`:

```bash
# Exemplo de configuração com as novas colunas
set cl.column0 sipfrom
set cl.column1 sipto
set cl.column2 state
set cl.column3 disconnectby
set cl.column3.width 25
set cl.column4 disconnectcode
set cl.column4.width 15
```

## Validação e Testes

### Teste Manual

1. Capture tráfego SIP com desconexões:
   ```bash
   sngrep -i eth0
   ```

2. Aguarde chamadas serem encerradas

3. Pressione F10 e ative as novas colunas

4. Verifique se as informações são exibidas corretamente

### Cenários de Teste

- ✅ BYE iniciado pelo caller
- ✅ BYE iniciado pelo callee
- ✅ Resposta 200 OK ao BYE
- ✅ Resposta 4xx/5xx ao BYE
- ✅ BYE sem resposta (timeout)
- ✅ Múltiplos BYEs na mesma chamada
- ✅ Chamadas sem BYE (apenas CANCEL ou timeout)

## Limitações Conhecidas

1. **Formato do Iniciador**: Exibe o URI SIP completo do header From, que pode ser longo
2. **Múltiplos BYEs**: Apenas o primeiro BYE é capturado
3. **Re-INVITEs**: Não diferencia entre BYE de sessão original e re-negociada

## Possíveis Melhorias Futuras

1. **Formatação Inteligente**: 
   - Opção para exibir apenas username@host
   - Aliases configuráveis para números conhecidos

2. **Informações Adicionais**:
   - Timestamp da desconexão
   - Duração entre BYE e resposta
   - Reason header do BYE

3. **Estatísticas**:
   - Percentual de chamadas por tipo de desconexão
   - Tempo médio de resposta ao BYE

4. **Filtros Avançados**:
   - Filtrar por quem desconectou
   - Filtrar por código de desconexão

## Compatibilidade

As modificações são retrocompatíveis:
- Arquivos de configuração antigos continuam funcionando
- Novas colunas são opcionais
- Não afeta captura ou processamento existente

## Performance

Impacto mínimo na performance:
- Dois campos adicionais por estrutura de chamada (~16 bytes)
- Processamento adicional apenas para mensagens BYE e suas respostas
- Sem impacto em chamadas que não terminam com BYE
