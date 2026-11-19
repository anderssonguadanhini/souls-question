/**
 * @file main.c
 * @brief Programa principal: carrega as perguntas, mostra o menu,
 *        executa a partida (15 níveis) e salva tudo em arquivo binário ao sair.
 */

#ifdef _WIN32
/* O windows.h DEVE vir antes do raylib.h.
 * 1) Funcoes com o mesmo nome nos dois: renomeadas so durante o include. */
#define Rectangle Win32Rectangle
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#include <windows.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
/* 2) Macros do windows.h que trocam o nome (ex.: LoadImage -> LoadImageA)
 *    e quebrariam as funcoes de mesmo nome do raylib. */
#undef DrawText
#undef DrawTextEx
#undef LoadImage
#undef PlaySound

/* MinGW antigo nao define esta constante */
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "raylib.h" /* reservado para a interface gráfica (parte criativa) */
#include "flameChaos.h"
#include "jogo_demilhao.h"

/**
 * @brief Retorna a cor ANSI correspondente ao nível (1 a 5).
 */
static const char *corNivelJogo(int nivel)
{
    switch (nivel)
    {
    case 1:
        return COR_CIANO; // Muito Fácil
    case 2:
        return COR_VERDE; // Fácil
    case 3:
        return COR_AMARELO; // Médio
    case 4:
        return COR_VERMELHO; // Difícil
    case 5:
        return COR_ROXO; // Muito Difícil
    default:
        return COR_RESET;
    }
}

/**
 * @brief Exibe as estatísticas de perguntas por nível de dificuldade.
 */
static void exibirEstatisticasBanco(const Banco *banco)
{
    int cont[6] = {0};
    for (int i = 0; i < banco->quantidade; i++)
    {
        int n = banco->dados[i].nivel;
        if (n >= 1 && n <= 5)
        {
            cont[n]++;
        }
    }

    printf("\n%s--- ESTATISTICAS DO BANCO DE PERGUNTAS ---%s\n", COR_CIANO, COR_RESET);
    printf("  %s1. Muito Facil:%s   %2d pergunta(s) (Minimo CSV: 20 | Partida: 2)\n", COR_CIANO, COR_RESET, cont[1]);
    printf("  %s2. Facil:%s         %2d pergunta(s) (Minimo CSV: 20 | Partida: 2)\n", COR_VERDE, COR_RESET, cont[2]);
    printf("  %s3. Medio:%s         %2d pergunta(s) (Minimo CSV: 15 | Partida: 4)\n", COR_AMARELO, COR_RESET, cont[3]);
    printf("  %s4. Dificil:%s       %2d pergunta(s) (Minimo CSV: 15 | Partida: 4)\n", COR_VERMELHO, COR_RESET, cont[4]);
    printf("  %s5. Muito Dificil:%s %2d pergunta(s) (Minimo CSV: 10 | Partida: 3)\n", COR_ROXO, COR_RESET, cont[5]);
    printf("  %sTOTAL NO BANCO:%s   %2d pergunta(s)\n", COR_BRANCO_NEGRITO, COR_RESET, banco->quantidade);
}

/**
 * @brief Executa uma partida completa de 15 perguntas com marcos de segurança.
 */
static void iniciarPartida(const Banco *banco)
{
    if (banco->quantidade <= 0)
    {
        tocarSom(3);
        printf("\n%s[!] Nenhuma pergunta cadastrada no banco!%s\n", COR_VERMELHO, COR_RESET);
        return;
    }

    const double premios[15] = {
        1000.0, 2000.0, 3000.0, 4000.0, 5000.0,           // 1-2 Muito Fácil | 3-4 Fácil | 5 Médio (1º Marco)
        10000.0, 20000.0, 30000.0, 40000.0, 50000.0,      // 6-8 Médio | 9 Difícil | 10 Difícil (2º Marco)
        100000.0, 200000.0, 300000.0, 500000.0, 1000000.0 // 11-12 Difícil | 13-15 Muito Difícil (1 Milhão)
    };

    int *usadas = calloc((size_t)banco->quantidade, sizeof(int));
    if (usadas == NULL)
    {
        tocarSom(3);
        printf("%s[!] Erro de memoria ao iniciar a partida!%s\n", COR_VERMELHO, COR_RESET);
        return;
    }

    double premioAtual = 0.0;
    double premioSeguro = 0.0;
    int venceuTudo = 1;

    for (int questao = 1; questao <= 15; questao++)
    {
        int nivelExigido;
        if (questao <= 2)
            nivelExigido = 1; // Muito Fácil (1 e 2)
        else if (questao <= 4)
            nivelExigido = 2; // Fácil (3 e 4)
        else if (questao <= 8)
            nivelExigido = 3; // Médio (5 a 8 - Marco no 5)
        else if (questao <= 12)
            nivelExigido = 4; // Difícil (9 a 12 - Marco no 10)
        else
            nivelExigido = 5; // Muito Difícil (13 a 15 - 1 Milhão)

        int *candidatas = malloc((size_t)banco->quantidade * sizeof(int));
        int qtdCandidatas = 0;

        for (int i = 0; i < banco->quantidade; i++)
        {
            if (!usadas[i] && banco->dados[i].nivel == nivelExigido)
            {
                candidatas[qtdCandidatas++] = i;
            }
        }

        if (qtdCandidatas == 0)
        {
            tocarSom(3);
            printf("\n%s[!] Nao existem perguntas suficientes de nivel '%s' para a questao %d!%s\n",
                   COR_AMARELO, textoDoNivel(nivelExigido), questao, COR_RESET);
            free(candidatas);
            venceuTudo = 0;
            break;
        }

        int idxSorteado = candidatas[rand() % qtdCandidatas];
        usadas[idxSorteado] = 1;
        free(candidatas);

        const Perguntas *p = &banco->dados[idxSorteado];
        const char *cor = corNivelJogo(p->nivel);

        limparTela();
        printf("%s========================================================================%s\n", cor, COR_RESET);
        printf("%s  QUESTAO %02d/15  |  NIVEL: %-13s  |  VALENDO: R$ %.2f%s\n",
               cor, questao, textoDoNivel(p->nivel), premios[questao - 1], COR_RESET);
        printf("  Jogo: %s%s%s\n", COR_CIANO, p->jogo, COR_RESET);

        if (questao == 5 || questao == 10)
        {
            printf("%s  *** MARCO DE SEGURANCA! ***%s\n", COR_AMARELO, COR_RESET);
        }
        else if (questao == 15)
        {
            printf("%s  *** PERGUNTA DO MILHAO! ***%s\n", COR_ROXO, COR_RESET);
        }
        printf("%s========================================================================%s\n", cor, COR_RESET);
        printf("  Premio Garantido (Marco): %sR$ %.2f%s\n\n", COR_VERDE, premioSeguro, COR_RESET);

        printf("  %s%s%s\n\n", COR_BRANCO_NEGRITO, p->enunciado, COR_RESET);
        char letras[4] = {'A', 'B', 'C', 'D'};
        for (int j = 0; j < 4; j++)
        {
            printf("    [%c] %s\n", letras[j], p->alternativa[j]);
        }

        char resp = '\0';
        while (1)
        {
            printf("\n  Sua resposta (A, B, C, D) ou [P] para Parar: ");
            if (scanf(" %c", &resp) != 1)
            {
                int c;
                while ((c = getchar()) != '\n' && c != EOF)
                {
                }
                continue;
            }
            resp = (char)toupper((unsigned char)resp);
            if ((resp >= 'A' && resp <= 'D') || resp == 'P')
            {
                break;
            }
            tocarSom(3);
            printf("%s  [!] Opcao invalida! Digite A, B, C, D ou P.%s\n", COR_VERMELHO, COR_RESET);
        }

        if (resp == 'P')
        {
            tocarSom(1);
            printf("\n%sVoce decidiu parar! Levou para casa: R$ %.2f%s\n", COR_AMARELO, premioAtual, COR_RESET);
            venceuTudo = 0;
            break;
        }

        if (resp == (char)toupper((unsigned char)p->correta))
        {
            premioAtual = premios[questao - 1];
            if (questao == 5 || questao == 10)
            {
                premioSeguro = premioAtual;
                tocarSom(2);
                printf("\n%s[OK] RESPOSTA CORRETA! Marco de seguranca atingido: R$ %.2f!%s\n",
                       COR_VERDE, premioSeguro, COR_RESET);
            }
            else
            {
                tocarSom(2);
                printf("\n%s[OK] RESPOSTA CORRETA! Premio acumulado: R$ %.2f!%s\n",
                       COR_VERDE, premioAtual, COR_RESET);
            }
            if (questao < 15)
            {
                pausar();
            }
        }
        else
        {
            tocarSom(3);
            printf("\n%s[X] RESPOSTA INCORRETA! A correta era [%c].%s\n",
                   COR_VERMELHO, (char)toupper((unsigned char)p->correta), COR_RESET);
            printf("%sVoce sai com o ultimo marco de seguranca: R$ %.2f%s\n",
                   COR_AMARELO, premioSeguro, COR_RESET);
            venceuTudo = 0;
            break;
        }
    }

    if (venceuTudo)
    {
        tocarSom(2);
        printf("\n%s*****************************************************************%s\n", COR_VERDE, COR_RESET);
        printf("%s   PARABENS! VOCE VENCEU O FLAME CHAOS E GANHOU R$ 1.000.000,00! %s\n", COR_VERDE, COR_RESET);
        printf("%s*****************************************************************%s\n", COR_VERDE, COR_RESET);
    }

    free(usadas);
    pausar();
}

/**
 * @brief Submenu de gerenciamento do banco de perguntas.
 */
static void submenuGerenciar(Banco *banco)
{
    int subOpcao = -1;
    do
    {
        printf("\n");
        printf("%s================================%s\n", COR_AMARELO, COR_RESET);
        printf("%s      GERENCIAR PERGUNTAS%s\n", COR_BRANCO_NEGRITO, COR_RESET);
        printf("%s================================%s\n", COR_AMARELO, COR_RESET);
        printf("  1. Cadastrar Pergunta\n");
        printf("  2. Listar Perguntas\n");
        printf("  3. Pesquisar por Nível\n");
        printf("  4. Alterar Pergunta\n");
        printf("  5. Excluir Pergunta\n");
        printf("  6. Exportar para CSV\n");
        printf("  7. Estatísticas por Nível\n");
        printf("  0. Voltar ao Menu Principal\n");
        printf("%s================================%s\n", COR_AMARELO, COR_RESET);

        int leitura = lerInteiro("Escolha uma opção: ", &subOpcao);
        if (leitura == -1)
        {
            subOpcao = 0;
        }
        else if (leitura == 0)
        {
            tocarSom(3);
            printf("%sEntrada inválida!%s\n", COR_VERMELHO, COR_RESET);
            subOpcao = -1;
            continue;
        }

        switch (subOpcao)
        {
        case 1:
            cadastrarPergunta(banco);
            break;
        case 2:
            listarPerguntas(banco);
            break;
        case 3:
            pesquisarPergunta(banco);
            break;
        case 4:
            alterarPergunta(banco);
            break;
        case 5:
            excluirPergunta(banco);
            break;
        case 6:
            exportarPerguntas(banco);
            break;
        case 7:
            exibirEstatisticasBanco(banco);
            break;
        case 0:
            tocarSom(1);
            break;
        default:
            tocarSom(3);
            printf("\n%sOpção inválida!%s\n", COR_VERMELHO, COR_RESET);
            break;
        }
    } while (subOpcao != 0);
}

/**
 * @brief Mostra o menu principal.
 */
static void mostrarMenu(void)
{
    printf("\n");
    printf("%s================================%s\n", COR_CIANO, COR_RESET);
    printf("%s        FLAME CHAOS%s\n", COR_BRANCO_NEGRITO, COR_RESET);
    printf("%s================================%s\n", COR_CIANO, COR_RESET);
    printf("  %s1. Jogar (15 Níveis / 1 Milhão)%s\n", COR_VERDE, COR_RESET);
    printf("  2. Gerenciar Perguntas (CRUD / CSV)\n");
    printf("  3. Regras e Marcos de Segurança\n");
    printf("  0. Sair e Salvar\n");
    printf("%s================================%s\n", COR_CIANO, COR_RESET);
}

/**
 * @brief Função principal.
 * @param argc Quantidade de argumentos.
 * @param argv Argumentos: o primeiro (opcional) é o caminho do CSV.
 *             Ex.: ./flameChaos caminho/do/perguntas.csv
 * @return EXIT_SUCCESS se tudo deu certo, EXIT_FAILURE se não carregou as perguntas.
 */
int main(int argc, char *argv[])
{
#ifdef _WIN32
    // Liga as cores ANSI no console do Windows
    HANDLE saida = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD modo = 0;
    GetConsoleMode(saida, &modo);
    SetConsoleMode(saida, modo | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    srand((unsigned int)time(NULL));
    configurarTerminal(); // UTF-8 no Windows (no Linux não faz nada)

    Banco banco = {NULL, 0, 0};              // vetor dinâmico vazio
    char caminhoBin[TAM_CAMINHO] = NOME_BIN; // onde o .bin será gravado

    // Se o usuário passou um caminho ao executar, usa ele como CSV
    const char *csvInformado = (argc > 1) ? argv[1] : NULL;

    // 1ª execução: lê o CSV. Nas outras: lê o binário
    if (iniciarBanco(&banco, csvInformado, caminhoBin, sizeof(caminhoBin)) <= 0)
    {
        printf("Erro ao carregar as perguntas.\n");
        printf("Confira se o CSV existe, está em UTF-8, separado por vírgula e com o formato certo.\n");
        printf("Você também pode executar assim: %s caminho/do/arquivo.csv\n", argv[0]);
        free(banco.dados);
        return EXIT_FAILURE;
    }
    tocarSom(2);
    printf("%s[OK] Perguntas carregadas: %d%s\n", COR_VERDE, banco.quantidade, COR_RESET);

    int opcao = -1;
    do
    {
        mostrarMenu();

        int leitura = lerInteiro("Escolha uma opção: ", &opcao);
        if (leitura == -1)
        {
            opcao = 0; // entrada fechada: sai salvando
        }
        else if (leitura == 0)
        {
            tocarSom(3);
            printf("%sEntrada inválida!%s\n", COR_VERMELHO, COR_RESET);
            opcao = -1;
            continue;
        }

        switch (opcao)
        {
        case 1:
            tocarSom(1);
            iniciarPartida(&banco);
            break;
        case 2:
            tocarSom(1);
            submenuGerenciar(&banco);
            break;
        case 3:
            tocarSom(1);
            printf("\n%s--- REGRAS E MARCOS DE SEGURANCA ---%s\n", COR_CIANO, COR_RESET);
            printf("  * Questoes 01 a 02: %sMuito Facil%s\n", COR_CIANO, COR_RESET);
            printf("  * Questoes 03 a 04: %sFacil%s\n", COR_VERDE, COR_RESET);
            printf("  * Questoes 05 a 08: %sMedio%s (Questao 05 = 1o Marco: R$ 5.000)\n", COR_AMARELO, COR_RESET);
            printf("  * Questoes 09 a 12: %sDificil%s (Questao 10 = 2o Marco: R$ 50.000)\n", COR_VERMELHO, COR_RESET);
            printf("  * Questoes 13 a 15: %sMuito Dificil%s (Questao 15 = R$ 1.000.000)\n", COR_ROXO, COR_RESET);
            break;
        case 0:
            printf("\nSalvando dados...\n");
            if (salvarBinario(&banco, caminhoBin) == 0)
            {
                tocarSom(2);
                printf("%s[OK] %d pergunta(s) salva(s) em '%s'.%s\n",
                       COR_VERDE, banco.quantidade, caminhoBin, COR_RESET);
            }
            else
            {
                tocarSom(3);
                printf("%s[!] Erro ao salvar '%s'!%s\n", COR_VERMELHO, caminhoBin, COR_RESET);
            }
            printf("Encerrando FlameChaos...\n");
            break;
        default:
            tocarSom(3);
            printf("\n%sOpção inválida!%s\n", COR_VERMELHO, COR_RESET);
            break;
        }
    } while (opcao != 0);

    free(banco.dados); // devolve a memória do vetor dinâmico
    return EXIT_SUCCESS;
}