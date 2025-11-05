# Documentação Técnica - sngrep

## Visão Geral

Esta pasta contém a documentação técnica completa do projeto sngrep, incluindo análises detalhadas da arquitetura, implementações de funcionalidades e guias de usuário.

## Documentos Disponíveis

### 1. [TECHNICAL_ANALYSIS.md](TECHNICAL_ANALYSIS.md)
**Análise Técnica Completa do Projeto**

Documento abrangente que detalha:
- Arquitetura do sistema
- Estrutura de diretórios e componentes
- Fluxo de dados e processamento
- Sistema de atributos e estados SIP
- Gerenciamento de memória
- Otimizações de performance
- Dependências e build system

**Público-alvo**: Desenvolvedores, arquitetos de software, contribuidores

---

### 2. [DISCONNECT_COLUMNS_IMPLEMENTATION.md](DISCONNECT_COLUMNS_IMPLEMENTATION.md)
**Implementação das Colunas de Desconexão**

Documentação técnica detalhada sobre a implementação das novas colunas:
- Modificações realizadas no código
- Fluxo de processamento de BYE/respostas
- Estruturas de dados modificadas
- Casos de uso e cenários
- Limitações e melhorias futuras

**Público-alvo**: Desenvolvedores, mantenedores do projeto

---

### 3. [USER_GUIDE_DISCONNECT_COLUMNS.md](USER_GUIDE_DISCONNECT_COLUMNS.md)
**Guia do Usuário - Colunas de Desconexão**

Manual prático para usuários finais:
- Como ativar as novas colunas
- Interpretação das informações
- Exemplos de uso prático
- Solução de problemas
- Dicas avançadas e FAQ

**Público-alvo**: Usuários finais, administradores de sistemas, suporte técnico

---

### 4. [sngrep.8](sngrep.8)
**Manual Page (man)**

Documentação oficial no formato man page do Unix/Linux:
- Sinopse e descrição
- Opções de linha de comando
- Teclas de atalho
- Arquivos de configuração
- Exemplos de uso

**Uso**: `man ./sngrep.8`

---

## Estrutura da Documentação

```
doc/
├── README.md                                # Este arquivo
├── TECHNICAL_ANALYSIS.md                   # Análise técnica do projeto
├── DISCONNECT_COLUMNS_IMPLEMENTATION.md    # Detalhes de implementação
├── USER_GUIDE_DISCONNECT_COLUMNS.md        # Guia do usuário
├── sngrep.8                               # Man page
├── footer.html                            # Rodapé para documentação HTML
└── ironlogo.png                          # Logo da Irontec
```

## Navegação Rápida

### Para Desenvolvedores
1. Comece com [TECHNICAL_ANALYSIS.md](TECHNICAL_ANALYSIS.md) para entender a arquitetura
2. Consulte [DISCONNECT_COLUMNS_IMPLEMENTATION.md](DISCONNECT_COLUMNS_IMPLEMENTATION.md) para exemplo de implementação

### Para Usuários
1. Leia [USER_GUIDE_DISCONNECT_COLUMNS.md](USER_GUIDE_DISCONNECT_COLUMNS.md) para usar as novas funcionalidades
2. Consulte o man page (`man sngrep`) para referência completa

### Para Suporte
1. Use [USER_GUIDE_DISCONNECT_COLUMNS.md](USER_GUIDE_DISCONNECT_COLUMNS.md) para treinar usuários
2. Consulte a seção de FAQ e Solução de Problemas

## Convenções Utilizadas

### Formatação de Código
- **C**: Snippets de código fonte
- **Bash**: Comandos e scripts
- **Configuration**: Exemplos de configuração

### Diagramas
- **Mermaid**: Fluxogramas e diagramas de sequência
- **ASCII Art**: Diagramas simples em texto

### Tabelas
- Códigos SIP e seus significados
- Comparações de funcionalidades
- Mapeamento de teclas

## Contribuindo com a Documentação

### Diretrizes
1. **Clareza**: Use linguagem simples e direta
2. **Exemplos**: Inclua exemplos práticos sempre que possível
3. **Estrutura**: Use headers hierárquicos (##, ###, ####)
4. **Formatação**: Use Markdown padrão
5. **Atualização**: Mantenha sincronizado com o código

### Template para Novos Documentos

```markdown
# Título do Documento

## Objetivo
Breve descrição do propósito do documento

## Conteúdo Principal
### Seção 1
Conteúdo...

### Seção 2
Conteúdo...

## Exemplos
Casos de uso práticos

## Referências
Links e recursos adicionais
```

## Recursos Adicionais

### Links Externos
- [GitHub do sngrep](https://github.com/irontec/sngrep)
- [Wiki do Projeto](https://github.com/irontec/sngrep/wiki)
- [RFC 3261 - SIP](https://tools.ietf.org/html/rfc3261)

### Ferramentas Relacionadas
- **tcpdump**: Captura de pacotes
- **wireshark**: Análise de protocolos
- **sipgrep**: Grep para SIP
- **homer**: Captura e análise SIP/RTP

## Histórico de Versões

### Versão 2.0 (2025)
- Adicionadas colunas de desconexão
- Documentação técnica completa criada
- Guias de usuário atualizados

### Versão 1.x
- Funcionalidades base do sngrep
- Documentação original

## Licença

Esta documentação está sob a mesma licença do projeto sngrep (GPLv3).

## Contato

Para questões sobre a documentação:
- Abra uma issue no GitHub
- Contribua com pull requests
- Consulte a comunidade no wiki

---

*Última atualização: Novembro de 2024*
