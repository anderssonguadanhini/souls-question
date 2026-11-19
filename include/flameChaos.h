/**
 * @file flameChaos.h
 * @brief Menu e cadastro de perguntas (inserir, listar, pesquisar,
 *        alterar, excluir e exportar), todos sobre o mesmo Banco
 *        definido em jogo_demilhao.h.
 *
 * Este arquivo não inclui windows.h nem raylib.h de propósito, para
 * poder ser incluído em qualquer lugar sem conflito de nomes.
 */

#ifndef FLAMECHAOS_H
#define FLAMECHAOS_H

#include "jogo_demilhao.h"

/* --- Cores ANSI para o terminal --- */
#define COR_RESET "\033[0m"
#define COR_VERDE "\033[1;32m"          /* nível 1 - Muito Fácil */
#define COR_CIANO "\033[1;36m"          /* nível 2 - Fácil       */
#define COR_AMARELO "\033[1;33m"        /* nível 3 - Médio       */
#define COR_VERMELHO "\033[1;31m"       /* nível 4 - Difícil     */
#define COR_ROXO "\033[1;35m"           /* nível 5 - Muito Difícil */
#define COR_BRANCO_NEGRITO "\033[1;37m" /* títulos */

/** Menor e maior nível de dificuldade (1 = Muito Fácil ... 5 = Muito Difícil). */
#define NIVEL_MIN 1
#define NIVEL_MAX 5

/**
 * @brief Toca um som curto (beep) no Windows; no Linux, o sino do terminal.
 * @param tipo 1 = navegar, 2 = sucesso, 3 = erro, 4 = nível Muito Difícil.
 */
void tocarSom(int tipo);

/**
 * @brief Devolve a cor ANSI associada a um nível de dificuldade.
 * @param nivel Nível de 1 a 5.
 * @return String com o código da cor (COR_RESET se o nível for inválido).
 */
const char *obterCorNivel(int nivel);

/**
 * @brief Lê um número inteiro digitado pelo usuário (uma linha inteira).
 * @param mensagem Texto mostrado antes da leitura.
 * @param valor    Recebe o número lido.
 * @return 1 se leu um número válido, 0 se a entrada não era um número,
 *         -1 se a entrada foi fechada (EOF).
 */
int lerInteiro(const char *mensagem, int *valor);

/**
 * @brief Cadastra uma nova pergunta no banco (o vetor cresce se precisar).
 * @param banco Banco de perguntas.
 */
void cadastrarPergunta(Banco *banco);

/**
 * @brief Lista todas as perguntas e mostra o resumo por nível.
 * @param banco Banco de perguntas.
 */
void listarPerguntas(const Banco *banco);

/**
 * @brief Pesquisa por nível e permite navegar entre as perguntas
 *        (anterior, próxima, revelar resposta, sair).
 * @param banco Banco de perguntas.
 */
void pesquisarPergunta(const Banco *banco);

/**
 * @brief Altera uma pergunta escolhida pelo número (ID mostrado na lista).
 * @param banco Banco de perguntas.
 */
void alterarPergunta(Banco *banco);

/**
 * @brief Exclui uma pergunta (o vetor diminui se ficar muito vazio).
 * @param banco Banco de perguntas.
 */
void excluirPergunta(Banco *banco);

/**
 * @brief Pede um nome de arquivo e exporta o banco em .csv.
 * @param banco Banco de perguntas.
 */
void exportarPerguntas(const Banco *banco);

#endif /* FLAMECHAOS_H */