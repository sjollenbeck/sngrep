# Instruções de Compilação e Instalação - sngrep com Colunas de Desconexão

## Requisitos do Sistema

### Dependências Obrigatórias
- **GCC** ou **Clang** (compilador C)
- **libpcap-dev** - Biblioteca de captura de pacotes
- **ncurses-dev** - Biblioteca de interface de terminal
- **autotools** (automake, autoconf) ou **CMake**

### Dependências Opcionais
- **OpenSSL-dev** ou **GnuTLS-dev** - Para suporte a TLS/SSL
- **PCRE-dev** - Para expressões regulares avançadas
- **libncursesw5-dev** - Para suporte a Unicode/UTF-8

## Instalação de Dependências

### Debian/Ubuntu
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    automake \
    autoconf \
    libpcap-dev \
    libncurses5-dev \
    libncursesw5-dev \
    libssl-dev \
    libpcre3-dev
```

### Red Hat/CentOS/Fedora
```bash
sudo yum install -y \
    gcc \
    make \
    automake \
    autoconf \
    libpcap-devel \
    ncurses-devel \
    openssl-devel \
    pcre-devel
```

### Arch Linux
```bash
sudo pacman -S \
    base-devel \
    libpcap \
    ncurses \
    openssl \
    pcre
```

### macOS (via Homebrew)
```bash
brew install \
    automake \
    autoconf \
    libpcap \
    ncurses \
    openssl \
    pcre
```

## Compilação

### Método 1: Usando Autotools (Recomendado)

1. **Clone o repositório ou extraia o código fonte**:
```bash
git clone https://github.com/irontec/sngrep.git
cd sngrep
```

2. **Gere os scripts de configuração**:
```bash
./bootstrap.sh
```

3. **Configure o build**:
```bash
# Configuração básica
./configure

# Com suporte a OpenSSL
./configure --with-openssl

# Com suporte a GnuTLS (alternativa ao OpenSSL)
./configure --with-gnutls

# Com suporte a PCRE
./configure --with-pcre

# Instalação em diretório customizado
./configure --prefix=/usr/local

# Todas as opções
./configure --with-openssl --with-pcre --prefix=/usr/local
```

4. **Compile o projeto**:
```bash
make -j$(nproc)
```

5. **Teste a compilação** (opcional):
```bash
make check
```

6. **Instale o programa**:
```bash
sudo make install
```

### Método 2: Usando CMake

1. **Crie diretório de build**:
```bash
mkdir build
cd build
```

2. **Configure com CMake**:
```bash
# Configuração básica
cmake ..

# Com opções específicas
cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DWITH_OPENSSL=ON \
    -DWITH_PCRE=ON
```

3. **Compile**:
```bash
make -j$(nproc)
```

4. **Instale**:
```bash
sudo make install
```

## Verificação da Instalação

### Teste Básico
```bash
# Verificar versão
sngrep -V

# Verificar help
sngrep -h

# Teste de captura (requer permissões root)
sudo sngrep -i lo
```

### Verificar Novas Colunas
```bash
# Iniciar sngrep
sudo sngrep

# Pressionar F10 ou 't'
# Verificar se "Disconnect By" e "Disc Code" aparecem na lista
```

## Compilação para Desenvolvimento

### Habilitar Debug
```bash
./configure CFLAGS="-g -O0 -DDEBUG"
make clean
make
```

### Análise com Valgrind
```bash
./configure --enable-debug
make
valgrind --leak-check=full ./sngrep -r test.pcap
```

### Compilação com Sanitizers
```bash
# Address Sanitizer
./configure CFLAGS="-fsanitize=address -g" LDFLAGS="-fsanitize=address"
make

# Thread Sanitizer
./configure CFLAGS="-fsanitize=thread -g" LDFLAGS="-fsanitize=thread"
make
```

## Criação de Pacotes

### DEB (Debian/Ubuntu)
```bash
# Instalar ferramentas
sudo apt-get install devscripts debhelper

# Criar pacote
dpkg-buildpackage -us -uc -b

# Pacote será criado no diretório pai
ls ../*.deb
```

### RPM (Red Hat/CentOS)
```bash
# Instalar ferramentas
sudo yum install rpm-build

# Criar estrutura RPM
rpmdev-setuptree

# Copiar spec file
cp pkg/rpm/SPECS/sngrep.spec ~/rpmbuild/SPECS/

# Criar tarball
make dist
cp sngrep-*.tar.gz ~/rpmbuild/SOURCES/

# Build RPM
rpmbuild -ba ~/rpmbuild/SPECS/sngrep.spec
```

## Configuração Pós-Instalação

### Arquivo de Configuração Global
```bash
sudo nano /etc/sngreprc
```

Adicione as colunas padrão:
```
# Configuração com novas colunas
set cl.column0 index
set cl.column1 sipfrom
set cl.column2 sipto
set cl.column3 state
set cl.column4 disconnectby
set cl.column5 disconnectcode
set cl.column6 msgcnt
```

### Permissões para Captura
```bash
# Opção 1: Adicionar usuário ao grupo pcap
sudo usermod -a -G pcap $USER

# Opção 2: Configurar capabilities
sudo setcap cap_net_raw,cap_net_admin=eip /usr/local/bin/sngrep
```

## Solução de Problemas

### Erro: "configure: error: libpcap not found"
```bash
# Debian/Ubuntu
sudo apt-get install libpcap-dev

# Red Hat/CentOS
sudo yum install libpcap-devel
```

### Erro: "error: curses.h: No such file or directory"
```bash
# Debian/Ubuntu
sudo apt-get install libncurses5-dev

# Red Hat/CentOS
sudo yum install ncurses-devel
```

### Erro ao executar: "permission denied"
```bash
# Executar com sudo
sudo sngrep

# Ou configurar capabilities
sudo setcap cap_net_raw,cap_net_admin=eip $(which sngrep)
```

### Colunas novas não aparecem
1. Verifique se está usando a versão compilada correta:
```bash
which sngrep
sngrep -V
```

2. Verifique os arquivos modificados:
```bash
grep -l "DISCONNECT" src/*.c src/*.h
```

3. Recompile do zero:
```bash
make clean
./bootstrap.sh
./configure
make
sudo make install
```

## Desinstalação

### Se instalado via make install
```bash
cd /caminho/para/sngrep/source
sudo make uninstall
```

### Remoção manual
```bash
sudo rm -f /usr/local/bin/sngrep
sudo rm -f /usr/local/share/man/man8/sngrep.8
sudo rm -f /etc/sngreprc
```

## Desenvolvimento e Testes

### Compilar e Executar sem Instalar
```bash
make
./sngrep -r tests/aaa.pcap
```

### Executar Testes
```bash
make check
```

### Gerar Documentação Doxygen
```bash
doxygen Doxyfile
# Documentação será gerada em doc/html/
```

## Integração Contínua

### GitHub Actions Workflow
```yaml
name: Build
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v2
    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y automake autoconf libpcap-dev libncurses5-dev
    - name: Build
      run: |
        ./bootstrap.sh
        ./configure
        make
    - name: Test
      run: make check
```

## Benchmarking

### Teste de Performance
```bash
# Capturar 10000 pacotes
time sngrep -c -N -q -l 10000 -i eth0

# Processar arquivo grande
time sngrep -N large_capture.pcap
```

### Profiling
```bash
# Compilar com profiling
./configure CFLAGS="-pg"
make clean && make

# Executar
./sngrep -r test.pcap

# Analisar
gprof sngrep gmon.out > analysis.txt
```

## Referências

- [Autotools Tutorial](https://www.gnu.org/software/automake/manual/)
- [CMake Documentation](https://cmake.org/documentation/)
- [libpcap Documentation](https://www.tcpdump.org/manpages/pcap.3pcap.html)
- [ncurses Programming Guide](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/)

---

*Documento criado para sngrep v2.0 com suporte às colunas de desconexão*
