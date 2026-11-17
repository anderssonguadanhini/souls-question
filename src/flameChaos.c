#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "raylib.h"
#include "flameChaos.h"

#ifdef _WIN32
#include <windows.h>
#define LIMPAR_TELA() system("cls")
#else
#include <unistd.h>
#define LIMPAR_TELA() system("clear")
#endif

// Limpa o buffer do teclado após leituras com scanf
static void limparBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

const char *obterNomeDificuldade(Dificuldade d)
{
    switch (d)
    {
    case FACIL:
        return "Facil";
    case MEDIO:
        return "Medio";
    case DIFICIL:
        return "Dificil";
    case MUITO_DIFICIL:
        return "Muito Dificil";
    default:
        return "Desconhecido";
    }
}

void tocarSom(int tipo)
{
#ifdef _WIN32
    switch (tipo)
    {
    case 1:
        Beep(750, 90);
        break;
    case 2:
        Beep(523, 100);
        Beep(659, 100);
        Beep(784, 150);
        break;
    case 3:
        Beep(250, 200);
        break;
    case 4:
        Beep(440, 120);
        Beep(415, 120);
        Beep(392, 250);
        break;
    }
#else
    (void)tipo;
    printf("\a");
    fflush(stdout);
#endif
}

const char *obterCorDificuldade(Dificuldade d)
{
    switch (d)
    {
    case FACIL:
        return COR_VERDE;
    case MEDIO:
        return COR_AMARELO;
    case DIFICIL:
        return COR_VERMELHO;
    case MUITO_DIFICIL:
        return COR_ROXO;
    default:
        return COR_RESET;
    }
}

void salvarPerguntas(Pergunta *lista, int tam)
{
    FILE *arquivo = fopen(ARQUIVO_DADOS, "wb");
    if (arquivo == NULL)
    {
        printf("%s[!] Erro ao abrir '%s' para salvamento!%s\n", COR_VERMELHO, ARQUIVO_DADOS, COR_RESET);
        tocarSom(3);
        return;
    }

    fwrite(&tam, sizeof(int), 1, arquivo);
    if (tam > 0)
    {
        fwrite(lista, sizeof(Pergunta), tam, arquivo);
    }

    fclose(arquivo);
    printf("%s[OK] %d pergunta(s) salva(s) em '%s'.%s\n", COR_VERDE, tam, ARQUIVO_DADOS, COR_RESET);
}

void carregarPerguntas(Pergunta **lista, int *tam, int *capacidade)
{
    FILE *arquivo = fopen(ARQUIVO_DADOS, "rb");
    if (arquivo == NULL)
    {
        printf("%s[i] Nenhum banco de dados anterior encontrado. Iniciando novo arquivo.%s\n", COR_CIANO, COR_RESET);
        return;
    }

    int totalSalvo = 0;
    if (fread(&totalSalvo, sizeof(int), 1, arquivo) != 1)
    {
        fclose(arquivo);
        return;
    }

    if (totalSalvo > 0)
    {
        if (totalSalvo > *capacidade)
        {
            Pergunta *temp = realloc(*lista, totalSalvo * sizeof(Pergunta));
            if (temp == NULL)
            {
                printf("%s[!] Erro de memoria ao carregar dados do disco!%s\n", COR_VERMELHO, COR_RESET);
                fclose(arquivo);
                return;
            }
            *lista = temp;
            *capacidade = totalSalvo;
        }

        size_t lidos = fread(*lista, sizeof(Pergunta), totalSalvo, arquivo);
        *tam = (int)lidos;
    }

    fclose(arquivo);
    printf("%s[OK] %d pergunta(s) carregada(s) de '%s'.%s\n", COR_VERDE, *tam, ARQUIVO_DADOS, COR_RESET);
}

void cadastrarPergunta(Pergunta **lista, int *tam, int *capacidade)
{
    if (*tam >= *capacidade)
    {
        int novaCap = (*capacidade) * 2;
        Pergunta *temp = realloc(*lista, novaCap * sizeof(Pergunta));
        if (temp == NULL)
        {
            printf("%s[!] Erro ao realocar memoria!%s\n", COR_VERMELHO, COR_RESET);
            return;
        }
        *lista = temp;
        *capacidade = novaCap;
    }

    Pergunta nova;
    limparBuffer();

    printf("\n--- CADASTRAR NOVA PERGUNTA ---\n");
    printf("Enunciado: ");
    fgets(nova.enunciado, sizeof(nova.enunciado), stdin);
    nova.enunciado[strcspn(nova.enunciado, "\n")] = '\0';

    char letras[4] = {'A', 'B', 'C', 'D'};
    for (int i = 0; i < 4; i++)
    {
        printf("Alternativa [%c]: ", letras[i]);
        fgets(nova.alternativas[i], sizeof(nova.alternativas[i]), stdin);
        nova.alternativas[i][strcspn(nova.alternativas[i], "\n")] = '\0';
    }

    do
    {
        printf("Alternativa correta (A, B, C ou D): ");
        scanf(" %c", &nova.correta);
        nova.correta = (char)toupper(nova.correta);
    } while (nova.correta < 'A' || nova.correta > 'D');

    int nivel;
    do
    {
        printf("Nivel (0-Facil, 1-Medio, 2-Dificil, 3-Muito Dificil): ");
        if (scanf("%d", &nivel) != 1)
        {
            limparBuffer();
            nivel = -1;
        }
    } while (nivel < 0 || nivel > 3);
    nova.nivel = (Dificuldade)nivel;

    (*lista)[*tam] = nova;
    (*tam)++;
    tocarSom(2);
    printf("%s[OK] Pergunta cadastrada com sucesso!%s\n", COR_VERDE, COR_RESET);
}

void listarPerguntas(Pergunta *lista, int tam)
{
    if (tam == 0)
    {
        printf("\n%s[!] Nenhuma pergunta cadastrada.%s\n", COR_AMARELO, COR_RESET);
        return;
    }

    printf("\n================ LISTA DE PERGUNTAS (%d) ================\n", tam);
    char letras[4] = {'A', 'B', 'C', 'D'};
    for (int i = 0; i < tam; i++)
    {
        const char *cor = obterCorDificuldade(lista[i].nivel);
        printf("\n%s[#%d] [%s]%s %s\n", cor, i + 1, obterNomeDificuldade(lista[i].nivel), COR_RESET, lista[i].enunciado);
        for (int j = 0; j < 4; j++)
        {
            printf("   [%c] %s\n", letras[j], lista[i].alternativas[j]);
        }
        printf("   Correta: %s%c%s\n", COR_VERDE, lista[i].correta, COR_RESET);
    }
    printf("=========================================================\n");
}

void pesquisarPergunta(Pergunta *lista, int tam)
{
    if (tam == 0)
    {
        tocarSom(3);
        printf("\n%s[!] Nenhuma pergunta cadastrada no sistema.%s\n", COR_VERMELHO, COR_RESET);
        return;
    }

    int escolhaNivel;
    LIMPAR_TELA();
    printf("%s==================================================%s\n", COR_CIANO, COR_RESET);
    printf("%s       PESQUISAR PERGUNTAS POR DIFICULDADE        %s\n", COR_BRANCO_NEGRITO, COR_RESET);
    printf("%s==================================================%s\n", COR_CIANO, COR_RESET);
    printf("  %s[0] Facil%s\n", COR_VERDE, COR_RESET);
    printf("  %s[1] Medio%s\n", COR_AMARELO, COR_RESET);
    printf("  %s[2] Dificil%s\n", COR_VERMELHO, COR_RESET);
    printf("  %s[3] Muito Dificil (MODO CHAOS)%s\n", COR_ROXO, COR_RESET);
    printf("%s==================================================%s\n", COR_CIANO, COR_RESET);
    printf("Escolha o nivel desejado: ");

    if (scanf("%d", &escolhaNivel) != 1 || escolhaNivel < 0 || escolhaNivel > 3)
    {
        tocarSom(3);
        printf("\n%s[!] Nivel invalido!%s\n", COR_VERMELHO, COR_RESET);
        limparBuffer();
        return;
    }

    Dificuldade nivelBuscado = (Dificuldade)escolhaNivel;

    int *indicesEncontrados = malloc(tam * sizeof(int));
    if (indicesEncontrados == NULL)
    {
        printf("Erro de memoria!\n");
        return;
    }

    int totalEncontradas = 0;
    for (int i = 0; i < tam; i++)
    {
        if (lista[i].nivel == nivelBuscado)
        {
            indicesEncontrados[totalEncontradas++] = i;
        }
    }

    if (totalEncontradas == 0)
    {
        tocarSom(3);
        printf("\n%s[!] Nenhuma pergunta encontrada para o nivel %s.%s\n",
               COR_AMARELO, obterNomeDificuldade(nivelBuscado), COR_RESET);
        free(indicesEncontrados);
        return;
    }

    if (nivelBuscado == MUITO_DIFICIL)
        tocarSom(4);
    else
        tocarSom(1);

    int posAtual = 0;
    int mostrarGabarito = 0;
    char comando;
    const char *corTema = obterCorDificuldade(nivelBuscado);

    do
    {
        LIMPAR_TELA();
        int idxReal = indicesEncontrados[posAtual];
        Pergunta p = lista[idxReal];

        printf("%s+----------------------------------------------------------+%s\n", corTema, COR_RESET);
        printf("%s|  FLAME CHAOS - NIVEL: %-15s   [Card %d de %d]  |%s\n",
               corTema, obterNomeDificuldade(p.nivel), posAtual + 1, totalEncontradas, COR_RESET);
        printf("%s+----------------------------------------------------------+%s\n", corTema, COR_RESET);

        printf("  Progresso: [");
        for (int i = 0; i < totalEncontradas; i++)
        {
            if (i == posAtual)
                printf("%s#%s", corTema, COR_RESET);
            else
                printf("-");
        }
        printf("] (ID Geral: #%d)\n\n", idxReal + 1);

        printf("  %sPERGUNTA:%s\n", COR_BRANCO_NEGRITO, COR_RESET);
        printf("  %s\n\n", p.enunciado);

        char letras[4] = {'A', 'B', 'C', 'D'};
        for (int i = 0; i < 4; i++)
        {
            if (mostrarGabarito && toupper(p.correta) == letras[i])
            {
                printf("  %s--> [%c] %s  (CORRETA!)%s\n",
                       COR_VERDE, letras[i], p.alternativas[i], COR_RESET);
            }
            else
            {
                printf("      [%c] %s\n", letras[i], p.alternativas[i]);
            }
        }

        printf("\n%s+----------------------------------------------------------+%s\n", corTema, COR_RESET);
        printf("  %s[A]%s Anterior  |  %s[D]%s Proxima  |  %s[R]%s Revelar  |  %s[S]%s Sair\n",
               COR_CIANO, COR_RESET, COR_CIANO, COR_RESET,
               COR_AMARELO, COR_RESET, COR_VERMELHO, COR_RESET);
        printf("%s+----------------------------------------------------------+%s\n", corTema, COR_RESET);
        printf("Comando: ");

        scanf(" %c", &comando);
        comando = (char)toupper(comando);

        switch (comando)
        {
        case 'D':
            if (posAtual < totalEncontradas - 1)
            {
                posAtual++;
                mostrarGabarito = 0;
                tocarSom(1);
            }
            else
            {
                tocarSom(3);
            }
            break;

        case 'A':
            if (posAtual > 0)
            {
                posAtual--;
                mostrarGabarito = 0;
                tocarSom(1);
            }
            else
            {
                tocarSom(3);
            }
            break;

        case 'R':
            mostrarGabarito = !mostrarGabarito;
            if (mostrarGabarito)
                tocarSom(2);
            else
                tocarSom(1);
            break;

        case 'S':
            tocarSom(1);
            break;

        default:
            tocarSom(3);
            break;
        }

    } while (comando != 'S');

    free(indicesEncontrados);
    LIMPAR_TELA();
}

void alterarPergunta(Pergunta *lista, int tam)
{
    if (tam == 0)
    {
        printf("\n%s[!] Nenhuma pergunta cadastrada para alterar.%s\n", COR_AMARELO, COR_RESET);
        return;
    }

    int id;
    printf("\nDigite o numero (ID) da pergunta para alterar (1 a %d): ", tam);
    if (scanf("%d", &id) != 1 || id < 1 || id > tam)
    {
        printf("%s[!] ID invalido!%s\n", COR_VERMELHO, COR_RESET);
        limparBuffer();
        return;
    }

    int idx = id - 1;
    limparBuffer();

    printf("Novo enunciado: ");
    fgets(lista[idx].enunciado, sizeof(lista[idx].enunciado), stdin);
    lista[idx].enunciado[strcspn(lista[idx].enunciado, "\n")] = '\0';

    char letras[4] = {'A', 'B', 'C', 'D'};
    for (int i = 0; i < 4; i++)
    {
        printf("Nova alternativa [%c]: ", letras[i]);
        fgets(lista[idx].alternativas[i], sizeof(lista[idx].alternativas[i]), stdin);
        lista[idx].alternativas[i][strcspn(lista[idx].alternativas[i], "\n")] = '\0';
    }

    do
    {
        printf("Nova alternativa correta (A, B, C ou D): ");
        scanf(" %c", &lista[idx].correta);
        lista[idx].correta = (char)toupper(lista[idx].correta);
    } while (lista[idx].correta < 'A' || lista[idx].correta > 'D');

    int nivel;
    do
    {
        printf("Novo nivel (0-Facil, 1-Medio, 2-Dificil, 3-Muito Dificil): ");
        if (scanf("%d", &nivel) != 1)
        {
            limparBuffer();
            nivel = -1;
        }
    } while (nivel < 0 || nivel > 3);
    lista[idx].nivel = (Dificuldade)nivel;

    tocarSom(2);
    printf("%s[OK] Pergunta #%d alterada com sucesso!%s\n", COR_VERDE, id, COR_RESET);
}

void excluirPergunta(Pergunta **lista, int *tam, int *capacidade)
{
    if (*tam == 0)
    {
        printf("\n%s[!] Nenhuma pergunta cadastrada para excluir.%s\n", COR_AMARELO, COR_RESET);
        return;
    }

    int id;
    printf("\nDigite o numero (ID) da pergunta para excluir (1 a %d): ", *tam);
    if (scanf("%d", &id) != 1 || id < 1 || id > *tam)
    {
        printf("%s[!] ID invalido!%s\n", COR_VERMELHO, COR_RESET);
        limparBuffer();
        return;
    }

    int idx = id - 1;
    for (int i = idx; i < (*tam) - 1; i++)
    {
        (*lista)[i] = (*lista)[i + 1];
    }
    (*tam)--;

    if (*tam > 0 && *tam <= (*capacidade) / 4 && *capacidade > 5)
    {
        int novaCap = (*capacidade) / 2;
        Pergunta *temp = realloc(*lista, novaCap * sizeof(Pergunta));
        if (temp != NULL)
        {
            *lista = temp;
            *capacidade = novaCap;
        }
    }

    tocarSom(2);
    printf("%s[OK] Pergunta #%d excluida com sucesso!%s\n", COR_VERDE, id, COR_RESET);
}