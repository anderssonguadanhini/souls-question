#ifndef PERGUNTA_H
#define PERGUNTA_H

#define MUITO_FACIL 1
#define FACIL 2
#define MEDIO 3
#define DIFICIL 4
#define MUITO_DIFICIL 5

typedef struct {
    char enunciado[300];
    char alternativas[4][150];
    char resposta;
    int dificuldade;
} Pergunta;

#endif
