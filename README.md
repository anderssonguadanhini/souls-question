# Flame Chaos — Jogo do Milhão Souls-like

Jogo de perguntas e respostas no estilo **Show do Milhão**, com o tema dos jogos
*souls-like* (Dark Souls, Elden Ring, Bloodborne, Sekiro...). Feito em **C (C99)**,
com menu colorido no terminal e uma **arena gráfica em [raylib](https://www.raylib.com/)**.
Funciona no **Windows** e no **Linux**.

Projeto prático da disciplina **Fundamentos de Programação 2 (FDCO2B)** — UTFPR,
Campus Apucarana, Engenharia de Computação, 2º semestre de 2026.

## Equipe

- João Guilherme dos Anjos
- Andersson
- Gabriel

---

## Funcionalidades

### Parte básica
- **Cadastro de perguntas** (inserir, listar, pesquisar, alterar e excluir) em um
  **vetor dinâmico** que cresce e diminui com `realloc`.
- Cada pergunta tem enunciado, 4 alternativas, letra correta, nível de dificuldade
  (1 a 5) e o jogo de origem.
- **Pesquisa por nível** navegando entre as perguntas (anterior / próxima /
  revelar resposta), com cores por nível e sons.
- **Arquivos**:
  - 1ª execução: lê as perguntas do CSV `perguntas_souls_like.csv`;
  - ao sair: salva tudo em `perguntas.bin` (ao lado do CSV);
  - próximas execuções: lê e salva só o `perguntas.bin`;
  - **exportação** dos dados atuais para um novo `.csv`.
- Tratamento de erros de digitação com mensagens, cores ANSI e *beeps*.

### Parte criativa
- **Partida no terminal** com as regras do Show do Milhão: 15 perguntas, sem
  repetir, com **marcos de segurança** nas perguntas 5 e 10 e opção de **parar**.
- **Arena gráfica (raylib)**, controlada pelo mouse:
  - o herói começa com **poder 5** e precisa derrotar Hollow Faca, Hollow Tocha,
    **Sif** e o chefe final **Gwyn**;
  - **baús** somam poder e a **Espada do Abismo** multiplica;
  - vencer um combate (poder do herói ≥ poder do inimigo) **absorve** o poder do
    inimigo e abre a **escolha de cartas**;
  - o herói **evolui visualmente** em 4 estágios (até a *Forma Suprema*).
- **Sistema de cartas**: 5 cartas de costas, uma por dificuldade, cada uma
  homenageando um chefe de Elden Ring (Margit, Rennala, Radahn, Radagon e
  Malenia). A carta sorteia uma pergunta do nível; **acertar multiplica** o poder
  e **errar divide**:

  | Carta | Nível         | Acerto | Erro |
  |:-----:|---------------|:------:|:----:|
  | I     | Muito Fácil   | x2     | ÷2   |
  | II    | Fácil         | x3     | ÷4   |
  | III   | Médio         | x4     | ÷6   |
  | IV    | Difícil       | x6     | ÷8   |
  | V     | Muito Difícil | x10    | ÷10  |

- Os poderes dos inimigos são sorteados a cada arena (até 9.999.999), mas sempre
  existe uma ordem que permite vencer.

---

## Como compilar e executar

O `Makefile` detecta o sistema operacional sozinho.

| Comando      | O que faz                                    |
|--------------|----------------------------------------------|
| `make`       | Compila o programa (`flameChaos` / `flameChaos.exe`) |
| `make run`   | Compila (se precisar) e executa              |
| `make clean` | Apaga o executável                           |
| `make docs`  | Gera a documentação Doxygen em `docs/html`   |

### Windows
Requer o **w64devkit** (gcc 64 bits) instalado em `C:\raylib\w64devkit` — é o
que vem com o instalador do raylib.

```powershell
mingw32-make
mingw32-make run
```

> Se houver um MinGW antigo de 32 bits no `PATH` (ex.: `C:\MinGW`), o Makefile
> usa automaticamente o gcc do w64devkit, porque a `libraylib.a` é de 64 bits.

### Linux
```bash
sudo apt install build-essential libgl1-mesa-dev libx11-dev
make
make run
```

Também é possível escolher outro CSV ao executar:
```bash
./flameChaos caminho/do/arquivo.csv
```

---

## Como jogar

### Menu principal
```
1. Jogar Modo Gráfico (Torre / Mouse)
2. Jogar Modo Terminal (15 Níveis)
3. Gerenciar Perguntas (CRUD / CSV)
4. Regras e Marcos de Segurança
0. Sair e Salvar
```

### Modo terminal (Show do Milhão)

| Perguntas | Nível         | Prêmios                              |
|-----------|---------------|--------------------------------------|
| 1 – 2     | Muito Fácil   | R$ 1.000 / 2.000                     |
| 3 – 4     | Fácil         | R$ 3.000 / 4.000                     |
| 5 – 8     | Médio         | R$ 5.000 (**1º marco**) até 30.000   |
| 9 – 12    | Difícil       | R$ 40.000, 50.000 (**2º marco**) até 200.000 |
| 13 – 15   | Muito Difícil | R$ 300.000 / 500.000 / **1.000.000** |

Responda com **A, B, C ou D**, ou **P** para parar e levar o prêmio acumulado.
Errando, o jogador leva o valor do último marco alcançado.

### Modo gráfico

| Controle                  | Ação                                         |
|---------------------------|----------------------------------------------|
| Clique no chão            | Andar                                        |
| Clique em item / inimigo  | Coletar / lutar                              |
| Clique na carta ou `1`–`5`| Escolher a carta                             |
| Clique na resposta ou `A`–`D` | Responder                                |
| `Enter` / `Espaço`        | Fechar o resultado da carta                  |
| `F11`                     | Alternar tela cheia / janela                 |
| `ESC`                     | Sair da tela cheia (ou voltar ao menu)       |

---

## Formato do CSV

Separado por vírgula, em UTF-8, com cabeçalho na primeira linha:

```csv
id,dificuldade,jogo,pergunta,alternativa_a,alternativa_b,alternativa_c,alternativa_d,resposta_correta
1,Muito Fácil,Dark Souls,Qual estúdio desenvolveu Dark Souls?,Bandai Namco,Capcom,FromSoftware,Square Enix,C
```

- `dificuldade`: `Muito Fácil`, `Fácil`, `Médio`, `Difícil` ou `Muito Difícil`;
- `resposta_correta`: `A`, `B`, `C` ou `D`;
- os textos não podem conter vírgula (o cadastro impede isso).

O arquivo da equipe tem **80 perguntas**: 20 muito fáceis, 20 fáceis, 15 médias,
15 difíceis e 10 muito difíceis.

---

## Estrutura do projeto

```
souls-question/
├── Makefile                   # make, run, clean, docs
├── Doxyfile                   # configuração do Doxygen
├── perguntas_souls_like.csv   # perguntas iniciais
├── include/
│   ├── jogo_demilhao.h        # structs Perguntas e Banco, CSV e binário
│   ├── flameChaos.h           # cadastro (CRUD), cores e sons do terminal
│   ├── sistema_cardGame.h     # struct CardGame, multiplicadores e redutores
│   ├── interface.h            # interface gráfica (raylib)
│   └── pergunta.h             # struct do protótipo antigo (pastaTeste)
├── src/
│   ├── main.c                 # menu principal e partida no terminal
│   ├── jogo_demilhao.c        # leitura/gravação de CSV e binário, busca do CSV
│   ├── flameChaos.c           # cadastro, pesquisa por nível, exportação
│   ├── sistema_cardGame.c     # sistema de cartas
│   └── interface.c            # arena, combate, evolução do herói e cartas
├── assets/                    # sprites do herói, inimigos, cenário e cartas
├── helloRaylib/               # raylib (headers e libs para Windows e Linux)
└── pastaTeste/                # protótipo antigo (não é compilado)
```

---

## Documentação

Todo o código está comentado no padrão **Doxygen**. Para gerar o HTML:

```bash
make docs
```

e abra `docs/html/index.html`.
