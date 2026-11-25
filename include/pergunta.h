/**
 * @file pergunta.h
 * @brief Estrutura de pergunta do protótipo antigo (usada só pela pastaTeste).
 *
 * @warning O programa principal usa a struct Perguntas de jogo_demilhao.h.
 *          Não inclua este arquivo junto com sistema_cardGame.h: os
 *          #define abaixo têm os mesmos nomes do enum Dificuldade.
 */

#ifndef PERGUNTA_H
#define PERGUNTA_H

#define MUITO_FACIL 1   ///< Nível 1.
#define FACIL 2         ///< Nível 2.
#define MEDIO 3         ///< Nível 3.
#define DIFICIL 4       ///< Nível 4.
#define MUITO_DIFICIL 5 ///< Nível 5.

/** @brief Uma pergunta do protótipo (formato antigo do CSV). */
typedef struct {
    char enunciado[300];       ///< Texto da pergunta.
    char alternativas[4][150]; ///< Alternativas A, B, C e D.
    char resposta;             ///< Letra da resposta certa.
    int dificuldade;           ///< Nível de 1 a 5.
} Pergunta;

#endif /* PERGUNTA_H */
