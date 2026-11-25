/**
 * @file jogo_demilhao.h
 * @author João Guilherme dos Anjos (joao.080607@alunos.utfpr.edu.br)
 * @brief Banco de perguntas (souls-like): estruturas, leitura/exportação de
 *        CSV, gravação em arquivo binário e funções de terminal.
 *        Funciona em Windows e Linux.
 * @version 0.6
 * @date 2026-11-24
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
#define TAM_CAMINHO 512 ///< Tamanho máximo de um caminho de arquivo.
#endif

#ifndef NOME_CSV
#define NOME_CSV "perguntas_souls_like.csv" ///< Nome do CSV com as perguntas iniciais.
#endif

#ifndef NOME_BIN
#define NOME_BIN "perguntas.bin" ///< Nome do arquivo binário (gravado ao lado do CSV).
#endif

/**
 * @brief Uma pergunta do jogo (uma linha do CSV).
 */
typedef struct
{
    int id;                   ///< Número da pergunta (único no banco).
    int nivel;                ///< Dificuldade: 1 (Muito Fácil) a 5 (Muito Difícil).
    char jogo[32];            ///< Jogo de origem (Dark Souls, Elden Ring...).
    char enunciado[200];      ///< Texto da pergunta.
    char alternativa[4][128]; ///< As 4 alternativas: A, B, C e D (índices 0 a 3).
    char correta;             ///< Letra da resposta certa: 'A' a 'D'.
} Perguntas;

/**
 * @brief Vetor dinâmico com todas as perguntas.
 *
 * Cresce (e diminui) com realloc, então não existe limite fixo de perguntas.
 */
typedef struct
{
    Perguntas *dados; ///< Vetor de perguntas (NULL = vazio).
    int quantidade;   ///< Quantas perguntas existem de verdade.
    int capacidade;   ///< Quantas perguntas cabem no vetor agora.
} Banco;

/* ---------------------------------------------------------------
 * TERMINAL
 * --------------------------------------------------------------- */

/**
 * @brief Coloca o console em UTF-8 (acentos) no Windows; no Linux não faz nada.
 */
void configurarTerminal(void);

/**
 * @brief Limpa a tela (cls no Windows, clear no Linux).
 */
void limparTela(void);

/**
 * @brief Espera o jogador apertar Enter.
 */
void pausar(void);

/* ---------------------------------------------------------------
 * ARQUIVOS
 * --------------------------------------------------------------- */

/**
 * @brief Verifica se um arquivo existe (consegue ser aberto).
 * @param caminho Caminho do arquivo.
 * @return 1 se existe, 0 se não.
 */
int arquivoExiste(const char *caminho);

/**
 * @brief Monta o caminho do .bin: na mesma pasta do CSV usado.
 * @param caminhoCSV Caminho do CSV.
 * @param saida      Recebe o caminho do .bin.
 * @param tam        Tamanho de @p saida.
 */
void caminhoDoBinario(const char *caminhoCSV, char *saida, size_t tam);

/* ---------------------------------------------------------------
 * PERGUNTAS
 * --------------------------------------------------------------- */

/**
 * @brief Converte "Muito Fácil".."Muito Difícil" em 1..5.
 * @param texto Texto da dificuldade.
 * @return 1 a 5, ou 0 se não for reconhecido.
 */
int nivelDoTexto(const char *texto);

/**
 * @brief Converte 1..5 no texto da dificuldade.
 * @param nivel Nível de 1 a 5.
 * @return Texto do nível, ou "" se for inválido.
 */
const char *textoDoNivel(int nivel);

/**
 * @brief Lê o CSV e acrescenta as perguntas ao banco.
 * @param banco   Banco que recebe as perguntas.
 * @param arquivo Caminho do CSV.
 * @return Quantidade de perguntas no banco, ou -1 se der erro.
 */
int carregarCSV(Banco *banco, const char *arquivo);

/**
 * @brief Exporta as perguntas atuais para um .csv separado por vírgula.
 * @param banco   Banco de perguntas.
 * @param arquivo Caminho do arquivo a criar.
 * @return 0 se deu certo, -1 se não conseguiu criar o arquivo.
 */
int exportarCSV(const Banco *banco, const char *arquivo);

/**
 * @brief Grava o banco inteiro em arquivo binário.
 * @param banco   Banco de perguntas.
 * @param arquivo Caminho do .bin.
 * @return 0 se deu certo, 1 se não conseguiu criar o arquivo.
 */
int salvarBinario(const Banco *banco, const char *arquivo);

/**
 * @brief Lê o banco do arquivo binário.
 * @param banco   Banco que recebe as perguntas.
 * @param arquivo Caminho do .bin.
 * @return Quantidade lida (>0), 0 se o arquivo não existe,
 *         ou -1 se estiver corrompido / faltar memória.
 */
int carregarBinario(Banco *banco, const char *arquivo);

/* ---------------------------------------------------------------
 * INICIALIZAÇÃO
 * --------------------------------------------------------------- */

/**
 * @brief Carrega as perguntas ao abrir o programa.
 *
 * - @p csvInformado != NULL: lê esse CSV (ignora o .bin);
 * - senão procura o CSV nas pastas (ou pede o caminho) e, se existir um
 *   .bin ao lado dele, lê só o .bin.
 *
 * @param banco        Banco que recebe as perguntas.
 * @param csvInformado CSV escolhido pelo usuário, ou NULL.
 * @param caminhoBin   Recebe onde o .bin deve ser gravado ao sair.
 * @param tamBin       Tamanho de @p caminhoBin.
 * @return Quantidade de perguntas, ou -1 se der erro.
 */
int iniciarBanco(Banco *banco, const char *csvInformado, char *caminhoBin, size_t tamBin);

#endif /* JOGO_DEMILHAO_H */
