#ifndef FLAMECHAOS_H
#define FLAMECHAOS_H
#define ARQUIVO_DADOS "perguntas.data"
#include "raylib.h"
// --- CORES ANSI PARA O TERMINAL ---
#define COR_RESET "\033[0m"
#define COR_VERDE "\033[1;32m"    // Facil
#define COR_AMARELO "\033[1;33m"  // Medio
#define COR_VERMELHO "\033[1;31m" // Dificil
#define COR_ROXO "\033[1;35m"     // Muito Dificil (Caos!)
#define COR_CIANO "\033[1;36m"    // Destaques / Bordas
#define COR_BRANCO_NEGRITO "\033[1;37m"

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

void carregarPerguntas(Pergunta **lista, int *tam, int *capacidade);
void salvarPerguntas(Pergunta *lista, int tam);
const char *obterNomeDificuldade(Dificuldade d);
void tocarSom(int tipo);
void cadastrarPergunta(Pergunta **lista, int *tam, int *capacidade);
void listarPerguntas(Pergunta *lista, int tam);
void pesquisarPergunta(Pergunta *lista, int tam);
void alterarPergunta(Pergunta *lista, int tam);
void excluirPergunta(Pergunta **lista, int *tam, int *capacidade);

#endif // FLAMECHAOS_H