/**
 * @file jogo.c
 * @brief Protótipo antigo da partida no terminal (não faz parte do programa
 *        final; a versão atual está em src/main.c).
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "jogo.h"

/**
 * @brief Sorteia uma pergunta de uma dificuldade, sem repetir as já usadas.
 * @param perguntas   Vetor de perguntas.
 * @param quantidade  Quantidade de perguntas.
 * @param dificuldade Nível desejado (1 a 5).
 * @param usadas      Marca (1) as perguntas já usadas na partida.
 * @return Índice da pergunta sorteada, ou -1 se não houver disponível.
 */
int sortearPergunta(Pergunta *perguntas, int quantidade,
                    int dificuldade, int usadas[]) {

    int disponiveis = 0;

    // Conta quantas perguntas ainda podem ser utilizadas
    for (int i = 0; i < quantidade; i++) {
        if (perguntas[i].dificuldade == dificuldade && !usadas[i]) {
            disponiveis++;
        }
    }

    if (disponiveis == 0)
        return -1;

    // Sorteia uma posição entre as perguntas disponíveis
    int sorteio = rand() % disponiveis;

    for (int i = 0; i < quantidade; i++) {

        if (perguntas[i].dificuldade == dificuldade && !usadas[i]) {

            if (sorteio == 0) {
                usadas[i] = 1;
                return i;
            }

            sorteio--;
        }
    }

    return -1;
}


/**
 * @brief Mostra uma pergunta e confere a resposta digitada.
 * @param pergunta Pergunta a apresentar.
 * @return 1 se acertou, 0 se errou.
 */
int responderPergunta(Pergunta pergunta) {

    char resposta;

    printf("\n%s\n\n", pergunta.enunciado);

    for (int i = 0; i < 4; i++) {
        printf("%c) %s\n", 'A' + i, pergunta.alternativas[i]);
    }

    printf("\nSua resposta: ");
    scanf(" %c", &resposta);

    resposta = toupper((unsigned char)resposta);

    if (resposta == pergunta.resposta) {
        printf("\nResposta correta!\n");
        return 1;
    }

    printf("\nResposta incorreta!\n");
    printf("A resposta correta era: %c\n", pergunta.resposta);

    return 0;
}


/**
 * @brief Controla as 15 perguntas de uma partida.
 * @param perguntas  Vetor de perguntas.
 * @param quantidade Quantidade de perguntas.
 */
void jogarPartida(Pergunta *perguntas, int quantidade) {

    if (quantidade <= 0) {
        printf("Nao existem perguntas cadastradas!\n");
        return;
    }

    int nivel = 1;
    int marcoSeguranca = 0;

    // Vetor que identifica as perguntas já utilizadas
    int *usadas = calloc(quantidade, sizeof(int));

    if (usadas == NULL) {
        printf("Erro ao alocar memoria!\n");
        return;
    }

    while (nivel <= 15) {

        int dificuldade;

        printf("\n================================\n");
        printf("           NIVEL %d\n", nivel);
        printf("================================\n");

        // Define a dificuldade de acordo com o nível
        switch (nivel) {

            case 1:
            case 2:
                dificuldade = MUITO_FACIL;
                break;

            case 3:
            case 4:
                dificuldade = FACIL;
                break;

            case 5:
            case 6:
            case 7:
            case 8:
                dificuldade = MEDIO;
                break;

            case 9:
            case 10:
            case 11:
            case 12:
                dificuldade = DIFICIL;
                break;

            case 13:
            case 14:
            case 15:
                dificuldade = MUITO_DIFICIL;
                break;
        }

        int indice = sortearPergunta(
            perguntas, quantidade, dificuldade, usadas
        );

        if (indice == -1) {
            printf("Nao existem perguntas suficientes "
                   "para esta dificuldade!\n");
            break;
        }

        // Apresenta e verifica a pergunta
        if (!responderPergunta(perguntas[indice])) {

            printf("\nFim da partida!\n");
            printf("Marco de seguranca alcancado: nivel %d\n",
                   marcoSeguranca);

            break;
        }

        // Primeiro marco de segurança
        if (nivel == 5) {
            marcoSeguranca = 5;
            printf("\nPRIMEIRO MARCO DE SEGURANCA ALCANCADO!\n");
        }

        // Segundo marco de segurança
        if (nivel == 10) {
            marcoSeguranca = 10;
            printf("\nSEGUNDO MARCO DE SEGURANCA ALCANCADO!\n");
        }

        // Vitória final
        if (nivel == 15) {
            printf("\nPARABENS! VOCE GANHOU UM MILHAO!\n");
        }

        nivel++;
    }

    free(usadas);
}
