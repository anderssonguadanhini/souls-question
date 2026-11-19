
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arquivo.h"

#define ARQUIVO_CSV "perguntas.csv"
#define ARQUIVO_BIN "perguntas.bin"

#define CAPACIDADE_INICIAL 5


/*
 * Carrega perguntas do CSV para um vetor dinamico.
 //ordem do csv (dificuldade,enunciado,A,B,C,D,resposta)
 */
int carregarCSV(Pergunta **perguntas, int *quantidade) {

    FILE *arquivo = fopen(ARQUIVO_CSV, "r");

    if (arquivo == NULL) {
        printf("Erro ao abrir o CSV!\n");
        return 0;
    }

    int capacidade = CAPACIDADE_INICIAL;
    *quantidade = 0;

    *perguntas = malloc(capacidade * sizeof(Pergunta));

    if (*perguntas == NULL) {
        fclose(arquivo);
        return 0;
    }

    char linha[1200];

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {

        Pergunta nova;
        char *campo = strtok(linha, ",");

        if (campo == NULL)
            continue;

        nova.dificuldade = atoi(campo);

        campo = strtok(NULL, ",");
        if (campo == NULL) continue;

        strcpy(nova.enunciado, campo);

        int erro = 0;

        for (int i = 0; i < 4; i++) {

            campo = strtok(NULL, ",");

            if (campo == NULL) {
                erro = 1;
                break;
            }

            strcpy(nova.alternativas[i], campo);
        }

        if (erro)
            continue;

        campo = strtok(NULL, ",\r\n");

        if (campo == NULL)
            continue;

        nova.resposta = campo[0];

        if (nova.dificuldade < MUITO_FACIL ||
            nova.dificuldade > MUITO_DIFICIL ||
            nova.resposta < 'A' || nova.resposta > 'D') {
            continue;
        }

        // Aumenta o vetor quando necessario
        if (*quantidade == capacidade) {

            capacidade += 5;

            Pergunta *temp = realloc(
                *perguntas, capacidade * sizeof(Pergunta)
            );

            if (temp == NULL) {
                free(*perguntas);
                *perguntas = NULL;
                *quantidade = 0;
                fclose(arquivo);
                return 0;
            }

            *perguntas = temp;
        }

        (*perguntas)[*quantidade] = nova;
        (*quantidade)++;
    }

    fclose(arquivo);

    return 1;
}


/*
 * Carrega perguntas do arquivo binario.
 */
int carregarBinario(Pergunta **perguntas, int *quantidade) {

    FILE *arquivo = fopen(ARQUIVO_BIN, "rb");

    if (arquivo == NULL)
        return 0;

    if (fread(quantidade, sizeof(int), 1, arquivo) != 1) {
        fclose(arquivo);
        return 0;
    }

    if (*quantidade < 0) {
        fclose(arquivo);
        return 0;
    }

    *perguntas = NULL;

    if (*quantidade > 0) {

        *perguntas = malloc(*quantidade * sizeof(Pergunta));

        if (*perguntas == NULL) {
            fclose(arquivo);
            return 0;
        }

        if (fread(*perguntas, sizeof(Pergunta),
                  *quantidade, arquivo) != (size_t)*quantidade) {

            free(*perguntas);
            *perguntas = NULL;
            *quantidade = 0;
            fclose(arquivo);
            return 0;
        }
    }

    fclose(arquivo);

    return 1;
}


/*
 * Salva perguntas no arquivo binario.
 */
int salvarBinario(Pergunta *perguntas, int quantidade) {

    FILE *arquivo = fopen(ARQUIVO_BIN, "wb");

    if (arquivo == NULL) {
        printf("Erro ao salvar arquivo binario!\n");
        return 0;
    }

    if (fwrite(&quantidade, sizeof(int), 1, arquivo) != 1) {
        fclose(arquivo);
        return 0;
    }

    if (quantidade > 0 &&
        fwrite(perguntas, sizeof(Pergunta), quantidade, arquivo)
        != (size_t)quantidade) {

        fclose(arquivo);
        return 0;
    }

    fclose(arquivo);

    return 1;
}


/*
 * Verifica se existe arquivo binario.
 * Caso contrario, carrega o CSV.
 */
int carregarDados(Pergunta **perguntas, int *quantidade) {

    FILE *arquivo = fopen(ARQUIVO_BIN, "rb");

    if (arquivo != NULL) {

        fclose(arquivo);

        printf("Carregando arquivo binario...\n");

        return carregarBinario(perguntas, quantidade);
    }

    printf("Primeira execucao: carregando CSV...\n");

    return carregarCSV(perguntas, quantidade);
}
