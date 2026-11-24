/**
 * @file sistema_cardGame.h
 * @author João Gulherme dos anjos (joao.080607@alunos.utfpr.edu.br)
 * @brief 
 * @version 0.1
 * @date 2026-11-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */

 #ifndef SISTEMA_CARDGAME_H
 #define SISTEMA_CARDGAME_H
 #include "jogo_demilhao.h"

 enum Dificuldade
 {
    
    MUITO_FACIL = 1,
    FACIL = 2,
    MEDIO = 3,
    DIFICIL = 4,
    MUITO_DIFICIL = 5
 };

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
    REDUTOR_X2 = 2,
    REDUTOR_X3 = 4,
    REDUTOR_X4 = 6,
    REDUTOR_X5 = 8,
    REDUTOR_X6 = 10,
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

void inicializarCards(CardGame *cards, int tamanho, Banco *banco);
int ProcurarDificuldadeCards(CardGame *cards, int tamanho, enum Dificuldade dificuldade,CardGame *vetorResultado);
void EscolherCard(CardGame *vetorResultado,CardGame *CardEscolhido, int tamanhoResultado);
void ConfereResposta(char resposta, CardGame *CardEscolhido);
void ConsequenciaEscolha(char resposta, CardGame *CardEscolhido);
void liberarCards(CardGame *cards);

void Teste(Banco *banco);

 #endif