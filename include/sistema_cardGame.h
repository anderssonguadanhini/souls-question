/**
 * @file sistema_cardGame.h
 * @author João Guilherme dos Anjos (joao.080607@alunos.utfpr.edu.br)
 * @brief Sistema de cartas: transforma as perguntas do banco em cartas com
 *        multiplicador (acerto) e redutor (erro) conforme a dificuldade.
 * @version 0.4
 * @date 2026-11-24
 *
 * @copyright Copyright (c) 2026
 */

#ifndef SISTEMA_CARDGAME_H
#define SISTEMA_CARDGAME_H
#include "jogo_demilhao.h"

/** @brief Níveis de dificuldade (mesmos valores do campo Perguntas::nivel). */
enum Dificuldade
{
    MUITO_FACIL = 1,  ///< Perguntas 1 e 2 da partida.
    FACIL = 2,        ///< Perguntas 3 e 4.
    MEDIO = 3,        ///< Perguntas 5 a 8.
    DIFICIL = 4,      ///< Perguntas 9 a 12.
    MUITO_DIFICIL = 5 ///< Perguntas 13 a 15.
};

/** @brief Quanto o poder é multiplicado ao ACERTAR uma carta de cada nível. */
enum Multiplicador
{
    MULTIPLICADOR_X2 = 2,  ///< Muito Fácil: x2.
    MULTIPLICADOR_X3 = 3,  ///< Fácil: x3.
    MULTIPLICADOR_X4 = 4,  ///< Médio: x4.
    MULTIPLICADOR_X5 = 6,  ///< Difícil: x6.
    MULTIPLICADOR_X6 = 10, ///< Muito Difícil: x10.
};

/** @brief Por quanto o poder é dividido ao ERRAR uma carta de cada nível. */
enum Redutor
{
    REDUTOR_X2 = 2,  ///< Muito Fácil: /2.
    REDUTOR_X3 = 4,  ///< Fácil: /4.
    REDUTOR_X4 = 6,  ///< Médio: /6.
    REDUTOR_X5 = 8,  ///< Difícil: /8.
    REDUTOR_X6 = 10, ///< Muito Difícil: /10.
};

/**
 * @brief Uma carta do jogo: uma pergunta mais o prêmio e a penalidade.
 */
typedef struct
{
    int nivel;                    ///< Nível da pergunta (1 a 5).
    int numMultiplicador;         ///< Multiplicador do poder no acerto.
    int numRedutor;               ///< Divisor do poder no erro.
    char enunciado[200];          ///< Texto da pergunta.
    char alternativa[4][128];     ///< Alternativas A, B, C e D.
    char correta;                 ///< Letra da resposta certa ('A' a 'D').
    enum Dificuldade dificuldade; ///< Dificuldade (mesmo valor de @c nivel).
} CardGame;

/**
 * @brief Converte as perguntas do banco em cartas.
 * @param[out] cards   Vetor com pelo menos @p tamanho posições.
 * @param[in]  tamanho Quantidade de perguntas a converter.
 * @param[in]  banco   Banco de perguntas.
 */
void inicializarCards(CardGame *cards, int tamanho, Banco *banco);

/**
 * @brief Copia para @p vetorResultado as cartas de uma dificuldade.
 * @param[in]  cards          Todas as cartas.
 * @param[in]  tamanho        Quantidade de cartas em @p cards.
 * @param[in]  dificuldade    Dificuldade procurada.
 * @param[out] vetorResultado Recebe as cartas encontradas (espaço para @p tamanho).
 * @return Quantidade de cartas encontradas.
 */
int ProcurarDificuldadeCards(CardGame *cards, int tamanho, enum Dificuldade dificuldade, CardGame *vetorResultado);

/**
 * @brief Sorteia uma carta de um vetor.
 * @param[in]  vetorResultado   Cartas candidatas.
 * @param[out] CardEscolhido    Recebe uma cópia da carta sorteada.
 * @param[in]  tamanhoResultado Quantidade de cartas candidatas.
 */
void EscolherCard(CardGame *vetorResultado, CardGame *CardEscolhido, int tamanhoResultado);

/**
 * @brief Mostra no terminal se a resposta está correta.
 * @param resposta      Letra escolhida pelo jogador.
 * @param CardEscolhido Carta respondida.
 */
void ConfereResposta(char resposta, CardGame *CardEscolhido);

/**
 * @brief Mostra no terminal o multiplicador (acerto) ou o redutor (erro).
 * @param resposta      Letra escolhida pelo jogador.
 * @param CardEscolhido Carta respondida.
 */
void ConsequenciaEscolha(char resposta, CardGame *CardEscolhido);

/**
 * @brief Libera um vetor de cartas alocado com malloc.
 * @param cards Vetor a liberar (NULL é aceito).
 */
void liberarCards(CardGame *cards);

/**
 * @brief Rodada de teste do sistema de cartas no terminal.
 * @param banco Banco de perguntas.
 */
void Teste(Banco *banco);

#endif /* SISTEMA_CARDGAME_H */
