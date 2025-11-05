# Guia do Usuário - Colunas de Desconexão no sngrep

## Introdução

Este guia explica como usar as duas novas colunas adicionadas ao sngrep para monitorar informações de desconexão em chamadas SIP:

- **Disconnect By** - Mostra quem iniciou o término da chamada
- **Disc Code** - Exibe o código de resposta SIP da desconexão

## Como Ativar as Novas Colunas

### Método 1: Usando a Interface (Recomendado)

1. **Inicie o sngrep**:
   ```bash
   sngrep
   # ou
   sngrep -i eth0  # para captura ao vivo
   # ou
   sngrep arquivo.pcap  # para análise de arquivo
   ```

2. **Na tela de lista de chamadas, pressione F10**:
   - Alternativamente, você pode pressionar `t` ou `T`
   - Isso abrirá o menu de seleção de colunas

3. **Navegue até as novas colunas**:
   - Use as setas ↑↓ para navegar
   - Procure por:
     - `[ ] Who Disconnected` 
     - `[ ] Disconnect SIP Code`

4. **Selecione as colunas desejadas**:
   - Pressione ESPAÇO para marcar/desmarcar
   - As colunas marcadas mostrarão `[*]`

5. **Aplique as mudanças**:
   - Pressione ENTER para confirmar
   - Ou ESC para cancelar

### Método 2: Via Arquivo de Configuração

1. **Localize o arquivo de configuração**:
   ```bash
   # Sistema
   /etc/sngreprc
   
   # Usuário (tem prioridade)
   ~/.sngreprc
   ```

2. **Edite o arquivo**:
   ```bash
   nano ~/.sngreprc
   ```

3. **Adicione as configurações das colunas**:
   ```bash
   # Configuração de exemplo com as novas colunas
   set cl.column0 index
   set cl.column1 sipfrom
   set cl.column1.width 25
   set cl.column2 sipto
   set cl.column2.width 25
   set cl.column3 state
   set cl.column4 disconnectby
   set cl.column4.width 20
   set cl.column5 disconnectcode
   set cl.column5.width 15
   set cl.column6 msgcnt
   set cl.column7 date
   set cl.column8 time
   ```

4. **Salve e reinicie o sngrep**

## Interpretando as Informações

### Coluna "Disconnect By"

Esta coluna mostra o URI SIP de quem enviou a mensagem BYE:

| Valor Exibido | Significado |
|--------------|-------------|
| `sip:1001@192.168.1.10` | Ramal 1001 encerrou a chamada |
| `sip:+551133334444@gateway.com` | Número externo encerrou |
| *(vazio)* | Chamada não foi encerrada com BYE |

### Coluna "Disc Code"

Esta coluna mostra o código de resposta ao BYE:

| Código | Significado |
|--------|------------|
| `200 OK` | Desconexão normal bem-sucedida |
| `481 Call/Transaction Does Not Exist` | Chamada não existe mais |
| `487 Request Terminated` | Requisição foi cancelada |
| `500 Internal Server Error` | Erro no servidor ao processar BYE |
| *(vazio)* | BYE ainda sem resposta ou chamada sem BYE |

## Exemplos de Uso Prático

### Exemplo 1: Identificar Quem Desliga Primeiro

**Cenário**: Você quer saber se os clientes ou o call center está desligando as chamadas.

**Como fazer**:
1. Ative a coluna "Disconnect By"
2. Capture o tráfego durante o horário de pico
3. Analise os padrões:
   - Se a maioria mostra números externos = clientes desligando
   - Se a maioria mostra ramais internos = agentes desligando

### Exemplo 2: Detectar Problemas de Desconexão

**Cenário**: Algumas chamadas estão caindo inesperadamente.

**Como fazer**:
1. Ative ambas as colunas
2. Procure por padrões anormais:
   - Códigos diferentes de "200 OK"
   - Campos vazios (BYE sem resposta)
   - Códigos de erro (4xx, 5xx)

### Exemplo 3: Auditoria de Duração de Chamadas

**Cenário**: Verificar se chamadas curtas são desligadas pelo cliente ou sistema.

**Como fazer**:
1. Configure todas as colunas relevantes:
   ```bash
   set cl.column0 sipfrom
   set cl.column1 sipto  
   set cl.column2 totaldur
   set cl.column3 disconnectby
   set cl.column4 disconnectcode
   ```
2. Filtre chamadas curtas (< 10 segundos)
3. Verifique quem está desconectando

## Filtragem e Busca

### Buscar por Quem Desconectou

1. Pressione `/` ou F3 para abrir o filtro
2. Digite parte do URI que desconectou
3. Exemplo: `/1001` mostrará apenas chamadas desconectadas pelo ramal 1001

### Filtrar por Código de Resposta

Use o filtro de display para mostrar apenas códigos específicos:
- `/200 OK` - Apenas desconexões normais
- `/487` - Apenas chamadas canceladas
- `/!200` - Todas exceto desconexões normais

## Combinando com Outras Funcionalidades

### Visualizar Detalhes da Desconexão

1. Selecione uma chamada na lista
2. Pressione ENTER para ver o fluxo de mensagens
3. Role até o final para ver as mensagens BYE e respostas

### Exportar Dados

1. Pressione F2 ou `s` para salvar
2. Escolha o formato (PCAP, TXT, etc.)
3. As informações de desconexão serão incluídas

### Estatísticas

1. Pressione `i` para ver estatísticas
2. Analise a distribuição de estados de chamadas
3. Cross-reference com as informações de desconexão

## Solução de Problemas

### As colunas não aparecem

**Verificar**:
- Versão do sngrep tem as modificações implementadas
- Arquivo de configuração está correto
- Reinicie o sngrep após mudanças na configuração

### Campos aparecem vazios

**Possíveis causas**:
- Chamada ainda ativa (sem BYE ainda)
- Chamada cancelada com CANCEL ao invés de BYE
- Captura iniciada após o BYE
- Timeout sem mensagem BYE

### Informações incorretas

**Verificar**:
- Sincronização de relógio entre sistemas
- Captura completa do tráfego (sem perda de pacotes)
- Filtros não estão ocultando mensagens relevantes

## Dicas Avançadas

### Script para Análise Automatizada

```bash
#!/bin/bash
# Extrai estatísticas de desconexão
sngrep -c -N -q arquivo.pcap | \
  grep "COMPLETED" | \
  awk '{print $NF}' | \
  sort | uniq -c | sort -rn
```

### Configuração para Call Center

```bash
# ~/.sngreprc otimizado para call center
set cl.column0 time
set cl.column1 sipfromuser
set cl.column1.width 15
set cl.column2 siptouser  
set cl.column2.width 15
set cl.column3 totaldur
set cl.column3.width 8
set cl.column4 disconnectby
set cl.column4.width 20
set cl.column5 disconnectcode
set cl.column6 state
```

### Monitoramento em Tempo Real

```bash
# Alerta quando há muitas desconexões anormais
sngrep -B | while read line; do
  if [[ $line =~ "48[0-9]|5[0-9]{2}" ]]; then
    echo "ALERTA: Desconexão anormal detectada: $line"
    # Enviar notificação, email, etc.
  fi
done
```

## FAQ

**P: As colunas ocupam muito espaço. Como ajustar?**
R: Use a configuração `.width` no arquivo de configuração ou redimensione manualmente na interface.

**P: Posso exportar apenas as informações de desconexão?**
R: Sim, use o filtro para mostrar apenas chamadas COMPLETED e exporte.

**P: Como diferenciar desconexão normal de anormal?**
R: Verifique o código: 200 OK é normal, outros códigos indicam situações especiais.

**P: É possível ver o horário exato da desconexão?**
R: Visualize o fluxo da chamada (ENTER) para ver timestamps detalhados.

## Referências

- [RFC 3261 - SIP Protocol](https://tools.ietf.org/html/rfc3261)
- [SIP Response Codes](https://www.iana.org/assignments/sip-parameters/sip-parameters.xhtml#sip-parameters-7)
- [sngrep Documentation](https://github.com/irontec/sngrep/wiki)
