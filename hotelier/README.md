# hotelier

A hotel management system in plain C, run entirely from the terminal: rooms, guests, reservations, check-in and check-out, services, finances, housekeeping, staff and reports.

Rebuild of a Programming Language course assignment at FAAT. The interface is in Portuguese.

## Features

It covers the whole life cycle of a stay, from registration to checkout:

| Module | Description |
|---|---|
| **Rooms** | Registration, status (free / occupied / maintenance / blocked), maintenance history, occupancy map |
| **Guests** | Registration with validated CPF or passport, loyalty program (Bronze → Diamond), blacklist |
| **Reservations** | Date conflict checks, multiple rooms, unique confirmation code, cancellation and no-show |
| **Check-in / check-out** | Early check-in, deposit, extra companions, late check-out, full final statement |
| **Services** | Minibar, room service, laundry, parking and other charges, all tied to the reservation |
| **Finance** | Billing, daily cash, revenue by period, accounts receivable, cash by shift (morning/afternoon/night) |
| **Housekeeping** | Cleaning list by priority, status per room, lost and found |
| **Staff** | CRUD with roles, shifts and access levels (1 = basic ... 5 = admin) |
| **Reports** | Occupancy rate, RevPAR/ADR, guest ranking, 30-day forecast, no-shows, CSV export |
| **Settings** | Hotel data, rates, loyalty program, password, data backup |

## Technologies

- **Language:** C99
- **Compiler:** GCC (`-std=c99 -Wall -Wextra -pedantic`)
- **Interface:** plain console, no ncurses or graphics libraries
- **Persistence:** binary `.dat` files, fully loaded at startup and written on every critical operation
- **External dependencies:** none, only the standard C library (`stdio`, `stdlib`, `string`, `time`, `ctype`) plus `direct.h` for creating backup folders
- **Build:** `make` (minimal Makefile)
- **Tests:** PowerShell (end-to-end smoke test with input piped in)

## Project structure

```
hotelier/
├── src/
│   ├── main.c
│   ├── types.h              ← all enums and structs
│   ├── auth.c / .h          ← login, session, djb2 hash
│   ├── menu.c / .h          ← menu navigation with function pointers
│   ├── storage.c / .h       ← binary I/O, backup, audit log
│   ├── utils.c / .h         ← safe input, dates, CPF, strings
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
│   └── smoke_test.ps1       ← automated functional test
├── data/                    ← created at runtime (gitignored)
├── reports/                 ← exported CSVs (gitignored)
├── backups/                 ← manual backups (gitignored)
└── Makefile
```

## Running

### Requirements

Windows, with GCC and `mingw32-make` on the `PATH` (for example from [MSYS2](https://www.msys2.org/) or [Strawberry Perl](https://strawberryperl.com/)). The backup uses `_mkdir` from `<direct.h>` and the Makefile uses `cmd` syntax, so the project does not build on Linux or macOS as it is.

### Build and run

```powershell
mingw32-make run
```

This produces `hotel.exe`. The Makefile creates the `data/` folder on the first build.

### Default login

```
User:     admin
Password: admin123
```

> Change the password after the first login, under **Configurações → Alterar minha senha**.

### Clean the build

```powershell
mingw32-make clean
```

## Automated test

`tests/smoke_test.ps1` runs a full flow from end to end (login → room → guest → reservation → check-in → service → checkout) and checks 9 assertions on the output.

```powershell
# from the project root
.\tests\smoke_test.ps1
```

The script clears the `data/` folder first so it always starts from a clean state. Expected output:

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

## Design decisions

- **Fixed-size static arrays** (`MAX_ROOMS 200`, `MAX_GUESTS 5000`, etc.) instead of linked lists or dynamic allocation, for simplicity.
- **Menus built from function pointers** in `MenuItem` arrays, so adding a module is mostly mechanical.
- **djb2 hash for passwords.** Enough for coursework. A real system would use bcrypt or Argon2.
- **Audit log** in plain text (`data/audit.log`) records every operation with a timestamp and `staff_id`.
- **Manual backup** copies every `.dat` file to `backups/YYYY-MM-DD_HHMMSS/` with `_mkdir` and a `fread`/`fwrite` loop, with no external dependencies.

## License

[MIT](LICENSE).
