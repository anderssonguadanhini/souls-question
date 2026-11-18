#ifndef ARQUIVO_H
#define ARQUIVO_H

#include "pergunta.h"

int carregarDados(Pergunta **perguntas, int *quantidade);

int carregarCSV(Pergunta **perguntas, int *quantidade);

int carregarBinario(Pergunta **perguntas, int *quantidade);

int salvarBinario(Pergunta *perguntas, int quantidade);

#endif
