/**
 * @file sistema_cardGame.c
 * @author Guilherme
 * @brief 
 * @version 0.1
 * @date 2026-11-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */
 #include <time.h>  
 #include "jogo_demilhao.h"
 #include<stdio.h>
 #include<stdlib.h>
 #include<string.h>

 enum Dificuldade
 {
    
    MUITO_FACIL = 1,
    FACIL = 2,
    MEDIO = 3,
    DIFICIL = 4,
    MUITO_DIFICIL = 5,
 }

 enum Multiplicador
 {
    MULTIPLICADOR_X2 = 2,
    MULTIPLICADOR_X3 = 3,
    MULTIPLICADOR_X4 = 4,
    MULTIPLICADOR_X5 = 6,
    MULTIPLICADOR_X6 = 10,
 };

 enum Redutor
 {
    REDUTOR_X2 = 0.9,
    REDUTOR_X3 = 0.8,
    REDUTOR_X4 = 0.5,
    REDUTOR_X5 = 0.3,
    REDUTOR_X6 = 0.1,
 };

 typedef struct
{
    int nivel;
    int numMultiplicador;
    int numRedutor;
    char enunciado[200];      
    char alternativa[4][128]; 
    char correta;
    enum Dificuldade dificuldade;  
} CardGame;


int main()
{
    int tamanho = 5;
    CardGame *cards = calloc(tamanho, sizeof(CardGame));
    if(cards == NULL)
    {
        perror("Erro ao alocar memória para os cards");
        return 1;
    }

    free(cards);
}

int sortear(int min, int max)
{
    return rand() % (max - min + 1) + min;
}

void inicializarCards(CardGame *cards, int tamanho, Banco *banco){
    if(cards == NULL || banco == NULL)
    {
        perror("Ponteiro nulo passado para inicializarCards");
        return;
    }

    for(int i = 0; i < tamanho; i++)
    {
        cards[i].nivel = banco->dados[i].nivel;
        cards[i].dificuldade = (enum Dificuldade)banco->dados[i].nivel;
        switch (cards[i].dificuldade)
        {
        case MUITO_FACIL:
            cards[i].numMultiplicador = MULTIPLICADOR_X2;
            cards[i].numRedutor = REDUTOR_X2;
            break;
        case FACIL:
            cards[i].numMultiplicador = MULTIPLICADOR_X3;
            cards[i].numRedutor = REDUTOR_X3;
            break;
        case MEDIO:
            cards[i].numMultiplicador = MULTIPLICADOR_X4;
            cards[i].numRedutor = REDUTOR_X4;
            break;
        case DIFICIL:
            cards[i].numMultiplicador = MULTIPLICADOR_X5;
            cards[i].numRedutor = REDUTOR_X5;
            break;
        case MUITO_DIFICIL:
            cards[i].numMultiplicador = MULTIPLICADOR_X6;
            cards[i].numRedutor = REDUTOR_X6;
            break;
        }

        strcpy(cards[i].enunciado, banco->dados[i].enunciado);
        for(int j = 0; j < 4; j++)
        {
            strcpy(cards[i].alternativa[j], banco->dados[i].alternativa[j]);
        }
        cards[i].correta = banco->dados[i].correta;
    }
}

int ProcurarDificuldadeCards(CardGame *cards, int tamanho, enum Dificuldade dificuldade,CardGame *vetorResultado)
{
    int tamanhoResultado = 0;
    for(int i = 0; i < tamanho; i++)
    {
        if(cards[i].dificuldade == dificuldade)
        {
            vetorResultado[tamanhoResultado] = cards[i];
            tamanhoResultado++;
        }
    }
    return tamanhoResultado;
}

void EscolherCard(CardGame *vetorResultado,CardGame *CardEscolhido, int tamanhoResultado)
{
    if(vetorResultado != NULL && CardEscolhido != NULL && tamanhoResultado > 0)
    {
        int indiceSorteado = sortear(0, tamanhoResultado - 1);
        *CardEscolhido = vetorResultado[indiceSorteado];
    }
}

void ConfereResposta(char resposta, CardGame *CardEscolhido)
{
    if(CardEscolhido != NULL)
    {
        if(resposta == CardEscolhido->correta)
        {
            printf("Resposta correta!\n");
        }
        else
        {
            printf("Resposta incorreta! A resposta correta era: %c\n", CardEscolhido->correta);
        }
    }
}

void ConsequenciaEscolha(char resposta, CardGame *CardEscolhido)
{
    if(CardEscolhido != NULL)
    {
        if(resposta == CardEscolhido->correta)
        {
            printf("Você ganhou um multiplicador de %d!\n", CardEscolhido->numMultiplicador);
        }
        else
        {
            printf("Você perdeu! Seu multiplicador foi reduzido para %.1f!\n", CardEscolhido->numRedutor);
        }
    }
}

void liberarCards(CardGame *cards)
{
    if(cards != NULL)
    {
        free(cards);
    }
}


