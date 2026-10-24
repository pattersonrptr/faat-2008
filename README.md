# faat-2008

Trabalhos da minha primeira faculdade, recriados.

Entre 2008 e 2011 cursei **Sistemas para Internet** (tecnólogo) na **FAAT — Faculdades Atibaia**. Eram 2 anos e meio, terminei em 3. Foi lá que aprendi C, redes e boa parte do que sei sobre programar.

O código original daquela época se perdeu. Em 2026, com a ajuda de IA, recriei alguns dos trabalhos a partir do que eu lembrava deles: o que faziam, como funcionavam, o que dava trabalho. Não são cópias fiéis — são reconstruções, no mesmo espírito: C puro, terminal, sem bibliotecas externas.

## Projetos

| Pasta | O que é | Disciplina | Quando |
|---|---|---|---|
| [`cdns/`](cdns/) | DNS + servidor web caseiros para descobrir serviços na rede local | Redes | ~2008–2009 |
| [`cchat/`](cchat/) | Chat via sockets TCP, no mesmo PC ou entre máquinas da rede | Redes | ~2008–2009 |
| [`hotelier/`](hotelier/) | Sistema de gerenciamento de hotel no terminal (hóspedes, quartos, check-in/out) | Linguagem de Programação | 2º sem. 2008 / início 2009 |
| [`c-spreadsheet/`](c-spreadsheet/) | Mini-planilha no console do Windows, com interface retrô | Linguagem de Programação | 2º sem. 2008 / início 2009 |
| [`truco/`](truco/) | Truco Paulista em JavaFX, com IA nos adversários (Java) | Programação Orientada a Objetos | ~2009 |
| [`pacman/`](pacman/) | Pac-Man em Java2D (Java) | Programação Orientada a Objetos | ~2009 |

Cada pasta tem seu próprio README com instruções de compilação.

> **`truco/` e `pacman/` são as duas que eu quero fazer crescer.** São arqueologia *e* projetos vivos: um dia pretendo evoluí-los para ficarem parecidos com os originais da faculdade (ver abaixo). Por enquanto ficam aqui engavetados. Quando for mexer, decido com a IA se extraio para um repo próprio ou evoluo aqui dentro.

## Lembranças

- O professor de Redes era bravo. Os trabalhos de sockets tinham que funcionar entre duas máquinas do laboratório, ao vivo, na frente dele.
- O sistema de hotel e a planilha foram da época em que eu estava aprendendo C: structs, ponteiros, arquivos binários, `malloc` e `free` na mão.
- O **Pac-Man** e o **Truco** foram da disciplina de Programação Orientada a Objetos, onde aprendi Java e OO.

## Os originais de POO (o que eu quero recriar)

As artes dos dois originais foram feitas pelo mesmo amigo, no GIMP e no Inkscape, enquanto eu fazia a programação.

- **Pac-Man** — tema de futebol: o labirinto era um campo, as paredes eram as linhas de cal e os fantasmas tinham as cores dos quatro grandes de São Paulo. A recriação em `pacman/` ainda não tem essa arte, mas é o plano.
- **Truco** — jogo de um jogador contra a máquina, com arte bonitinha. Naquela época ainda não tinha a lógica real do truco: as cartas tinham peso e a maior ganhava da menor. A versão em `truco/` já implementa o Truco Paulista de verdade, mas falta a arte.

Perdi o código dos dois originais (não usava GitHub na época, e eles se foram nas formatações do Windows).

## Histórico

Os projetos `cdns`, `cchat`, `hotelier`, `truco` (ex-`jtruco`) e `pacman` (ex-`Java-Pac-Man`) eram repositórios separados e foram trazidos para cá com `git subtree`, preservando os commits.
