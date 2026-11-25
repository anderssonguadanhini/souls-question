/**
 * @file sistema_cardGame.c
 * @author Guilherme
 * @brief Sistema de cartas do jogo: cria cartas a partir do banco de perguntas,
 *        sorteia por dificuldade e confere a resposta do jogador.
 * @version 0.4
 * @date 2026-11-24
 *
 * @copyright Copyright (c) 2026
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "jogo_demilhao.h"
#include "sistema_cardGame.h"

/* ------------------------------------------------------------------ */
/* Funcoes auxiliares internas (static: invisiveis para outros .c,     */
/* entao nao geram conflito de nomes no projeto).                      */
/* ------------------------------------------------------------------ */

/**
 * @brief Descarta o que sobrou na entrada padrao ate o fim da linha.
 *
 * Usada depois de um scanf que falhou, para nao ficar preso em lixo.
 */
static void limparBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
    }
}

/**
 * @brief Compara a resposta do jogador com a resposta correta da carta.
 *
 * @param[in] resposta Letra escolhida pelo jogador.
 * @param[in] card     Carta sorteada.
 * @return 1 se a resposta estiver correta (ignora maiusculas/minusculas),
 *         0 se estiver errada ou se @p card for NULL.
 */
static int respostaCorreta(char resposta, const CardGame *card)
{
    return card != NULL &&
           toupper((unsigned char)resposta) == toupper((unsigned char)card->correta);
}

/**
 * @brief Sorteia um inteiro no intervalo fechado [min, max].
 *
 * @note Chame srand(time(NULL)) uma vez no main antes de usar esta funcao,
 *       senao a sequencia sorteada sera sempre a mesma.
 *
 * @param min Menor valor possivel (inclusive).
 * @param max Maior valor possivel (inclusive). Se for menor que @p min,
 *            a funcao devolve @p min (evita divisao por zero).
 * @return Inteiro sorteado entre min e max.
 */
static int sortear(int min, int max)
{
    if (max < min)
    {
        return min;
    }
    return rand() % (max - min + 1) + min;
}

/**
 * @brief Converte as perguntas do banco em cartas do jogo.
 *
 * Para cada pergunta copia nivel, enunciado, alternativas e resposta correta,
 * e define o multiplicador (acerto) e o redutor (erro) conforme a dificuldade:
 * - MUITO_FACIL: MULTIPLICADOR_X2 / REDUTOR_X2
 * - FACIL: MULTIPLICADOR_X3 / REDUTOR_X3
 * - MEDIO: MULTIPLICADOR_X4 / REDUTOR_X4
 * - DIFICIL: MULTIPLICADOR_X5 / REDUTOR_X5
 * - MUITO_DIFICIL: MULTIPLICADOR_X6 / REDUTOR_X6
 *
 * Niveis fora de 1 a 5 recebem multiplicador 1 e redutor 1.0 (neutros).
 *
 * @param[out] cards   Vetor de cartas ja alocado com pelo menos @p tamanho posicoes.
 * @param[in]  tamanho Quantidade de perguntas a converter. Se for maior que
 *                     banco->quantidade, e reduzido para esse valor.
 * @param[in]  banco   Banco com as perguntas de origem.
 *
 * @warning Se @p cards ou @p banco forem NULL, a funcao apenas escreve uma
 *          mensagem em stderr e retorna sem fazer nada.
 * @note Textos maiores que o campo da carta sao truncados (nunca estouram).
 */
void inicializarCards(CardGame *cards, int tamanho, Banco *banco)
{
    if (cards == NULL || banco == NULL)
    {
        fprintf(stderr, "Ponteiro nulo passado para inicializarCards\n");
        return;
    }

    /* nunca le alem do que o banco realmente tem */
    if (tamanho > banco->quantidade)
    {
        tamanho = banco->quantidade;
    }

    for (int i = 0; i < tamanho; i++)
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
        default:
            /* nivel invalido no banco: valores neutros */
            cards[i].numMultiplicador = 1;
            cards[i].numRedutor = 1.0;
            break;
        }

        /* snprintf copia no maximo sizeof(campo) bytes e sempre termina com '\0' */
        snprintf(cards[i].enunciado, sizeof(cards[i].enunciado), "%s", banco->dados[i].enunciado);
        for (int j = 0; j < 4; j++)
        {
            snprintf(cards[i].alternativa[j], sizeof(cards[i].alternativa[j]), "%s", banco->dados[i].alternativa[j]);
        }
        cards[i].correta = banco->dados[i].correta;
    }
}

/**
 * @brief Filtra as cartas de uma determinada dificuldade.
 *
 * Copia para @p vetorResultado todas as cartas de @p cards cuja dificuldade
 * seja igual a @p dificuldade, mantendo a ordem original.
 *
 * @param[in]  cards          Vetor com todas as cartas.
 * @param[in]  tamanho        Quantidade de cartas em @p cards.
 * @param[in]  dificuldade    Dificuldade procurada.
 * @param[out] vetorResultado Vetor que recebe as cartas encontradas. Deve ter
 *                            espaco para, no maximo, @p tamanho cartas.
 * @return Quantidade de cartas copiadas para @p vetorResultado (0 se nenhuma
 *         ou se algum ponteiro for NULL).
 */
int ProcurarDificuldadeCards(CardGame *cards, int tamanho, enum Dificuldade dificuldade, CardGame *vetorResultado)
{
    int tamanhoResultado = 0;

    if (cards == NULL || vetorResultado == NULL)
    {
        return 0;
    }

    for (int i = 0; i < tamanho; i++)
    {
        if (cards[i].dificuldade == dificuldade)
        {
            vetorResultado[tamanhoResultado] = cards[i];
            tamanhoResultado++;
        }
    }
    return tamanhoResultado;
}

/**
 * @brief Escolhe uma carta aleatoria dentro de um vetor de cartas.
 *
 * @param[in]  vetorResultado Vetor de cartas candidatas.
 * @param[out] CardEscolhido  Recebe uma copia da carta sorteada.
 * @param[in]  tamanhoResultado Quantidade de cartas em @p vetorResultado.
 *
 * @note Nao faz nada se algum ponteiro for NULL ou se
 *       @p tamanhoResultado for menor ou igual a zero.
 * @see sortear
 */
void EscolherCard(CardGame *vetorResultado, CardGame *CardEscolhido, int tamanhoResultado)
{
    if (vetorResultado != NULL && CardEscolhido != NULL && tamanhoResultado > 0)
    {
        int indiceSorteado = sortear(0, tamanhoResultado - 1);
        *CardEscolhido = vetorResultado[indiceSorteado];
    }
}

/**
 * @brief Informa na tela se a resposta do jogador esta correta.
 *
 * A comparacao ignora maiusculas e minusculas ('a' e 'A' valem o mesmo).
 *
 * @param[in] resposta       Letra escolhida pelo jogador.
 * @param[in] CardEscolhido  Carta que foi sorteada.
 */
void ConfereResposta(char resposta, CardGame *CardEscolhido)
{
    if (CardEscolhido == NULL)
    {
        return;
    }

    if (respostaCorreta(resposta, CardEscolhido))
    {
        printf("Resposta correta!\n");
    }
    else
    {
        printf("Resposta incorreta! A resposta correta era: %c\n", CardEscolhido->correta);
    }
}

/**
 * @brief Mostra a consequencia da resposta: multiplicador no acerto, redutor no erro.
 *
 * @param[in] resposta       Letra escolhida pelo jogador.
 * @param[in] CardEscolhido  Carta que foi sorteada.
 *
 * @note Esta funcao apenas exibe a mensagem; nao altera pontuacao nem estado.
 */
void ConsequenciaEscolha(char resposta, CardGame *CardEscolhido)
{
    if (CardEscolhido == NULL)
    {
        return;
    }

    if (respostaCorreta(resposta, CardEscolhido))
    {
        printf("Voce ganhou um multiplicador de %d!\n", (int)CardEscolhido->numMultiplicador);
    }
    else
    {
        printf("Voce perdeu! Seu multiplicador foi reduzido para %.1f!\n", (double)CardEscolhido->numRedutor);
    }
}

/**
 * @brief Libera a memoria de um vetor de cartas alocado dinamicamente.
 *
 * @param[in] cards Vetor alocado com malloc/calloc. Se for NULL, nada acontece.
 *
 * @warning Depois da chamada o ponteiro fica invalido; nao use @p cards novamente.
 */
void liberarCards(CardGame *cards)
{
    free(cards); /* free(NULL) e seguro pelo padrao C */
}

/**
 * @brief Rodada de teste no terminal.
 *
 * Fluxo: monta as cartas a partir do banco, pede a dificuldade (1 a 5),
 * sorteia uma carta desse nivel, mostra o enunciado e as alternativas,
 * le a resposta (A a D) e exibe o resultado e a consequencia.
 *
 * @param[in] banco Banco com as perguntas carregadas.
 *
 * @note Confira se o campo @c quantidade existe na struct Banco; se o nome
 *       for outro, ajuste a linha que define @c tamanho.
 * @see inicializarCards, ProcurarDificuldadeCards, EscolherCard,
 *      ConfereResposta, ConsequenciaEscolha
 */
void Teste(Banco *banco)
{
    if (banco == NULL)
    {
        fprintf(stderr, "Banco nulo passado para Teste\n");
        return;
    }

    int dificuldadeEscolhida = 0;
    char alternativa = '\0';

    /* ATENCAO: confira o nome do campo de quantidade na struct Banco */
    int tamanho = banco->quantidade;
    if (tamanho <= 0)
    {
        printf("Banco de perguntas vazio.\n");
        return;
    }

    CardGame *carta = malloc(tamanho * sizeof(CardGame));
    if (carta == NULL)
    {
        fprintf(stderr, "Falha ao alocar memoria\n");
        return;
    }

    inicializarCards(carta, tamanho, banco);

    printf("Escolha a dificuldade (1 a 5): ");
    if (scanf("%d", &dificuldadeEscolhida) != 1 || dificuldadeEscolhida < MUITO_FACIL || dificuldadeEscolhida > MUITO_DIFICIL)
    {
        printf("Dificuldade invalida.\n");
        limparBuffer();
        liberarCards(carta);
        return;
    }

    /* aloca com o tamanho total: sempre comporta os resultados */
    CardGame *vetorResultado = malloc(tamanho * sizeof(CardGame));
    if (vetorResultado == NULL)
    {
        fprintf(stderr, "Falha ao alocar memoria\n");
        liberarCards(carta);
        return;
    }

    int tamanhoResultado = ProcurarDificuldadeCards(carta, tamanho, (enum Dificuldade)dificuldadeEscolhida, vetorResultado);
    if (tamanhoResultado == 0)
    {
        printf("Nenhuma pergunta encontrada para essa dificuldade.\n");
        liberarCards(vetorResultado);
        liberarCards(carta);
        return;
    }

    CardGame cartaEscolhida;
    EscolherCard(vetorResultado, &cartaEscolhida, tamanhoResultado);

    printf("Pergunta: %s\n", cartaEscolhida.enunciado);
    for (int i = 0; i < 4; i++)
    {
        printf("%s\n", cartaEscolhida.alternativa[i]);
    }

    printf("Resposta: ");
    /* espaco antes do %c ignora o Enter pendente; valida retorno e letra A-D */
    if (scanf(" %c", &alternativa) != 1 ||
        toupper((unsigned char)alternativa) < 'A' ||
        toupper((unsigned char)alternativa) > 'D')
    {
        printf("Resposta invalida. Use A, B, C ou D.\n");
        limparBuffer();
        liberarCards(vetorResultado);
        liberarCards(carta);
        return;
    }

    ConfereResposta(alternativa, &cartaEscolhida);
    ConsequenciaEscolha(alternativa, &cartaEscolhida);

    liberarCards(vetorResultado);
    liberarCards(carta);
}