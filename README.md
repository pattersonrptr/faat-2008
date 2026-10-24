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

Cada pasta tem seu próprio README com instruções de compilação.

## Lembranças

- O professor de Redes era bravo. Os trabalhos de sockets tinham que funcionar entre duas máquinas do laboratório, ao vivo, na frente dele.
- O sistema de hotel e a planilha foram da época em que eu estava aprendendo C: structs, ponteiros, arquivos binários, `malloc` e `free` na mão.

## Também da FAAT

- [**Java-Pac-Man**](https://github.com/pattersonrptr/Java-Pac-Man) — o original era um Pac-Man com tema de futebol: o labirinto era um campo, as paredes eram as linhas de cal e os fantasmas tinham as cores dos quatro grandes de São Paulo. As artes foram feitas por um amigo, no GIMP e no Inkscape. A recriação ainda não chegou lá, mas é o plano.

## Histórico

Os projetos `cdns`, `cchat` e `hotelier` eram repositórios separados e foram trazidos para cá com `git subtree`, preservando os commits.
