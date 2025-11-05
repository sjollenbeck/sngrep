# Fix: Usar Descrições Padrão RFC para Códigos SIP

## Problema Identificado
Servidores SIP enviando descrições personalizadas e **incorretas** para códigos de resposta:

### Exemplos Observados:
| Código | Descrição do Servidor (ERRADA) | Descrição RFC (CORRETA) |
|--------|--------------------------------|-------------------------|
| 480 | "Forbidden" ❌ | "Temporarily Unavailable" ✅ |
| 404 | "Call Failed" ❌ | "Not Found" ✅ |
| 403 | "Not Authorized" ❌ | "Forbidden" ✅ |

## Causa Raiz
O código estava usando a descrição enviada pelo servidor quando ela diferia da padrão:

```c
// ANTES - Usava descrição customizada do servidor
const char *sip_get_msg_reqresp_str(sip_msg_t *msg) {
    if (msg->resp_str) {  // Descrição do servidor
        return msg->resp_str;
    } else {
        return sip_method_str(msg->reqresp);  // Descrição padrão
    }
}
```

Problema: Servidores podem enviar descrições **incorretas ou não-padrão**.

## Solução Implementada

### Sempre usar a descrição padrão RFC:
```c
// DEPOIS - Sempre usa descrição RFC padrão
const char *sip_get_msg_reqresp_str(sip_msg_t *msg) {
    // Always return standard RFC description
    // Ignore custom server descriptions which may be wrong
    return sip_method_str(msg->reqresp);
}
```

## Benefícios

1. **Consistência**: Sempre mostra a descrição correta segundo RFC
2. **Clareza**: Usuário entende imediatamente o significado do código
3. **Conformidade**: Segue os padrões SIP estabelecidos

## Tabela de Códigos SIP Padrão (src/sip.c)

### Códigos Comuns de Desconexão:
| Código | Descrição RFC |
|--------|--------------|
| 180 | "180 Ringing" |
| 181 | "181 Call is Being Forwarded" |
| 183 | "183 Session Progress" |
| 200 | "200 OK" |
| 401 | "401 Unauthorized" |
| 403 | "403 Forbidden" |
| 404 | "404 Not Found" |
| 407 | "407 Proxy Authentication Required" |
| 480 | "480 Temporarily Unavailable" |
| 486 | "486 Busy Here" |
| 487 | "487 Request Terminated" |
| 488 | "488 Not Acceptable Here" |
| 503 | "503 Service Unavailable" |
| 600 | "600 Busy Everywhere" |
| 603 | "603 Decline" |

## Teste de Validação

```bash
cd /var/www/devel/sngrep
make clean && make
./src/sngrep
```

### O que verificar:
1. Chamadas com código **480** devem mostrar "480 Temporarily Unavailable"
2. Chamadas com código **404** devem mostrar "404 Not Found"
3. Chamadas com código **603** devem mostrar "603 Decline"
4. **NENHUMA** descrição personalizada incorreta deve aparecer

## Resultado Esperado

✅ Todas as descrições de códigos SIP seguem o padrão RFC
✅ Não há mais confusão com descrições incorretas dos servidores
✅ Interface consistente e profissional
