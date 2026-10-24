# faat-2008

Recriações de trabalhos do curso de **Sistemas para Internet** (tecnólogo) da **FAAT — Faculdades Atibaia** (2008–2011).

O código original se perdeu. Estas são reconstruções feitas em 2026 a partir do escopo de cada trabalho, mantendo as restrições originais: C e Java puros, terminal/desktop, sem bibliotecas externas.

## Projetos

| Pasta | Descrição | Linguagem | Disciplina |
|---|---|---|---|
| [`cdns/`](cdns/) | Servidor DNS + web para descoberta de serviços em rede local | C | Redes |
| [`cchat/`](cchat/) | Chat via sockets TCP (local ou em rede) | C | Redes |
| [`hotelier/`](hotelier/) | Gerenciamento de hotel via terminal (hóspedes, quartos, check-in/out) | C | Linguagem de Programação |
| [`c-spreadsheet/`](c-spreadsheet/) | Planilha no console do Windows, renderizada no buffer de console | C | Linguagem de Programação |
| [`truco/`](truco/) | Truco Paulista em JavaFX, com IA nos adversários | Java | Programação Orientada a Objetos |
| [`pacman/`](pacman/) | Pac-Man em Java2D | Java | Programação Orientada a Objetos |

Cada pasta tem seu próprio README com instruções de compilação.

## Roadmap

`truco/` e `pacman/` são os dois que pretendo evoluir:

- **pacman** — adicionar tema visual customizado (campo de futebol, sprites próprios).
- **truco** — a lógica do Truco Paulista já está implementada; falta a camada visual.

## Histórico

`cdns`, `cchat`, `hotelier`, `truco` (ex-`jtruco`) e `pacman` (ex-`Java-Pac-Man`) eram repositórios separados, trazidos para cá com `git subtree`, preservando os commits.
