# cchat

Chat em C puro usando sockets TCP. Funciona na mesma máquina (dois terminais) ou
entre máquinas diferentes na rede.

## Estrutura

```
cchat/
├── include/
│   └── chat.h        # tipos e abstrações de plataforma
├── src/
│   ├── main.c        # parsing de argumentos e setup da conexão (server/client)
│   └── chat.c        # loop de envio/recebimento (threads)
└── Makefile
```

## Dependências

| Plataforma | Compilador | Observação |
|------------|------------|------------|
| Windows    | GCC (MinGW) | `ws2_32.lib` é linkado automaticamente |
| Linux/macOS | GCC / Clang | `pthreads` é linkado automaticamente |

## Compilar

```bash
# com Make (MinGW no Windows ou GCC no Linux/macOS)
make

# ou diretamente com gcc no Windows
gcc -Wall -Iinclude -o cchat.exe src/main.c src/chat.c -lws2_32

# ou no Linux/macOS
gcc -Wall -Iinclude -o cchat src/main.c src/chat.c -lpthread
```

## Uso

```
Servidor:  cchat server <porta> <username>
Cliente:   cchat client <host> <porta> <username>
```

### Mesma máquina (dois terminais)

```bash
# Terminal 1
./cchat server 5000 Alice

# Terminal 2
./cchat client localhost 5000 Bob
```

### Máquinas diferentes na rede

```bash
# Máquina A (IP: 192.168.1.10)
./cchat server 5000 Alice

# Máquina B
./cchat client 192.168.1.10 5000 Bob
```

## Comandos dentro do chat

| Comando | Ação |
|---------|------|
| `/quit` | Desconecta e encerra o programa |
| `Ctrl+C` | Encerra forçadamente |

## Como funciona

```
┌──────────────────────────────────────────────────────────┐
│                    processo cchat                         │
│                                                           │
│  Thread principal          Thread de recebimento          │
│  ─────────────────         ──────────────────────         │
│  fgets(stdin)              recv_line(socket)              │
│  send(socket)              printf(mensagem)               │
│  repete até /quit          repete até desconexão          │
└──────────────────────────────────────────────────────────┘
```

- **Protocolo**: mensagens de texto terminadas por `\n`, formato `NOME: texto\n`.
- **Threads**: uma thread lê do socket e imprime; a thread principal lê do stdin e envia.
- **Encerramento**: `/quit` chama `shutdown()`, o que faz o `recv()` da outra ponta retornar 0 (desconexão limpa).

## Notas

- Se o peer desconectar enquanto você está digitando, será exibida a mensagem
  `[*] Connection closed by peer.` — tecle Enter para sair do programa.
- Mensagens longas (> 4095 bytes) são truncadas.
- Apenas uma conexão por vez (1-para-1).
