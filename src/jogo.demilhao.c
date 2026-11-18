/**
 * @file jogo.demilhao.c
 * @author João Gulherme dos anjos (joao.080607@alunos.utfpr.edu.br)
 * @brief
 * @version 0.1
 * @date 2026-11-15
 *
 * @copyright Copyright (c) 2026
 *
 */

/*
1) A lista com os dados para as perguntas iniciais será construída lendo os dados
de um arquivo .csv (valores separados por vírgula) construído pela equipe. Esse
arquivo deve ter no mínimo a seguinte quantidade de perguntas: 20 muito fácil,
20 fácil, 15 médio, 15 difícil e 10 muito difícil.

2) O sistema deverá permitir exportar os dados atuais em um arquivo de texto no
formato .CSV (separado por vírgula).*/

/* ---------------------------------------------------------------
 * BIBLIOTECAS
 * stdio.h   -> printf, fopen, fgets, fprintf, fread, fwrite...
 * stdlib.h  -> malloc, realloc, free, atoi
 * string.h  -> strcmp, strtok, strcspn
 * windows.h -> SetConsoleOutputCP (acentos corretos no terminal)
 * --------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* ---------------------------------------------------------------
 * STRUCT Perguntas
 * Guarda UMA pergunta do jogo (uma linha do CSV).
 * --------------------------------------------------------------- */
typedef struct
{
    int id;                  // número da pergunta no CSV
    int nivel;               // dificuldade em número: 1 a 5
    char jogo[32];           // jogo de origem (Dark Souls, Elden Ring...)
    char enunciado[200];     // texto da pergunta
    char alternativa[4][128]; // as 4 alternativas: A, B, C e D (índices 0 a 3)
    char correta;            // letra da resposta certa: 'A' a 'D'
} Perguntas;

/* ---------------------------------------------------------------
 * STRUCT Banco
 * É o vetor dinâmico que guarda todas as perguntas.
 * Cresce com realloc, então não existe limite fixo de perguntas.
 * --------------------------------------------------------------- */
typedef struct
{
    Perguntas *dados; // ponteiro para o vetor de perguntas (NULL = vazio)
    int quantidade;   // quantas perguntas existem de verdade
    int capacidade;   // quantas perguntas cabem no vetor agora
} Banco;

/* ---------------------------------------------------------------
 * PROTÓTIPOS
 * Avisam ao compilador que as funções existem, para o main poder
 * chamá-las antes de elas serem escritas lá embaixo.
 * --------------------------------------------------------------- */
int carregarCSV(Banco *banco, const char *arquivo);
int nivelDoTexto(const char *texto);
const char *textoDoNivel(int nivel);
int exportarCSV(const Banco *banco, const char *arquivo);
int salvarBinario(const Banco *banco, const char *arquivo);
int carregarBinario(Banco *banco, const char *arquivo);
int iniciarBanco(Banco *banco);

/* ---------------------------------------------------------------
 * MAIN
 * Fluxo do programa: carrega as perguntas, (aqui entram o menu, o
 * CRUD e o jogo), salva tudo no binário e libera a memória.
 * --------------------------------------------------------------- */
int main(void)
{
    SetConsoleOutputCP(65001); // terminal em UTF-8: mostra acentos certos
    Banco banco = {NULL, 0, 0}; // banco vazio: sem vetor, 0 perguntas, capacidade 0

    // Carrega as perguntas: do .bin se existir, senão do CSV (1ª execução)
    if (iniciarBanco(&banco) <= 0)
    {
        printf("Erro ao carregar as perguntas.\n");
        return 1; // sai com código de erro
    }
    printf("Perguntas carregadas: %d\n", banco.quantidade);

    // ... aqui entram o menu, o CRUD e o jogo (partes dos colegas) ...

    // Ao sair, grava todas as perguntas (com as alterações) no binário
    salvarBinario(&banco, "perguntas.bin");

    free(banco.dados); // devolve ao sistema a memória do vetor dinâmico
    return 0;
} // main

/* ---------------------------------------------------------------
 * nivelDoTexto
 * Converte o texto da dificuldade (como vem no CSV) em número.
 * Retorna 1 a 5, ou 0 se o texto não for reconhecido.
 * strcmp devolve 0 quando os dois textos são iguais.
 * --------------------------------------------------------------- */
int nivelDoTexto(const char *texto)
{
    if (strcmp(texto, "Muito Fácil") == 0)
        return 1;
    if (strcmp(texto, "Fácil") == 0)
        return 2;
    if (strcmp(texto, "Médio") == 0)
        return 3;
    if (strcmp(texto, "Difícil") == 0)
        return 4;
    if (strcmp(texto, "Muito Difícil") == 0)
        return 5;
    return 0; // não  reconhecido
}

/* ---------------------------------------------------------------
 * carregarCSV
 * Lê o arquivo .csv e guarda cada pergunta no vetor dinâmico.
 * Retorna: a quantidade de perguntas guardadas, ou -1 se der erro
 * (arquivo não abriu ou faltou memória).
 * --------------------------------------------------------------- */
int carregarCSV(Banco *banco, const char *arquivo)
{
    // Abre o arquivo para leitura ("r")
    FILE *arq = fopen(arquivo, "r");
    if (arq == NULL)
    {
        return -1; // não conseguil abrir
    }

    // Lê a primeira linha (cabeçalho com os nomes das colunas) e descarta
    char linha[1024];
    fgets(linha, sizeof(linha), arq); // lê e descarta o cabeçalho;

    // passo 4 entra aqui: ler as linhas  de dados

    // Repete uma vez por linha de dados, até o arquivo acabar
    while (fgets(linha, sizeof(linha), arq) != NULL)
    {
        // Troca o Enter do fim da linha por '\0' para ele não grudar no último campo
        linha[strcspn(linha, "\r\n")] = '\0'; // tira o enter do fim

        // Separa a linha nas vírgulas: campo[0] = id, campo[1] = dificuldade,
        // campo[2] = jogo, campo[3] = pergunta, campo[4..7] = alternativas,
        // campo[8] = resposta correta
        char *campo[9];
        int n = 0;

        char *parte = strtok(linha, ",");
        while (parte != NULL && n < 9)
        {
            campo[n] = parte;
            n++;
            parte = strtok(NULL, ",");
        }

        // Linha que não tem exatamente 9 campos está com defeito: pula
        if (n != 9)
        {
            continue; // linha com defeito: ignora
        }

        // Converte o texto da dificuldade em número; se não reconhecer, pula
        int nivel = nivelDoTexto(campo[1]);
        if (nivel == 0)
        {
            continue; // dificuldade desconhecida: ignora
        }

        // Se o vetor está cheio, aumenta a capacidade:
        // começa com 16 e depois dobra (16, 32, 64...)
        if (banco->quantidade == banco->capacidade)
        {
            int nova;
            if (banco->capacidade == 0)
            {
                nova = 16;
            }
            else
            {
                nova = banco->capacidade * 2;
            }
            // O resultado vai primeiro para tmp: se o realloc falhar,
            // banco->dados continua válido e não perdemos o que já foi lido
            Perguntas *tmp = realloc(banco->dados, nova * sizeof(Perguntas));
            if (tmp == NULL)
            {
                fclose(arq);
                return -1; // faltou memoria
            }
            banco->dados = tmp;
            banco->capacidade = nova;
        }

        // p aponta para a próxima posição livre do vetor; preenche os campos dela
        Perguntas *p = &banco->dados[banco->quantidade];
        p->id = atoi(campo[0]); // converte texto em número inteiro
        p->nivel = nivel;
        // snprintf copia o texto sem passar do tamanho do campo
        snprintf(p->jogo, sizeof(p->jogo), "%s", campo[2]);
        snprintf(p->enunciado, sizeof(p->enunciado), "%s", campo[3]);

        // Copia as 4 alternativas (campo[4] a campo[7])
        for (int i = 0; i < 4; i++)
        {
            snprintf(p->alternativa[i], sizeof(p->alternativa[i]), "%s", campo[4 + i]);
        }
        p->correta = campo[8][0]; // só a primeira letra: 'A', 'B', 'C' ou 'D'
        banco->quantidade++;      // conta a pergunta guardada
    }

    fclose(arq); // fecha o arquivo
    return banco->quantidade; // devolve quantas perguntas foram lidas
}

/* ---------------------------------------------------------------
 * textoDoNivel
 * Faz o caminho inverso de nivelDoTexto: converte o número (1 a 5)
 * no texto da dificuldade. Usada na exportação para CSV.
 * --------------------------------------------------------------- */
const char *textoDoNivel(int nivel)
{
    if (nivel == 1) return "Muito Fácil";
    if (nivel == 2) return "Fácil";
    if (nivel == 3) return "Médio";
    if (nivel == 4) return "Difícil";
    if (nivel == 5) return "Muito Difícil";
    return "";
}

/* ---------------------------------------------------------------
 * exportarCSV
 * Grava as perguntas atuais em um arquivo .csv (separado por vírgula),
 * no mesmo formato do CSV original, para poder ser lido de volta.
 * Retorna 0 se deu certo, ou -1 se não conseguiu criar o arquivo.
 * --------------------------------------------------------------- */
int exportarCSV(const Banco *banco, const char *arquivo)
{
    // Abre para escrita ("w"): cria o arquivo ou apaga o que existir
    FILE *arq = fopen(arquivo, "w");
    if (arq == NULL)
    {
        return -1;
    }

    // Primeira linha: cabeçalho com o nome das colunas
    fprintf(arq, "id,dificuldade,jogo,pergunta,alternativa_a,alternativa_b,"
                 "alternativa_c,alternativa_d,resposta_correta\n");

    // Uma linha por pergunta, com os campos separados por vírgula
    for (int i = 0; i < banco->quantidade; i++)
    {
        const Perguntas *p = &banco->dados[i];
        fprintf(arq, "%d,%s,%s,%s,%s,%s,%s,%s,%c\n",
                p->id, textoDoNivel(p->nivel), p->jogo, p->enunciado,
                p->alternativa[0], p->alternativa[1],
                p->alternativa[2], p->alternativa[3], p->correta);
    }

    fclose(arq);
    return 0;
}

/* ---------------------------------------------------------------
 * salvarBinario
 * Grava o banco inteiro em um arquivo binário.
 * Formato do arquivo: primeiro a quantidade (um int), depois o vetor
 * de perguntas de uma vez só.
 * Retorna 0 se deu certo, ou 1 se não conseguiu criar o arquivo.
 * --------------------------------------------------------------- */
int salvarBinario(const Banco *banco, const char *arquivo)
{
    // "wb" = escrita em modo binário
    FILE *arq = fopen(arquivo,"wb");
    if(arq == NULL)
    {
        return 1;
    }

    // Grava a quantidade, para saber quantas perguntas ler depois
    fwrite(&banco->quantidade, sizeof(int), 1, arq);
    // Grava todas as perguntas de uma vez
    fwrite(banco->dados, sizeof(Perguntas), banco->quantidade,arq);

    fclose(arq);
    return 0;
}

/* ---------------------------------------------------------------
 * carregarBinario
 * Lê o arquivo binário e preenche o banco.
 * Retorna: a quantidade lida (>0), 0 se o arquivo não existe
 * (primeira execução), ou -1 se o arquivo está corrompido/sem memória.
 * --------------------------------------------------------------- */
int carregarBinario(Banco *banco, const char *arquivo)
{
    // "rb" = leitura em modo binário
    FILE *arq = fopen(arquivo, "rb");
    if(arq == NULL)
    {
        return 0;   //arquivo inesistente
    }

    // Lê a quantidade de perguntas gravada no começo do arquivo
    int qtd;
    if(fread(&qtd,sizeof(int), 1, arq) != 1 || qtd < 0)
    {
        fclose(arq);
        return -1;              //arquivo corrompido
    }

    // Reserva memória para as perguntas (no mínimo 1 posição)
    Perguntas *tmp = malloc((qtd > 0 ? qtd : 1) * sizeof(Perguntas));
    if (tmp ==NULL)
    {
        fclose(arq);
        return -1;
    }

    // Lê todas as perguntas de uma vez; se vier menos que o esperado, o arquivo está quebrado
    if(fread(tmp, sizeof(Perguntas),qtd, arq) != (size_t)qtd)
    {
        free(tmp);
        fclose(arq);
        return -1;
    }

    // Deu tudo certo: entrega o vetor lido para o banco
    banco->dados = tmp;
    banco->quantidade = qtd;
    banco->capacidade = qtd > 0 ? qtd : 1;

    fclose(arq);
    return qtd;

}

/* ---------------------------------------------------------------
 * iniciarBanco
 * Decide de onde vêm as perguntas ao abrir o programa:
 *  - se existe o .bin: lê só ele (já tem as alterações salvas);
 *  - se não existe (1ª execução): lê o CSV criado pela equipe.
 * Retorna a quantidade de perguntas, ou -1 se der erro.
 * --------------------------------------------------------------- */
int iniciarBanco(Banco *banco)
{
    int r = carregarBinario(banco, "perguntas.bin");

    if (r > 0)
    {
        return r;               // já existe .bin: usa só ele
    }
    if (r < 0)
    {
        return -1;              // .bin corrompido
    }

    // r == 0: primeira execução, lê o CSV
    return carregarCSV(banco, "perguntas_souls_like.csv");
}