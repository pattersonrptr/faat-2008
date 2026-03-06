# Hotel Management System

Sistema de gerenciamento hoteleiro completo, desenvolvido em **C puro**, operado inteiramente via terminal.

> **Nota:** Este projeto foi um exercício de aprendizagem — um experimento pessoal para revisitar a linguagem C, reacostumar com gerenciamento manual de memória, structs, I/O de arquivos binários e organização de código sem o conforto de frameworks modernos. Nada de C++, nada de bibliotecas externas.

---

## O que o sistema faz

Cobre o ciclo de vida completo de uma hospedagem, do cadastro ao checkout:

| Módulo | Descrição |
|---|---|
| **Quartos** | Cadastro, status (livre / ocupado / manutenção / bloqueado), histórico de manutenções, mapa de ocupação |
| **Hóspedes** | Cadastro com CPF validado ou passaporte, programa de fidelidade (Bronze → Diamante), blacklist |
| **Reservas** | Criação com conflito de datas, múltiplos quartos, código de confirmação único, cancelamento e no-show |
| **Check-in / Check-out** | Early check-in, caução, acompanhantes extras, late check-out, extrato final completo |
| **Serviços** | Frigobar, room service, lavanderia, estacionamento, outros consumos — todos vinculados à reserva |
| **Financeiro** | Faturamento, caixa do dia, receita por período, contas a receber, caixa por turno (manhã/tarde/noite) |
| **Governança** | Lista de limpeza por prioridade, status por quarto, objetos esquecidos |
| **Funcionários** | CRUD com cargos, turnos e níveis de acesso (1 = básico ... 5 = admin) |
| **Relatórios** | Taxa de ocupação, RevPAR/ADR, ranking de hóspedes, previsão 30 dias, no-shows, exportação CSV |
| **Configurações** | Dados do hotel, taxas, programa de fidelidade, senha, backup dos dados |

---

## Tecnologias

- **Linguagem:** C99
- **Compilador:** GCC (`-std=c99 -Wall -Wextra -pedantic`)
- **Interface:** Console puro — sem ncurses, sem bibliotecas gráficas
- **Persistência:** Arquivos binários `.dat` (leitura total na inicialização, escrita em cada operação crítica)
- **Dependências externas:** nenhuma — apenas `libc` padrão (`stdio`, `stdlib`, `string`, `time`, `ctype`)
- **Build:** `make` (Makefile minimalista)
- **Testes:** PowerShell (smoke test end-to-end com entradas via pipe)

---

## Estrutura do projeto

```
hotel-management-system/
├── src/
│   ├── main.c
│   ├── types.h              ← todos os enums e structs
│   ├── auth.c / .h          ← login, sessão, hash djb2
│   ├── menu.c / .h          ← navegação por menus com function pointers
│   ├── storage.c / .h       ← I/O binário, backup, audit log
│   ├── utils.c / .h         ← input seguro, datas, CPF, strings
│   ├── rooms.c / .h
│   ├── guests.c / .h
│   ├── reservations.c / .h
│   ├── checkin.c / .h
│   ├── services.c / .h
│   ├── finance.c / .h
│   ├── staff.c / .h
│   ├── housekeeping.c / .h
│   ├── reports.c / .h
│   └── config.c / .h
├── tests/
│   └── smoke_test.ps1       ← teste funcional automatizado
├── data/                    ← gerado em runtime (gitignored)
├── reports/                 ← CSVs exportados (gitignored)
├── backups/                 ← backups manuais (gitignored)
└── Makefile
```

---

## Como rodar

### Pré-requisitos

- GCC disponível no PATH (Linux/macOS nativo; Windows via [Strawberry Perl](https://strawberryperl.com/) ou [MSYS2](https://www.msys2.org/))
- `make` (incluído com GCC no Linux/macOS; no Windows use `mingw32-make`)

### Compilar e executar

```bash
# Linux / macOS
make run

# Windows (PowerShell ou cmd)
mingw32-make run
```

O executável gerado é `hotel` (Linux/macOS) ou `hotel.exe` (Windows). A pasta `data/` é criada automaticamente pelo Makefile na primeira compilação.

### Login padrão

```
Usuário: admin
Senha:   admin123
```

> Altere a senha logo após o primeiro acesso via **Configurações → Alterar minha senha**.

### Limpar a build

```bash
make clean        # Linux/macOS
mingw32-make clean  # Windows
```

---

## Teste automatizado

O script `tests/smoke_test.ps1` executa um fluxo completo de ponta a ponta (login → quarto → hóspede → reserva → check-in → serviço → checkout) e valida 9 assertivas na saída.

```powershell
# A partir da raiz do projeto
.\tests\smoke_test.ps1
```

O script limpa a pasta `data/` antes de rodar para garantir um ambiente limpo. A saída esperada é:

```
[PASS] Login admin bem-sucedido
[PASS] Quarto 101 cadastrado
[PASS] Hospede Joao Silva cadastrado
[PASS] Reserva criada (R$300,00)
[PASS] Check-in realizado
[PASS] Servico 'Cafe da manha' R$25,00 registrado
[PASS] Total checkout: R$325,00
[PASS] Mensagem de despedida correta
[PASS] Sistema encerrado normalmente

RESULTADO: PASSOU - todos os testes OK
```

---

## Decisões de design

- **Arrays estáticos de tamanho fixo** (`MAX_ROOMS 200`, `MAX_GUESTS 5000`, etc.) em vez de listas ligadas ou alocação dinâmica — simplicidade intencional.
- **Menus via `function pointers`** em arrays de `MenuItem`, tornando a adição de módulos o mais mecânica possível.
- **Hash djb2** para senhas — suficiente para um exercício; em produção usaria bcrypt ou Argon2.
- **Sem `bool` do C++** — flags são `int`, como manda o C99 sem `<stdbool.h>` quando não necessário.
- **Audit log** em texto plano (`data/audit.log`) registra toda operação com timestamp e `staff_id`.
- **Backup manual** copia todos os `.dat` para `backups/YYYY-MM-DD_HHMMSS/` via `_mkdir` + loop `fread/fwrite`, sem dependências externas.

---

## Licença

Uso livre. Projeto pessoal de aprendizagem — sem garantias de qualquer natureza.
