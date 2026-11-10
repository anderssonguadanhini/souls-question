
#ifndef FLAMECHAOS_H
#define FLAMECHAOS_H

// Conteúdo do header

// Definição dos níveis de dificuldade
typedef enum
{
    FACIL = 0,
    MEDIO,
    DIFICIL,
    MUITO_DIFICIL
} Dificuldade;

// Estrutura das perguntas
typedef struct
{
    char enunciado[200];
    char alternativas[4][100];
    char correta;
    Dificuldade nivel;
} Pergunta;

// Protótipos das funções
const char *obterNomeDificuldade(Dificuldade d);

void cadastrarPergunta(
    Pergunta **lista,
    int *tam,
    int *capacidade);

void listarPerguntas(Pergunta *lista, int tam);

void pesquisarPergunta(Pergunta *lista, int tam);

void alterarPergunta(Pergunta *lista, int tam);

void excluirPergunta(
    Pergunta **lista,
    int *tam,
    int *capacidade);

#endif // FLAMECHAOS_H
