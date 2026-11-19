/**
 * @file jogo_demilhao.c
 * @author João Guilherme dos Anjos (joao.080607@alunos.utfpr.edu.br)
 * @brief Jogo de perguntas (souls-like) - Windows e Linux, acha o CSV sozinho
 *        (sem main: o main fica no main.c; as declarações estão no jogo_demilhao.h)
 * @version 0.5
 * @date 2026-11-18
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
formato .CSV (separado por vírgula).

COMO O CSV É ENCONTRADO (nesta ordem)
  1) Caminho passado ao executar:   ./programa caminho/do/arquivo.csv
  2) Busca automática: procura "perguntas_souls_like.csv" e, se não achar,
     qualquer arquivo "perguntas*.csv" (ex.: perguntas.csv). A busca olha a
     pasta de onde o programa foi executado, a pasta do executável, as
     pastas acima delas e todas as subpastas (até 3 níveis).
  3) Se mesmo assim não achar, o programa pede para você digitar o caminho.
*/

/* No Linux, libera readlink/stat/opendir mesmo se compilar com -std=c99 */
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

/* ---------------------------------------------------------------
 * BIBLIOTECAS
 * stdio.h   -> printf, fopen, fgets, fprintf, fread, fwrite...
 * stdlib.h  -> malloc, realloc, free, atoi, system
 * string.h  -> strcmp, strtok, strcspn, strrchr, strncat
 * ctype.h   -> tolower (comparar nomes sem diferenciar maiúscula)
 *
 * PARTE ESPECÍFICA DE SISTEMA OPERACIONAL
 * _WIN32 só existe quando compilamos no Windows. Assim, o windows.h
 * só é incluído lá, e o Linux usa dirent.h, sys/stat.h e unistd.h.
 * --------------------------------------------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "jogo_demilhao.h" // Perguntas, Banco, constantes e protótipos públicos

#ifdef _WIN32
    #include <windows.h>                  // SetConsoleOutputCP, GetModuleFileNameA, FindFirstFileA
    #define LIMPAR_TELA() system("cls")   // limpar tela no Windows
#else
    #include <unistd.h>                   // readlink (descobrir onde o executável está)
    #include <dirent.h>                   // opendir, readdir (listar arquivos de uma pasta)
    #include <sys/stat.h>                 // stat (saber se algo é pasta)
    #define LIMPAR_TELA() system("clear") // limpar tela no Linux
#endif

// Tipos de busca de arquivo (uso interno)
#define MODO_CSV_EXATO    0 // arquivo chamado exatamente perguntas_souls_like.csv
#define MODO_CSV_QUALQUER 1 // qualquer arquivo perguntas*.csv
#define MODO_BIN          2 // arquivo perguntas.bin

#define PROFUNDIDADE_BUSCA 3 // quantas subpastas para baixo a busca desce
#define NIVEIS_ACIMA       2 // quantas pastas para cima a busca sobe

/* ---------------------------------------------------------------
 * PROTÓTIPOS PRIVADOS (static = só existem dentro deste arquivo,
 * por isso não ficam no .h e não conflitam com outros arquivos)
 * --------------------------------------------------------------- */
static int igualIgnorandoCaixa(const char *a, const char *b);
static int nomeCombina(const char *nome, int modo);
static int pastaDoExecutavel(char *saida, size_t tam);
static int tratarEntrada(const char *pasta, const char *nome, int ehPasta, int fase,
                         int profundidade, int modo, char *saida, size_t tam);
static int buscarNaPasta(const char *pasta, int profundidade, int modo, char *saida, size_t tam);
static int procurarArquivo(int modo, char *saida, size_t tam);
static int pedirCaminho(char *saida, size_t tam);

/* ---------------------------------------------------------------
 * configurarTerminal
 * No Windows, coloca o console em UTF-8 para mostrar os acentos.
 * No Linux o terminal já é UTF-8, então não precisa fazer nada.
 * --------------------------------------------------------------- */
void configurarTerminal(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
#endif
}

/* ---------------------------------------------------------------
 * limparTela
 * Limpa o terminal usando o comando certo para cada sistema
 * (definido lá no começo pelo LIMPAR_TELA).
 * O (void) avisa ao compilador que ignoramos o retorno do system.
 * --------------------------------------------------------------- */
void limparTela(void)
{
    (void)LIMPAR_TELA();
}

/* ---------------------------------------------------------------
 * pausar
 * Espera o jogador apertar Enter. Usa só getchar, então é padrão
 * C e funciona igual no Windows e no Linux.
 * --------------------------------------------------------------- */
void pausar(void)
{
    printf("\nPressione Enter para continuar...");
    int c;
    // descarta o que sobrou no buffer até o fim da linha
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
    // espera um novo Enter
    getchar();
}

/* ---------------------------------------------------------------
 * arquivoExiste
 * Tenta abrir o arquivo para leitura. Retorna 1 se conseguiu
 * (o arquivo existe), ou 0 se não.
 * --------------------------------------------------------------- */
int arquivoExiste(const char *caminho)
{
    FILE *arq = fopen(caminho, "rb");
    if (arq == NULL)
    {
        return 0;
    }
    fclose(arq);
    return 1;
}

/* ---------------------------------------------------------------
 * igualIgnorandoCaixa
 * Compara dois textos sem diferenciar maiúscula de minúscula.
 * Necessário porque o Linux diferencia "Perguntas.csv" de
 * "perguntas.csv" e o Windows não. Retorna 1 se forem iguais.
 * --------------------------------------------------------------- */
static int igualIgnorandoCaixa(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0')
    {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
        {
            return 0;
        }
        a++;
        b++;
    }
    return *a == *b; // os dois terminaram juntos?
}

/* ---------------------------------------------------------------
 * nomeCombina
 * Diz se o nome de um arquivo é o que estamos procurando:
 *  - MODO_CSV_EXATO:   perguntas_souls_like.csv
 *  - MODO_CSV_QUALQUER: começa com "perguntas" e termina com ".csv"
 *  - MODO_BIN:         perguntas.bin
 * Retorna 1 se combina, ou 0 se não.
 * --------------------------------------------------------------- */
static int nomeCombina(const char *nome, int modo)
{
    if (modo == MODO_CSV_EXATO)
    {
        return igualIgnorandoCaixa(nome, NOME_CSV);
    }
    if (modo == MODO_BIN)
    {
        return igualIgnorandoCaixa(nome, NOME_BIN);
    }

    // MODO_CSV_QUALQUER: "perguntas" (9 letras) + ".csv" (4 letras) = mínimo 13
    size_t n = strlen(nome);
    if (n < 13)
    {
        return 0;
    }
    const char *inicio = "perguntas";
    for (int i = 0; i < 9; i++)
    {
        if (tolower((unsigned char)nome[i]) != inicio[i])
        {
            return 0;
        }
    }
    return igualIgnorandoCaixa(nome + n - 4, ".csv");
}

/* ---------------------------------------------------------------
 * pastaDoExecutavel
 * Descobre em qual pasta o programa (.exe ou binário) está guardado,
 * e não de onde ele foi executado. Isso muda de sistema para sistema:
 *  - Windows: GetModuleFileNameA
 *  - Linux:   readlink("/proc/self/exe")
 * Escreve o caminho em "saida" (sempre com '/') e retorna 1 se deu
 * certo, ou 0 se não foi possível descobrir.
 * --------------------------------------------------------------- */
static int pastaDoExecutavel(char *saida, size_t tam)
{
#ifdef _WIN32
    DWORD n = GetModuleFileNameA(NULL, saida, (DWORD)tam);
    if (n == 0 || n >= tam)
    {
        return 0;
    }
    // troca '\' por '/' para tratar igual ao Linux
    for (char *p = saida; *p != '\0'; p++)
    {
        if (*p == '\\')
        {
            *p = '/';
        }
    }
#else
    ssize_t n = readlink("/proc/self/exe", saida, tam - 1);
    if (n <= 0)
    {
        return 0;
    }
    saida[n] = '\0';
#endif

    // Corta o nome do executável, ficando só com a pasta
    char *barra = strrchr(saida, '/');
    if (barra == NULL)
    {
        return 0;
    }
    *barra = '\0';
    return 1;
}

/* ---------------------------------------------------------------
 * tratarEntrada
 * Analisa UM item (arquivo ou pasta) encontrado dentro de uma pasta.
 * A busca é feita em duas fases, para os arquivos mais próximos
 * serem preferidos aos das subpastas:
 *  - fase 0: só olha arquivos e vê se o nome combina;
 *  - fase 1: só olha subpastas e procura dentro delas.
 * Retorna 1 se achou o arquivo (caminho escrito em "saida").
 * --------------------------------------------------------------- */
static int tratarEntrada(const char *pasta, const char *nome, int ehPasta, int fase,
                  int profundidade, int modo, char *saida, size_t tam)
{
    // Ignora "." , ".." , itens ocultos (como .git) e node_modules
    if (nome[0] == '.' || strcmp(nome, "node_modules") == 0)
    {
        return 0;
    }

    char completo[TAM_CAMINHO];
    snprintf(completo, sizeof(completo), "%s/%s", pasta, nome);

    if (fase == 0 && !ehPasta && nomeCombina(nome, modo))
    {
        snprintf(saida, tam, "%s", completo);
        return 1;
    }
    if (fase == 1 && ehPasta)
    {
        return buscarNaPasta(completo, profundidade - 1, modo, saida, tam);
    }
    return 0;
}

/* ---------------------------------------------------------------
 * buscarNaPasta
 * Procura o arquivo dentro de "pasta" e, descendo até "profundidade"
 * níveis, dentro das subpastas dela. Listar o conteúdo de uma pasta
 * é diferente em cada sistema:
 *  - Windows: FindFirstFileA / FindNextFileA
 *  - Linux:   opendir / readdir
 * Retorna 1 se achou (caminho em "saida"), ou 0 se não achou.
 * --------------------------------------------------------------- */
static int buscarNaPasta(const char *pasta, int profundidade, int modo, char *saida, size_t tam)
{
    for (int fase = 0; fase < 2; fase++)
    {
        // fase 1 = entrar nas subpastas; só se ainda puder descer
        if (fase == 1 && profundidade <= 0)
        {
            break;
        }

        int achou = 0;

#ifdef _WIN32
        WIN32_FIND_DATAA dados;
        char padrao[TAM_CAMINHO];
        snprintf(padrao, sizeof(padrao), "%s/*", pasta);

        HANDLE h = FindFirstFileA(padrao, &dados);
        if (h == INVALID_HANDLE_VALUE)
        {
            return 0; // pasta não existe ou sem permissão
        }
        do
        {
            int ehPasta = (dados.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if (tratarEntrada(pasta, dados.cFileName, ehPasta, fase, profundidade, modo, saida, tam))
            {
                achou = 1;
                break;
            }
        } while (FindNextFileA(h, &dados));
        FindClose(h);
#else
        DIR *d = opendir(pasta);
        if (d == NULL)
        {
            return 0; // pasta não existe ou sem permissão
        }
        struct dirent *e;
        while ((e = readdir(d)) != NULL)
        {
            // stat diz se o item é uma pasta
            char completo[TAM_CAMINHO];
            snprintf(completo, sizeof(completo), "%s/%s", pasta, e->d_name);
            struct stat st;
            int ehPasta = (stat(completo, &st) == 0 && S_ISDIR(st.st_mode));

            if (tratarEntrada(pasta, e->d_name, ehPasta, fase, profundidade, modo, saida, tam))
            {
                achou = 1;
                break;
            }
        }
        closedir(d);
#endif

        if (achou)
        {
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------
 * procurarArquivo
 * Procura o arquivo (conforme o "modo") a partir de dois pontos de
 * partida: a pasta de onde o programa foi executado e a pasta onde o
 * executável está. De cada um, também sobe algumas pastas
 * (NIVEIS_ACIMA), e em cada nível olha também todas as subpastas.
 * Retorna 1 se achou (caminho em "saida"), ou 0 se não achou.
 * --------------------------------------------------------------- */
static int procurarArquivo(int modo, char *saida, size_t tam)
{
    char pastaExe[TAM_CAMINHO];
    const char *bases[2];
    int totalBases = 0;

    bases[totalBases++] = "."; // pasta atual
    if (pastaDoExecutavel(pastaExe, sizeof(pastaExe)))
    {
        bases[totalBases++] = pastaExe; // pasta do executável
    }

    for (int b = 0; b < totalBases; b++)
    {
        // nivel = quantas pastas acima da base vamos olhar
        for (int nivel = 0; nivel <= NIVEIS_ACIMA; nivel++)
        {
            // Monta o caminho: base + "/.." repetido "nivel" vezes
            char pasta[TAM_CAMINHO];
            snprintf(pasta, sizeof(pasta), "%s", bases[b]);
            for (int k = 0; k < nivel; k++)
            {
                strncat(pasta, "/..", sizeof(pasta) - strlen(pasta) - 1);
            }

            if (buscarNaPasta(pasta, PROFUNDIDADE_BUSCA, modo, saida, tam))
            {
                return 1;
            }
        }
    }
    return 0;
}

/* ---------------------------------------------------------------
 * pedirCaminho
 * Último recurso: pede para o usuário digitar (ou arrastar para o
 * terminal) o caminho do CSV. Retira aspas, que o Windows coloca ao
 * arrastar um arquivo. Retorna 1 se o arquivo existe, ou 0 se não.
 * --------------------------------------------------------------- */
static int pedirCaminho(char *saida, size_t tam)
{
    printf("Não encontrei o CSV de perguntas automaticamente.\n");
    printf("Digite o caminho do arquivo (ou só Enter para sair): ");

    if (fgets(saida, (int)tam, stdin) == NULL)
    {
        return 0;
    }
    saida[strcspn(saida, "\r\n")] = '\0';

    // Tira aspas no começo e no fim, se houver
    size_t n = strlen(saida);
    if (n >= 2 && (saida[0] == '"' || saida[0] == '\'') && saida[n - 1] == saida[0])
    {
        memmove(saida, saida + 1, n - 2);
        saida[n - 2] = '\0';
    }

    if (saida[0] == '\0')
    {
        return 0; // usuário só apertou Enter
    }
    if (!arquivoExiste(saida))
    {
        printf("Arquivo não encontrado: %s\n", saida);
        return 0;
    }
    return 1;
}

/* ---------------------------------------------------------------
 * caminhoDoBinario
 * Define onde o .bin será gravado: na mesma pasta do CSV que foi
 * usado. Assim o .bin sempre fica junto do CSV, não importa de onde
 * o programa for executado.
 * --------------------------------------------------------------- */
void caminhoDoBinario(const char *caminhoCSV, char *saida, size_t tam)
{
    // Copia o caminho do CSV e corta o nome do arquivo (fica só a pasta)
    snprintf(saida, tam, "%s", caminhoCSV);

    char *barra = strrchr(saida, '/');
    char *barraInvertida = strrchr(saida, '\\'); // caminhos digitados no Windows
    if (barraInvertida != NULL && (barra == NULL || barraInvertida > barra))
    {
        barra = barraInvertida;
    }

    if (barra == NULL)
    {
        snprintf(saida, tam, "%s", NOME_BIN); // CSV na pasta atual
        return;
    }

    barra[1] = '\0'; // mantém a barra, descarta o nome do CSV
    strncat(saida, NOME_BIN, tam - strlen(saida) - 1);
}

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
    return 0; // não reconhecido
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
        return -1; // não conseguiu abrir
    }

    // Lê a primeira linha (cabeçalho com os nomes das colunas) e descarta
    char linha[1024];
    fgets(linha, sizeof(linha), arq); // lê e descarta o cabeçalho

    // Repete uma vez por linha de dados, até o arquivo acabar
    while (fgets(linha, sizeof(linha), arq) != NULL)
    {
        // Troca o Enter do fim da linha por '\0' para ele não grudar no último campo.
        // "\r\n" cobre os dois sistemas: Windows termina linha com \r\n e Linux com \n.
        linha[strcspn(linha, "\r\n")] = '\0';

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
                return -1; // faltou memória
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

    fclose(arq);              // fecha o arquivo
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
    FILE *arq = fopen(arquivo, "wb");
    if (arq == NULL)
    {
        return 1;
    }

    // Grava a quantidade, para saber quantas perguntas ler depois
    fwrite(&banco->quantidade, sizeof(int), 1, arq);
    // Grava todas as perguntas de uma vez
    fwrite(banco->dados, sizeof(Perguntas), banco->quantidade, arq);

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
    if (arq == NULL)
    {
        return 0; // arquivo inexistente
    }

    // Lê a quantidade de perguntas gravada no começo do arquivo
    int qtd;
    if (fread(&qtd, sizeof(int), 1, arq) != 1 || qtd < 0)
    {
        fclose(arq);
        return -1; // arquivo corrompido
    }

    // Reserva memória para as perguntas (no mínimo 1 posição)
    Perguntas *tmp = malloc((qtd > 0 ? qtd : 1) * sizeof(Perguntas));
    if (tmp == NULL)
    {
        fclose(arq);
        return -1;
    }

    // Lê todas as perguntas de uma vez; se vier menos que o esperado, o arquivo está quebrado
    if (fread(tmp, sizeof(Perguntas), qtd, arq) != (size_t)qtd)
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
 *  - se o usuário informou um CSV (argv[1]): lê ele, ignorando o .bin;
 *  - senão, se existe o .bin: lê só ele (já tem as alterações salvas);
 *  - senão (1ª execução): procura o CSV nas pastas e lê; se não achar,
 *    pede para o usuário digitar o caminho.
 * Em "caminhoBin" devolve onde o .bin deve ser gravado ao sair.
 * Retorna a quantidade de perguntas, ou -1 se der erro.
 * --------------------------------------------------------------- */
int iniciarBanco(Banco *banco, const char *csvInformado, char *caminhoBin, size_t tamBin)
{
    char caminhoCSV[TAM_CAMINHO];

    if (csvInformado != NULL)
    {
        // Usuário escolheu o CSV: lê ele direto, sem olhar o .bin
        snprintf(caminhoCSV, sizeof(caminhoCSV), "%s", csvInformado);
    }
    else
    {
        // Procura o .bin; se existe, usa só ele
        char achadoBin[TAM_CAMINHO];
        if (procurarArquivo(MODO_BIN, achadoBin, sizeof(achadoBin)))
        {
            int r = carregarBinario(banco, achadoBin);
            if (r > 0)
            {
                snprintf(caminhoBin, tamBin, "%s", achadoBin);
                return r; // já existe .bin: usa só ele
            }
            if (r < 0)
            {
                return -1; // .bin corrompido
            }
        }

        // Sem .bin (primeira execução): procura o CSV.
        // 1º tenta o nome exato; 2º qualquer perguntas*.csv; 3º pergunta ao usuário
        if (!procurarArquivo(MODO_CSV_EXATO, caminhoCSV, sizeof(caminhoCSV)) &&
            !procurarArquivo(MODO_CSV_QUALQUER, caminhoCSV, sizeof(caminhoCSV)) &&
            !pedirCaminho(caminhoCSV, sizeof(caminhoCSV)))
        {
            return -1; // não achou o CSV
        }
    }

    printf("Lendo perguntas de: %s\n", caminhoCSV);
    caminhoDoBinario(caminhoCSV, caminhoBin, tamBin); // .bin ficará junto do CSV
    return carregarCSV(banco, caminhoCSV);
}