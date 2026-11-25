/**
 * @file arquivo.h
 * @brief Protótipo antigo: leitura e gravação das perguntas (CSV e binário).
 */
#ifndef ARQUIVO_H
#define ARQUIVO_H

#include "pergunta.h"

/**
 * @brief Carrega do binário se ele existir; senão, do CSV.
 * @param perguntas  Recebe o vetor alocado.
 * @param quantidade Recebe a quantidade de perguntas.
 * @return 1 se deu certo, 0 se der erro.
 */
int carregarDados(Pergunta **perguntas, int *quantidade);

/**
 * @brief Carrega as perguntas do CSV.
 * @param perguntas  Recebe o vetor alocado.
 * @param quantidade Recebe a quantidade de perguntas.
 * @return 1 se deu certo, 0 se der erro.
 */
int carregarCSV(Pergunta **perguntas, int *quantidade);

/**
 * @brief Carrega as perguntas do arquivo binário.
 * @param perguntas  Recebe o vetor alocado.
 * @param quantidade Recebe a quantidade de perguntas.
 * @return 1 se deu certo, 0 se der erro.
 */
int carregarBinario(Pergunta **perguntas, int *quantidade);

/**
 * @brief Salva as perguntas no arquivo binário.
 * @param perguntas  Vetor de perguntas.
 * @param quantidade Quantidade de perguntas.
 * @return 1 se deu certo, 0 se der erro.
 */
int salvarBinario(Pergunta *perguntas, int quantidade);

#endif /* ARQUIVO_H */
