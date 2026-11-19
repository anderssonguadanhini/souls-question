/**
 * @file jogo_demilhao.h
 * @author João Guilherme dos Anjos (joao.080607@alunos.utfpr.edu.br)
 * @brief Interface do jogo de perguntas (souls-like): banco de perguntas,
 *        leitura/exportação de CSV e gravação em arquivo binário.
 *        Funciona em Windows e Linux.
 * @version 0.5
 * @date 2026-11-18
 *
 * @copyright Copyright (c) 2026
 *
 * Este .h não inclui windows.h de propósito: assim ele pode ser incluído
 * junto com raylib.h sem causar conflito de nomes. O windows.h fica só
 * dentro do jogo_demilhao.c.
 */

#ifndef JOGO_DEMILHAO_H
#define JOGO_DEMILHAO_H

#include <stddef.h> /* size_t */

/* ---------------------------------------------------------------
 * CONSTANTES
 * (com #ifndef: se outro arquivo já definir o mesmo nome, não dá aviso)
 * --------------------------------------------------------------- */
#ifndef TAM_CAMINHO
#define TAM_CAMINHO 512 /* tamanho máximo de um caminho de arquivo */
#endif

#ifndef NOME_CSV
#define NOME_CSV "perguntas_souls_like.csv" /* nome do CSV da equipe */
#endif

#ifndef NOME_BIN
#define NOME_BIN "perguntas.bin" /* nome do arquivo binário */
#endif

/* ---------------------------------------------------------------
 * STRUCT Perguntas
 * Guarda UMA pergunta do jogo (uma linha do CSV).
 * --------------------------------------------------------------- */
typedef struct
{
    int id;                   /* número da pergunta no CSV */
    int nivel;                /* dificuldade em número: 1 a 5 */
    char jogo[32];            /* jogo de origem (Dark Souls, Elden Ring...) */
    char enunciado[200];      /* texto da pergunta */
    char alternativa[4][128]; /* as 4 alternativas: A, B, C e D (índices 0 a 3) */
    char correta;             /* letra da resposta certa: 'A' a 'D' */
} Perguntas;

/* ---------------------------------------------------------------
 * STRUCT Banco
 * É o vetor dinâmico que guarda todas as perguntas.
 * Cresce com realloc, então não existe limite fixo de perguntas.
 * --------------------------------------------------------------- */
typedef struct
{
    Perguntas *dados; /* ponteiro para o vetor de perguntas (NULL = vazio) */
    int quantidade;   /* quantas perguntas existem de verdade */
    int capacidade;   /* quantas perguntas cabem no vetor agora */
} Banco;

/* ---------------------------------------------------------------
 * TERMINAL
 * --------------------------------------------------------------- */

/* No Windows coloca o console em UTF-8 (acentos); no Linux não faz nada. */
void configurarTerminal(void);

/* Limpa a tela (cls no Windows, clear no Linux). */
void limparTela(void);

/* Espera o jogador apertar Enter. */
void pausar(void);

/* ---------------------------------------------------------------
 * ARQUIVOS
 * --------------------------------------------------------------- */

/* Retorna 1 se o arquivo existe (consegue abrir), ou 0 se não. */
int arquivoExiste(const char *caminho);

/* Escreve em "saida" o caminho do .bin: na mesma pasta do CSV usado. */
void caminhoDoBinario(const char *caminhoCSV, char *saida, size_t tam);

/* ---------------------------------------------------------------
 * PERGUNTAS
 * --------------------------------------------------------------- */

/* Converte "Muito Fácil".."Muito Difícil" em 1..5 (0 = não reconhecido). */
int nivelDoTexto(const char *texto);

/* Faz o inverso: converte 1..5 no texto da dificuldade. */
const char *textoDoNivel(int nivel);

/* Lê o CSV e guarda as perguntas no banco.
 * Retorna a quantidade guardada, ou -1 se der erro. */
int carregarCSV(Banco *banco, const char *arquivo);

/* Exporta as perguntas atuais para um .csv separado por vírgula.
 * Retorna 0 se deu certo, ou -1 se não conseguiu criar o arquivo. */
int exportarCSV(const Banco *banco, const char *arquivo);

/* Grava o banco inteiro em arquivo binário.
 * Retorna 0 se deu certo, ou 1 se não conseguiu criar o arquivo. */
int salvarBinario(const Banco *banco, const char *arquivo);

/* Lê o banco do arquivo binário.
 * Retorna a quantidade lida (>0), 0 se o arquivo não existe,
 * ou -1 se estiver corrompido / faltar memória. */
int carregarBinario(Banco *banco, const char *arquivo);

/* ---------------------------------------------------------------
 * INICIALIZAÇÃO
 * --------------------------------------------------------------- */

/* Carrega as perguntas ao abrir o programa:
 *  - csvInformado != NULL: lê esse CSV (ignora o .bin);
 *  - senão, se existe .bin: lê só ele;
 *  - senão: procura o CSV nas pastas; se não achar, pede o caminho.
 * Em "caminhoBin" devolve onde o .bin deve ser gravado ao sair.
 * Retorna a quantidade de perguntas, ou -1 se der erro. */
int iniciarBanco(Banco *banco, const char *csvInformado, char *caminhoBin, size_t tamBin);

#endif /* JOGO_DEMILHAO_H */