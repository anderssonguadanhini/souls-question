#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "raylib.h"
#include "interface.h"

#define LARGURA_TELA 1200
#define ALTURA_TELA 720

typedef enum
{
    TELA_MENU = 0,
    TELA_TORRE,
    TELA_DESAFIO,
    TELA_VITORIA,
    TELA_DERROTA
} EstadoTela;

typedef enum
{
    ITEM_INIMIGO = 0,
    ITEM_BAU_SOMA,
    ITEM_BAU_MULT,
    ITEM_BOSS
} TipoNo;

typedef struct
{
    Vector2 pos;      // Posição na arena isométrica
    Rectangle hitbox; // Área clicável do mouse
    TipoNo tipo;
    int poder;         // Poder do inimigo ou bônus (+10, x2)
    int nivelPergunta; // 1 (Muito Fácil) a 5 (Muito Difícil)
    int andar;         // 1, 2 ou 3
    int concluido;
    const char *nome;
} NoArena;

// ============================================================================
// FUNÇÕES AUXILIARES DE TEXTO E DESENHO (ZERO WARNINGS)
// ============================================================================

static void desenharTextoQuebrado(const char *texto, int x, int y, int larguraMax, int tamFonte, Color cor)
{
    char copia[512];
    snprintf(copia, sizeof(copia), "%s", texto);

    char linha[512] = "";
    int posY = y;

    char *palavra = strtok(copia, " ");
    while (palavra != NULL)
    {
        char teste[1024];
        if (linha[0] == '\0')
        {
            snprintf(teste, sizeof(teste), "%s", palavra);
        }
        else
        {
            snprintf(teste, sizeof(teste), "%s %s", linha, palavra);
        }

        if (MeasureText(teste, tamFonte) > larguraMax && linha[0] != '\0')
        {
            DrawText(linha, x, posY, tamFonte, cor);
            posY += tamFonte + 8;
            snprintf(linha, sizeof(linha), "%s", palavra);
        }
        else
        {
            snprintf(linha, sizeof(linha), "%.*s", (int)sizeof(linha) - 1, teste);
        }
        palavra = strtok(NULL, " ");
    }

    if (linha[0] != '\0')
    {
        DrawText(linha, x, posY, tamFonte, cor);
    }
}

static int desenharBotao(Rectangle ret, const char *texto, int tamFonte, Color normal, Color hover, Color borda)
{
    Vector2 mouse = GetMousePosition();
    int emCima = CheckCollisionPointRec(mouse, ret);

    DrawRectangleRounded((Rectangle){ret.x + 4, ret.y + 5, ret.width, ret.height}, 0.25f, 8, (Color){8, 8, 12, 180});
    DrawRectangleRounded(ret, 0.25f, 8, emCima ? hover : normal);
    DrawRectangleRoundedLinesEx(ret, 0.25f, 8, 2.0f, emCima ? WHITE : borda);

    int largTexto = MeasureText(texto, tamFonte);
    DrawText(texto,
             (int)(ret.x + (ret.width - largTexto) / 2.0f),
             (int)(ret.y + (ret.height - tamFonte) / 2.0f),
             tamFonte, RAYWHITE);

    return emCima && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

static void desenharPersonagem(Vector2 pos, int poder, Color corCorpo, Color corBadge, float pulso)
{
    DrawEllipse((int)pos.x, (int)pos.y + 8, 24, 11, (Color){0, 0, 0, 150});

    DrawCircle((int)pos.x, (int)(pos.y - 14 + pulso), 16, corCorpo);
    DrawCircleLines((int)pos.x, (int)(pos.y - 14 + pulso), 16, WHITE);
    DrawCircle((int)pos.x, (int)(pos.y - 34 + pulso), 11, RAYWHITE);
    DrawCircleLines((int)pos.x, (int)(pos.y - 34 + pulso), 11, corCorpo);

    const char *txt = TextFormat("%d", poder);
    int larg = MeasureText(txt, 22);
    Rectangle badge = {pos.x - (larg / 2.0f) - 10, pos.y - 76 + pulso, (float)larg + 20, 28};
    DrawRectangleRounded(badge, 0.4f, 6, (Color){12, 12, 18, 235});
    DrawRectangleRoundedLinesEx(badge, 0.4f, 6, 2.0f, corBadge);
    DrawText(txt, (int)(badge.x + 10), (int)(badge.y + 3), 22, corBadge);
}

static void desenharBau(Vector2 pos, int valor, int ehMult, int bloqueado, float pulso)
{
    Color corBau = bloqueado ? DARKGRAY : (ehMult ? PURPLE : GOLD);
    Color corTxt = bloqueado ? GRAY : (ehMult ? VIOLET : YELLOW);

    DrawEllipse((int)pos.x, (int)pos.y + 8, 24, 11, (Color){0, 0, 0, 150});

    Rectangle caixa = {pos.x - 20, pos.y - 26 + pulso, 40, 28};
    DrawRectangleRounded(caixa, 0.2f, 6, corBau);
    DrawRectangleRoundedLinesEx(caixa, 0.2f, 6, 2.0f, bloqueado ? GRAY : WHITE);
    DrawLineEx((Vector2){caixa.x, caixa.y + 11}, (Vector2){caixa.x + 40, caixa.y + 11}, 3.0f, BLACK);

    const char *txt = ehMult ? TextFormat("x%d", valor) : TextFormat("+%d", valor);
    int larg = MeasureText(txt, 22);
    Rectangle badge = {pos.x - (larg / 2.0f) - 10, pos.y - 64 + pulso, (float)larg + 20, 28};
    DrawRectangleRounded(badge, 0.4f, 6, (Color){12, 12, 18, 235});
    DrawRectangleRoundedLinesEx(badge, 0.4f, 6, 2.0f, corTxt);
    DrawText(txt, (int)(badge.x + 10), (int)(badge.y + 3), 22, corTxt);
}

// ============================================================================
// LÓGICA DO JOGO E SORTEIO DO BANCO
// ============================================================================

static int sortearPerguntaPorNivel(const Banco *banco, int nivel, int *usadas)
{
    if (banco->quantidade <= 0)
        return -1;

    int disp = 0;
    for (int i = 0; i < banco->quantidade; i++)
    {
        if (!usadas[i] && banco->dados[i].nivel == nivel)
        {
            disp++;
        }
    }

    if (disp == 0)
    {
        for (int i = 0; i < banco->quantidade; i++)
        {
            if (banco->dados[i].nivel == nivel)
            {
                usadas[i] = 0;
                disp++;
            }
        }
    }

    if (disp == 0)
        return rand() % banco->quantidade;

    int alvo = rand() % disp;
    for (int i = 0; i < banco->quantidade; i++)
    {
        if (!usadas[i] && banco->dados[i].nivel == nivel)
        {
            if (alvo == 0)
            {
                usadas[i] = 1;
                return i;
            }
            alvo--;
        }
    }
    return 0;
}

static void inicializarArena(NoArena nos[], int *qtdNos, int *poderHeroi, int *marcoSeguranca, Vector2 *posHeroi)
{
    *poderHeroi = 10;
    *marcoSeguranca = 10;
    *posHeroi = (Vector2){520.0f, 600.0f};

    NoArena layout[9] = {
        // ANDAR 1 (Degraus inferiores - Muito Fácil / Fácil)
        {{675.0f, 545.0f}, {625, 480, 100, 95}, ITEM_INIMIGO, 8, 1, 1, 0, "Morto-Vivo"},
        {{445.0f, 495.0f}, {395, 430, 100, 95}, ITEM_BAU_SOMA, 7, 1, 1, 0, "Estus +7"},
        {{770.0f, 455.0f}, {720, 390, 100, 95}, ITEM_INIMIGO, 20, 2, 1, 0, "Cavaleiro Negro"},
        {{430.0f, 385.0f}, {380, 320, 100, 95}, ITEM_BAU_SOMA, 15, 2, 1, 0, "Alma +15"},

        // ANDAR 2 (Pátio intermediário - Médio / Difícil - Marco de Segurança)
        {{755.0f, 355.0f}, {705, 290, 100, 95}, ITEM_INIMIGO, 45, 3, 2, 0, "Guardiao das Cinzas"},
        {{485.0f, 295.0f}, {435, 230, 100, 95}, ITEM_BAU_MULT, 2, 3, 2, 0, "Brasa x2"},
        {{705.0f, 265.0f}, {655, 200, 100, 95}, ITEM_INIMIGO, 160, 4, 2, 0, "Cavaleiro de Gwyn"},

        // ANDAR 3 (Altar superior - Muito Difícil / Boss Final e Tesouro 1 Milhão)
        {{595.0f, 210.0f}, {540, 140, 110, 100}, ITEM_BOSS, 320, 5, 3, 0, "Gwyn, Senhor das Cinzas"},
        {{595.0f, 125.0f}, {540, 65, 110, 90}, ITEM_BAU_SOMA, 1000000, 5, 3, 0, "Chama Final (1M)"}};

    *qtdNos = 9;
    for (int i = 0; i < 9; i++)
    {
        nos[i] = layout[i];
    }
}

// ============================================================================
// LOOP PRINCIPAL DA INTERFACE GRÁFICA (100% MOUSE)
// ============================================================================
void executarInterfaceGrafica(Banco *banco)
{
    InitWindow(LARGURA_TELA, ALTURA_TELA, "FLAME CHAOS - Kiln of the First Flame (Mouse)");
    SetTargetFPS(60);

    Texture2D texCenario = {0};
    int temCenario = 0;
    const char *caminhosPossiveis[4] = {
        "assets/cenario.png",
        "assets/cenario.jpg",
        "assets/cenario.jpeg",
        "../assets/cenario.png"};
    for (int i = 0; i < 4; i++)
    {
        if (FileExists(caminhosPossiveis[i]))
        {
            texCenario = LoadTexture(caminhosPossiveis[i]);
            if (texCenario.id > 0)
            {
                temCenario = 1;
                break;
            }
        }
    }

    EstadoTela tela = TELA_MENU;
    NoArena nos[12];
    int qtdNos = 0;
    int poderHeroi = 10;
    int marcoSeguranca = 10;
    Vector2 posHeroi = {520.0f, 600.0f};

    int indiceAlvo = -1;
    int indicePergunta = -1;
    char avisoInferior[180] = "Clique com o mouse em um alvo disponivel na arena!";
    Color corAviso = SKYBLUE;

    int *usadas = calloc((size_t)(banco->quantidade > 0 ? banco->quantidade : 1), sizeof(int));

    while (!WindowShouldClose())
    {
        Vector2 mouse = GetMousePosition();
        float tempo = (float)GetTime();
        float pulso = sinf(tempo * 4.0f) * 3.0f;

        BeginDrawing();
        ClearBackground((Color){16, 15, 22, 255});

        if (temCenario)
        {
            float escala = (float)ALTURA_TELA / (float)texCenario.height;
            float largDest = texCenario.width * escala;
            Rectangle src = {0, 0, (float)texCenario.width, (float)texCenario.height};
            Rectangle dst = {(LARGURA_TELA - largDest) / 2.0f, 0, largDest, (float)ALTURA_TELA};
            Color tintFundo = (tela == TELA_TORRE) ? WHITE : (Color){110, 110, 120, 255};
            DrawTexturePro(texCenario, src, dst, (Vector2){0, 0}, 0.0f, tintFundo);
        }

        // --------------------------------------------------------------------
        // 1. MENU INICIAL
        // --------------------------------------------------------------------
        if (tela == TELA_MENU)
        {
            DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){10, 10, 16, 175});

            DrawText("FLAME CHAOS", LARGURA_TELA / 2 - MeasureText("FLAME CHAOS", 68) / 2, 130, 68, GOLD);
            DrawText("Conquiste a Fornalha da Primeira Chama (100% Mouse)",
                     LARGURA_TELA / 2 - MeasureText("Conquiste a Fornalha da Primeira Chama (100% Mouse)", 22) / 2,
                     215, 22, LIGHTGRAY);

            DrawText(TextFormat("Banco carregado: %d perguntas prontas", banco->quantidade),
                     LARGURA_TELA / 2 - MeasureText(TextFormat("Banco carregado: %d perguntas prontas", banco->quantidade), 20) / 2,
                     260, 20, GREEN);

            Rectangle btnIniciar = {LARGURA_TELA / 2.0f - 170, 330, 340, 68};
            Rectangle btnSair = {LARGURA_TELA / 2.0f - 170, 420, 340, 68};

            if (desenharBotao(btnIniciar, "INICIAR JORNADA", 26, (Color){150, 35, 25, 255}, (Color){210, 55, 35, 255}, GOLD))
            {
                inicializarArena(nos, &qtdNos, &poderHeroi, &marcoSeguranca, &posHeroi);
                snprintf(avisoInferior, sizeof(avisoInferior), "Clique em um inimigo mais fraco ou bau nos degraus inferiores!");
                corAviso = SKYBLUE;
                tela = TELA_TORRE;
            }

            if (desenharBotao(btnSair, "VOLTAR AO TERMINAL", 22, (Color){45, 45, 58, 255}, (Color){75, 75, 95, 255}, GRAY))
            {
                break;
            }
        }

        // --------------------------------------------------------------------
        // 2. ARENA ISOMÉTRICA (PUZZLE DE PODER + CLIQUE DO MOUSE)
        // --------------------------------------------------------------------
        else if (tela == TELA_TORRE)
        {
            DrawRectangle(20, 20, 300, 170, (Color){14, 13, 20, 225});
            DrawRectangleLinesEx((Rectangle){20, 20, 300, 170}, 2.0f, GOLD);

            DrawText("STATUS DO GUERREIRO", 38, 34, 18, GOLD);
            DrawText(TextFormat("PODER: %d", poderHeroi), 38, 68, 30, SKYBLUE);
            DrawText(TextFormat("MARCO SEGURO: %d", marcoSeguranca), 38, 110, 20, GREEN);

            Rectangle btnMenu = {38, 142, 130, 34};
            if (desenharBotao(btnMenu, "MENU", 18, DARKGRAY, GRAY, WHITE))
            {
                tela = TELA_MENU;
            }

            if (!temCenario)
            {
                DrawPoly((Vector2){600, 360}, 4, 290, 0.0f, (Color){52, 48, 58, 255});
                DrawPolyLinesEx((Vector2){600, 360}, 4, 290, 0.0f, 3.0f, ORANGE);
            }

            int restaAndar1 = 0, restaAndar2 = 0;
            for (int i = 0; i < qtdNos; i++)
            {
                if (!nos[i].concluido && nos[i].andar == 1)
                    restaAndar1++;
                if (!nos[i].concluido && nos[i].andar == 2)
                    restaAndar2++;
            }

            for (int i = 0; i < qtdNos; i++)
            {
                if (nos[i].concluido)
                    continue;

                int bloqueado = (nos[i].andar == 2 && restaAndar1 > 0) ||
                                (nos[i].andar == 3 && restaAndar2 > 0);
                int hover = (!bloqueado && CheckCollisionPointRec(mouse, nos[i].hitbox));

                if (hover)
                {
                    DrawCircleLines((int)nos[i].pos.x, (int)nos[i].pos.y + 6, 34, GOLD);
                }

                if (nos[i].tipo == ITEM_INIMIGO || nos[i].tipo == ITEM_BOSS)
                {
                    Color cCorpo = bloqueado ? DARKGRAY : (nos[i].tipo == ITEM_BOSS ? ORANGE : RED);
                    Color cBadge = bloqueado ? GRAY : (hover ? YELLOW : RED);
                    desenharPersonagem(nos[i].pos, nos[i].poder, cCorpo, cBadge, hover ? pulso : 0.0f);
                }
                else
                {
                    desenharBau(nos[i].pos, nos[i].poder, (nos[i].tipo == ITEM_BAU_MULT), bloqueado, hover ? pulso : 0.0f);
                }

                int largNome = MeasureText(nos[i].nome, 14);
                DrawRectangle((int)(nos[i].pos.x - largNome / 2.0f - 5), (int)nos[i].pos.y + 18, largNome + 10, 18, (Color){10, 10, 15, 200});
                DrawText(nos[i].nome, (int)(nos[i].pos.x - largNome / 2.0f), (int)nos[i].pos.y + 20, 14, bloqueado ? GRAY : WHITE);

                if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    if ((nos[i].tipo == ITEM_INIMIGO || nos[i].tipo == ITEM_BOSS) && poderHeroi <= nos[i].poder)
                    {
                        snprintf(avisoInferior, sizeof(avisoInferior),
                                 "PODER INSUFICIENTE! Voce tem %d e '%s' tem %d. Colete almas/baus antes!",
                                 poderHeroi, nos[i].nome, nos[i].poder);
                        corAviso = RED;
                    }
                    else
                    {
                        indiceAlvo = i;
                        indicePergunta = sortearPerguntaPorNivel(banco, nos[i].nivelPergunta, usadas);
                        tela = TELA_DESAFIO;
                    }
                }
            }

            desenharPersonagem(posHeroi, poderHeroi, (Color){30, 135, 240, 255}, SKYBLUE, pulso);

            DrawRectangle(0, ALTURA_TELA - 50, LARGURA_TELA, 50, (Color){12, 11, 18, 235});
            DrawText(avisoInferior, 25, ALTURA_TELA - 34, 20, corAviso);
        }

        // --------------------------------------------------------------------
        // 3. TELA DE PERGUNTA (100% CLIQUE DO MOUSE NAS ALTERNATIVAS)
        // --------------------------------------------------------------------
        else if (tela == TELA_DESAFIO && indicePergunta >= 0)
        {
            const Perguntas *p = &banco->dados[indicePergunta];
            const NoArena *alvo = &nos[indiceAlvo];

            DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){0, 0, 0, 160});

            Rectangle modal = {130, 55, LARGURA_TELA - 260, ALTURA_TELA - 110};
            DrawRectangleRounded(modal, 0.06f, 10, (Color){24, 22, 34, 248});
            DrawRectangleRoundedLinesEx(modal, 0.06f, 10, 3.0f, GOLD);

            DrawText(TextFormat("ALVO: %s  |  DIFICULDADE: %s  |  UNIVERSO: %s",
                                alvo->nome, textoDoNivel(p->nivel), p->jogo),
                     165, 80, 20, GOLD);

            desenharTextoQuebrado(p->enunciado, 165, 125, LARGURA_TELA - 330, 24, WHITE);

            char letras[4] = {'A', 'B', 'C', 'D'};
            for (int j = 0; j < 4; j++)
            {
                Rectangle btnAlt = {165, (float)(275 + j * 80), LARGURA_TELA - 330, 64};
                char rotulo[320];
                snprintf(rotulo, sizeof(rotulo), "[%c]  %s", letras[j], p->alternativa[j]);

                if (desenharBotao(btnAlt, rotulo, 20, (Color){46, 44, 66, 255}, (Color){80, 75, 115, 255}, SKYBLUE))
                {
                    char resp = letras[j];
                    char certa = (char)toupper((unsigned char)p->correta);

                    if (resp == certa)
                    {
                        nos[indiceAlvo].concluido = 1;
                        posHeroi = nos[indiceAlvo].pos;

                        if (alvo->tipo == ITEM_INIMIGO || alvo->tipo == ITEM_BOSS)
                        {
                            poderHeroi += alvo->poder;
                            snprintf(avisoInferior, sizeof(avisoInferior),
                                     "INIMIGO DERROTADO! Voce absorveu +%d de poder de %s!", alvo->poder, alvo->nome);
                        }
                        else if (alvo->tipo == ITEM_BAU_SOMA)
                        {
                            poderHeroi += alvo->poder;
                            snprintf(avisoInferior, sizeof(avisoInferior),
                                     "TESOURO COLETADO! Voce ganhou +%d de poder!", alvo->poder);
                        }
                        else if (alvo->tipo == ITEM_BAU_MULT)
                        {
                            poderHeroi *= alvo->poder;
                            snprintf(avisoInferior, sizeof(avisoInferior),
                                     "BRASA ATIVADA! Seu poder foi multiplicado por x%d!", alvo->poder);
                        }

                        if (alvo->andar == 2)
                        {
                            marcoSeguranca = poderHeroi;
                        }

                        corAviso = GREEN;
                        tela = (indiceAlvo == qtdNos - 1) ? TELA_VITORIA : TELA_TORRE;
                    }
                    else
                    {
                        tela = TELA_DERROTA;
                    }
                }
            }
        }

        // --------------------------------------------------------------------
        // 4. TELA DE VITÓRIA
        // --------------------------------------------------------------------
        else if (tela == TELA_VITORIA)
        {
            DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){10, 10, 15, 210});
            DrawText("PRIMEIRA CHAMA ACESA!", LARGURA_TELA / 2 - MeasureText("PRIMEIRA CHAMA ACESA!", 56) / 2, 170, 56, GOLD);

            const char *resumo = TextFormat("Voce derrotou Gwyn e conquistou o Trono com Poder: %d!", poderHeroi);
            DrawText(resumo, LARGURA_TELA / 2 - MeasureText(resumo, 24) / 2, 260, 24, GREEN);

            Rectangle btnReplay = {LARGURA_TELA / 2.0f - 170, 360, 340, 66};
            if (desenharBotao(btnReplay, "NOVA JORNADA (NG+)", 24, DARKGREEN, GREEN, WHITE))
            {
                inicializarArena(nos, &qtdNos, &poderHeroi, &marcoSeguranca, &posHeroi);
                tela = TELA_TORRE;
            }
        }

        // --------------------------------------------------------------------
        // 5. TELA DE DERROTA (YOU DIED)
        // --------------------------------------------------------------------
        else if (tela == TELA_DERROTA)
        {
            DrawRectangle(0, 0, LARGURA_TELA, ALTURA_TELA, (Color){15, 5, 5, 220});
            DrawText("YOU DIED", LARGURA_TELA / 2 - MeasureText("YOU DIED", 76) / 2, 170, 76, RED);
            DrawText("Voce errou o desafio e sucumbiu na Fornalha!",
                     LARGURA_TELA / 2 - MeasureText("Voce errou o desafio e sucumbiu na Fornalha!", 24) / 2,
                     270, 24, LIGHTGRAY);

            Rectangle btnTentar = {LARGURA_TELA / 2.0f - 170, 360, 340, 66};
            if (desenharBotao(btnTentar, "RENASCER NA FOGUEIRA", 22, (Color){145, 30, 30, 255}, RED, GOLD))
            {
                inicializarArena(nos, &qtdNos, &poderHeroi, &marcoSeguranca, &posHeroi);
                tela = TELA_TORRE;
            }
        }

        EndDrawing();
    }

    if (temCenario)
    {
        UnloadTexture(texCenario);
    }
    free(usadas);
    CloseWindow();
}