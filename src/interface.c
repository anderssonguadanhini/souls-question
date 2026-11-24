#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "raylib.h"
#include "interface.h"

#define MAX_INIMIGOS 16
#define MAX_ITENS 12
#define MAX_POPUPS 12
#define PODER_MAX_INIMIGO 9999999
#define PODER_INICIAL_HEROI 5

typedef enum
{
    TIPO_ITEM_BAU = 0,
    TIPO_ITEM_ARMA_MULT
} TipoItem;

typedef struct
{
    Vector2 pos;
    Vector2 destino;
    int movendo;
    int direcaoX; // 1 = direita, -1 = esquerda
    float velocidade;
    int inimigoAlvoIdx; // >= 0 se estiver indo atacar um inimigo
    int itemAlvoIdx;    // >= 0 se estiver indo pegar um item
    float timerAtaque;  // Animação de golpe ao chegar no inimigo
    float timerRecuo;   // Animação de dano caso perca o combate
    float timerLevelUp; // Efeito visual de evolução ao pegar item / upar
    int poder;
    int nivelArma;       // Quantidade de itens coletados
    int estagioEvolucao; // 0: hero, 1: hero_espada1, 2: hero_espada2, 3: hero_final
    int derrotouSif;     // 1 ao derrotar Sif (libera forma final)
    int vivo;
    Texture2D texHeroi;
    int temTexHeroi;
    Texture2D texHeroiEspada1;
    int temTexHeroiEspada1;
    Texture2D texHeroiEspada2;
    int temTexHeroiEspada2;
    Texture2D texHeroiFinal;
    int temTexHeroiFinal;
} Heroi;

typedef struct
{
    int id;
    char nome[48];
    Vector2 pos;
    int poder;
    int nivelDificuldade;
    int derrotado;
    int ehChefe;
    Texture2D texInimigo;
    int temTexInimigo;
} Inimigo;

typedef struct
{
    int id;
    char nome[48];
    TipoItem tipo;
    Vector2 pos;
    int bonusPoder;    // Ex: +4, +10...
    int multiplicador; // Ex: x4, x100...
    int coletado;
    Texture2D texItem;
    int temTexItem;
} Item;

typedef struct
{
    Texture2D texCenario;
    int temCenario;
} Cenario;

typedef struct
{
    int limiarEspada1;
    int limiarEspada2;
    int limiarFormaFinal;
    Cenario cenario;
    Heroi heroi;
    Inimigo inimigos[MAX_INIMIGOS];
    int qtdInimigos;
    Item itens[MAX_ITENS];
    int qtdItens;
    int vitoriaArena;
} Arena;

typedef struct
{
    float escalaX;
    float escalaY;
    float passoY;
    float inclinacao;
    float sombraEscala;
} MovimentoCorpo;

typedef struct
{
    int ativo;
    char texto[48];
    Vector2 pos;
    Color cor;
    float tempoRestante;
} TextoFlutuante;

static TextoFlutuante popups[MAX_POPUPS] = {0};

static int clampIntMax(long long valor, int minimo, int maximo)
{
    if (valor < minimo)
        return minimo;
    if (valor > maximo)
        return maximo;
    return (int)valor;
}

static int sortearPoderAteMaximo(int minimo, int maximo)
{
    if (maximo <= minimo)
        return minimo;
    unsigned int r1 = (unsigned int)(rand() & 0xFFFF);
    unsigned int r2 = (unsigned int)(rand() & 0xFFFF);
    unsigned int combinado = (r1 << 16) | r2;
    unsigned int intervalo = (unsigned int)(maximo - minimo + 1);
    return minimo + (int)(combinado % intervalo);
}

static Vector2 sortearPosicaoLivre(Vector2 ocupados[], int *qtdOcupados, float largTela, float altTela, float distMinima)
{
    Vector2 candidata = {largTela * 0.5f, altTela * 0.55f};

    for (int tentativa = 0; tentativa < 80; tentativa++)
    {
        float rx = (float)GetRandomValue(16, 84) / 100.0f;
        float ry = (float)GetRandomValue(34, 80) / 100.0f;
        candidata = (Vector2){largTela * rx, altTela * ry};

        int valida = 1;
        for (int i = 0; i < *qtdOcupados; i++)
        {
            float dx = candidata.x - ocupados[i].x;
            float dy = candidata.y - ocupados[i].y;
            if (sqrtf(dx * dx + dy * dy) < distMinima)
            {
                valida = 0;
                break;
            }
        }

        if (valida)
            break;
    }

    if (*qtdOcupados < 32)
    {
        ocupados[*qtdOcupados] = candidata;
        (*qtdOcupados)++;
    }
    return candidata;
}

static void adicionarPopup(const char *texto, Vector2 pos, Color cor)
{
    for (int i = 0; i < MAX_POPUPS; i++)
    {
        if (!popups[i].ativo)
        {
            popups[i].ativo = 1;
            snprintf(popups[i].texto, sizeof(popups[i].texto), "%s", texto);
            popups[i].pos = pos;
            popups[i].cor = cor;
            popups[i].tempoRestante = 1.5f;
            return;
        }
    }
}

static void atualizarEDesenharPopups(float dt)
{
    for (int i = 0; i < MAX_POPUPS; i++)
    {
        if (!popups[i].ativo)
            continue;

        popups[i].tempoRestante -= dt;
        popups[i].pos.y -= 38.0f * dt;

        if (popups[i].tempoRestante <= 0.0f)
        {
            popups[i].ativo = 0;
            continue;
        }

        unsigned char alpha = (popups[i].tempoRestante < 0.45f)
                                  ? (unsigned char)((popups[i].tempoRestante / 0.45f) * 255.0f)
                                  : 255;

        Color c = popups[i].cor;
        c.a = alpha;
        Color sombra = {0, 0, 0, alpha};

        int larg = MeasureText(popups[i].texto, 26);
        int x = (int)(popups[i].pos.x - larg / 2.0f);
        int y = (int)popups[i].pos.y;

        DrawText(popups[i].texto, x + 2, y + 2, 26, sombra);
        DrawText(popups[i].texto, x, y, 26, c);
    }
}

// Carrega o sprite removendo o fundo preto puro (ou branco se o canto for claro)
static Texture2D carregarSpriteRecortado(const char *caminho, int *carregado)
{
    *carregado = 0;
    Texture2D tex = {0};
    if (!FileExists(caminho))
        return tex;

    Image img = LoadImage(caminho);
    if (img.data == NULL)
        return tex;

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *pixels = LoadImageColors(img);
    int total = img.width * img.height;

    int fundoBranco = (total > 0 && pixels[0].a > 200 && pixels[0].r > 240 && pixels[0].g > 240 && pixels[0].b > 240);

    for (int i = 0; i < total; i++)
    {
        if (fundoBranco)
        {
            if (pixels[i].r > 240 && pixels[i].g > 240 && pixels[i].b > 240)
            {
                pixels[i].a = 0;
            }
        }
        else
        {
            if (pixels[i].r < 14 && pixels[i].g < 14 && pixels[i].b < 14)
            {
                pixels[i].a = 0;
            }
        }
    }

    UnloadImage(img);
    img.data = pixels;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    img.mipmaps = 1;

    tex = LoadTextureFromImage(img);
    UnloadImageColors(pixels);

    if (tex.id > 0)
    {
        *carregado = 1;
    }
    return tex;
}

// Define o movimento corporal do personagem
static MovimentoCorpo calcularMovimentoPersonagem(int ehHeroi, int movendo, int direcaoX, float faseTempo)
{
    MovimentoCorpo mov = {1.0f, 1.0f, 0.0f, 0.0f, 1.0f};
    float t = (float)GetTime() + faseTempo;

    if (movendo)
    {
        float cicloPasso = fabsf(sinf(t * 14.0f));
        float balanco = sinf(t * 14.0f);

        mov.passoY = -cicloPasso * 8.0f;
        mov.escalaY = 1.0f + (cicloPasso * 0.035f);
        mov.escalaX = 1.0f - (cicloPasso * 0.025f);
        mov.inclinacao = (direcaoX * 4.5f) + (balanco * 2.2f);
        mov.sombraEscala = 1.0f - (cicloPasso * 0.14f);
    }
    else
    {
        float resp = sinf(t * (ehHeroi ? 3.2f : 2.6f));
        mov.passoY = 0.0f;
        mov.escalaY = 1.0f + (resp * 0.024f);
        mov.escalaX = 1.0f - (resp * 0.012f);
        mov.inclinacao = ehHeroi ? (sinf(t * 1.6f) * 1.2f) : (sinf(t * 2.1f) * 1.5f);
        mov.sombraEscala = 1.0f;
    }

    return mov;
}

// Avalia e atualiza o estágio visual do herói (0 -> 1 -> 2 -> 3)
// Forma Suprema (3) ativa ao derrotar o Sif (unindo-se a ele) ou atingir o poder necessário antes de enfrentar o Gwyn
static void atualizarEvolucaoHeroi(Arena *arena, int forcarEspada2)
{
    Heroi *heroi = &arena->heroi;
    int novoEstagio = heroi->estagioEvolucao;

    if (heroi->derrotouSif || heroi->poder >= arena->limiarFormaFinal)
    {
        novoEstagio = 3; // hero_final.png (Forma Suprema)
    }
    else if (forcarEspada2 || heroi->nivelArma >= 2 || heroi->poder >= arena->limiarEspada2)
    {
        if (novoEstagio < 2)
            novoEstagio = 2; // hero_espada2.png
    }
    else if (heroi->nivelArma >= 1 || heroi->poder >= arena->limiarEspada1)
    {
        if (novoEstagio < 1)
            novoEstagio = 1; // hero_espada1.png
    }

    if (novoEstagio > heroi->estagioEvolucao)
    {
        heroi->estagioEvolucao = novoEstagio;
        heroi->timerLevelUp = 1.05f;

        if (novoEstagio == 3)
        {
            adicionarPopup("FORMA SUPREMA! ARTORIAS & SIF",
                           (Vector2){heroi->pos.x, heroi->pos.y - 205.0f},
                           (Color){255, 215, 70, 255});
        }
        else if (novoEstagio == 2)
        {
            adicionarPopup("EVOLUCAO II! ESPADA DO ABISMO",
                           (Vector2){heroi->pos.x, heroi->pos.y - 190.0f},
                           (Color){130, 220, 255, 255});
        }
        else if (novoEstagio == 1)
        {
            adicionarPopup("EVOLUCAO I! ESPADA EQUIPADA",
                           (Vector2){heroi->pos.x, heroi->pos.y - 185.0f},
                           (Color){115, 235, 150, 255});
        }
    }
}

// Aplica o efeito do item coletado e evolui o herói
static void coletarItem(Arena *arena, int idxItem)
{
    Item *it = &arena->itens[idxItem];
    if (it->coletado)
        return;

    it->coletado = 1;
    arena->heroi.timerLevelUp = 0.85f;
    arena->heroi.nivelArma++;

    if (it->tipo == TIPO_ITEM_ARMA_MULT)
    {
        long long novoPoder = (long long)arena->heroi.poder * (long long)it->multiplicador;
        arena->heroi.poder = clampIntMax(novoPoder, 1, PODER_MAX_INIMIGO * 2);
        adicionarPopup(TextFormat("PODER x%d!", it->multiplicador),
                       (Vector2){arena->heroi.pos.x, arena->heroi.pos.y - 170.0f},
                       (Color){255, 220, 70, 255});
        atualizarEvolucaoHeroi(arena, 1);
    }
    else
    {
        long long novoPoder = (long long)arena->heroi.poder + (long long)it->bonusPoder;
        arena->heroi.poder = clampIntMax(novoPoder, 1, PODER_MAX_INIMIGO * 2);
        adicionarPopup(TextFormat("+%d PODER!", it->bonusPoder),
                       (Vector2){arena->heroi.pos.x, arena->heroi.pos.y - 170.0f},
                       (Color){100, 235, 130, 255});
        atualizarEvolucaoHeroi(arena, 0);
    }
}

// Resolve o combate entre o herói e o inimigo alvo
static void executarCombate(Arena *arena, int idxInimigo)
{
    Inimigo *ini = &arena->inimigos[idxInimigo];
    if (ini->derrotado)
        return;

    arena->heroi.timerAtaque = 0.30f;

    if (arena->heroi.poder >= ini->poder)
    {
        int ganho = ini->poder;
        ini->derrotado = 1;
        long long novoPoder = (long long)arena->heroi.poder + (long long)ganho;
        arena->heroi.poder = clampIntMax(novoPoder, 1, PODER_MAX_INIMIGO * 2);

        adicionarPopup(TextFormat("+%d", ganho),
                       (Vector2){ini->pos.x, ini->pos.y - 150.0f},
                       (Color){90, 200, 255, 255});

        if (strcmp(ini->nome, "Sif") == 0)
        {
            arena->heroi.derrotouSif = 1;
        }

        atualizarEvolucaoHeroi(arena, 0);

        int restantes = 0;
        for (int i = 0; i < arena->qtdInimigos; i++)
        {
            if (!arena->inimigos[i].derrotado)
                restantes++;
        }
        if (restantes == 0)
        {
            arena->vitoriaArena = 1;
        }
    }
    else
    {
        arena->heroi.timerRecuo = 0.45f;
        arena->heroi.pos.x -= arena->heroi.direcaoX * 42.0f;
        arena->heroi.destino = arena->heroi.pos;

        adicionarPopup("PODER INSUFICIENTE!",
                       (Vector2){arena->heroi.pos.x, arena->heroi.pos.y - 170.0f},
                       (Color){255, 75, 75, 255});
    }
}

// Verifica clique em itens ou inimigos no cenário
static void processarCliqueCenario(Arena *arena, Vector2 clique)
{
    if (!arena->heroi.vivo)
        return;

    // 1. Verifica clique nos itens
    for (int i = 0; i < arena->qtdItens; i++)
    {
        Item *it = &arena->itens[i];
        if (it->coletado)
            continue;

        Rectangle hitboxItem = {it->pos.x - 45.0f, it->pos.y - 95.0f, 90.0f, 105.0f};
        if (CheckCollisionPointRec(clique, hitboxItem))
        {
            float lado = (arena->heroi.pos.x <= it->pos.x) ? -48.0f : 48.0f;
            arena->heroi.destino = (Vector2){it->pos.x + lado, it->pos.y};
            arena->heroi.direcaoX = (arena->heroi.destino.x >= arena->heroi.pos.x) ? 1 : -1;
            arena->heroi.movendo = 1;
            arena->heroi.itemAlvoIdx = i;
            arena->heroi.inimigoAlvoIdx = -1;
            return;
        }
    }

    // 2. Verifica clique nos inimigos
    for (int i = 0; i < arena->qtdInimigos; i++)
    {
        Inimigo *ini = &arena->inimigos[i];
        if (ini->derrotado)
            continue;

        float largHit = ini->ehChefe ? 140.0f : 110.0f;
        float altHit = ini->ehChefe ? 175.0f : 155.0f;
        Rectangle hitboxIni = {ini->pos.x - largHit / 2.0f, ini->pos.y - altHit + 10.0f, largHit, altHit};

        if (CheckCollisionPointRec(clique, hitboxIni))
        {
            float distanciaCombate = ini->ehChefe ? 82.0f : 68.0f;
            float lado = (arena->heroi.pos.x <= ini->pos.x) ? -distanciaCombate : distanciaCombate;
            arena->heroi.destino = (Vector2){ini->pos.x + lado, ini->pos.y};
            arena->heroi.direcaoX = (arena->heroi.destino.x >= arena->heroi.pos.x) ? 1 : -1;
            arena->heroi.movendo = 1;
            arena->heroi.inimigoAlvoIdx = i;
            arena->heroi.itemAlvoIdx = -1;
            return;
        }
    }
}

// Atualiza a caminhada do herói e dispara coleta ou combate ao chegar no alvo
static void atualizarHeroi(Arena *arena, float dt)
{
    Heroi *heroi = &arena->heroi;

    if (heroi->timerAtaque > 0.0f)
        heroi->timerAtaque -= dt;
    if (heroi->timerRecuo > 0.0f)
        heroi->timerRecuo -= dt;
    if (heroi->timerLevelUp > 0.0f)
        heroi->timerLevelUp -= dt;

    if (!heroi->movendo)
        return;

    float dx = heroi->destino.x - heroi->pos.x;
    float dy = heroi->destino.y - heroi->pos.y;
    float dist = sqrtf(dx * dx + dy * dy);

    if (fabsf(dx) > 1.0f)
    {
        heroi->direcaoX = (dx >= 0.0f) ? 1 : -1;
    }

    if (dist <= 5.0f)
    {
        heroi->pos = heroi->destino;
        heroi->movendo = 0;

        if (heroi->itemAlvoIdx >= 0)
        {
            int idx = heroi->itemAlvoIdx;
            heroi->itemAlvoIdx = -1;
            coletarItem(arena, idx);
        }
        else if (heroi->inimigoAlvoIdx >= 0)
        {
            int idx = heroi->inimigoAlvoIdx;
            heroi->inimigoAlvoIdx = -1;
            heroi->direcaoX = (arena->inimigos[idx].pos.x >= heroi->pos.x) ? 1 : -1;
            executarCombate(arena, idx);
        }
    }
    else
    {
        float passo = heroi->velocidade * dt;
        if (passo > dist)
            passo = dist;
        heroi->pos.x += (dx / dist) * passo;
        heroi->pos.y += (dy / dist) * passo;
    }
}

static void desenharItem(const Item *it, int selecionado)
{
    if (it->coletado)
        return;

    float t = (float)GetTime() + it->id * 0.9f;
    float flutuar = sinf(t * 3.2f) * 4.0f;

    if (selecionado)
    {
        DrawEllipseLines((int)it->pos.x, (int)it->pos.y, 34, 12, (Color){255, 225, 90, 230});
    }

    DrawEllipse((int)it->pos.x, (int)it->pos.y, 26, 9, (Color){0, 0, 0, 185});

    float topoY = it->pos.y - 68.0f;

    if (it->temTexItem)
    {
        float escala = 85.0f / (float)it->texItem.height;
        float larg = it->texItem.width * escala;
        float alt = it->texItem.height * escala;
        float offsetY = (it->tipo == TIPO_ITEM_ARMA_MULT) ? flutuar : 0.0f;

        Rectangle src = {0.0f, 0.0f, (float)it->texItem.width, (float)it->texItem.height};
        Rectangle dst = {it->pos.x, it->pos.y + offsetY, larg, alt};
        DrawTexturePro(it->texItem, src, dst, (Vector2){larg / 2.0f, alt}, 0.0f, WHITE);
        topoY = it->pos.y - alt + offsetY;
    }
    else if (it->tipo == TIPO_ITEM_ARMA_MULT)
    {
        float cy = it->pos.y - 44.0f + flutuar;
        DrawCircleGradient((int)it->pos.x, (int)cy, 38, (Color){255, 210, 60, 130}, (Color){255, 180, 0, 0});
        DrawLineEx((Vector2){it->pos.x - 16, cy + 20}, (Vector2){it->pos.x + 18, cy - 24}, 5.0f, (Color){255, 235, 130, 255});
        DrawLineEx((Vector2){it->pos.x - 18, cy + 8}, (Vector2){it->pos.x - 4, cy + 20}, 4.0f, (Color){210, 150, 40, 255});
        topoY = cy - 32.0f;
    }
    else
    {
        float bx = it->pos.x - 24.0f;
        float by = it->pos.y - 38.0f;
        DrawRectangleRounded((Rectangle){bx, by, 48, 36}, 0.2f, 6, (Color){125, 70, 25, 255});
        DrawRectangleRoundedLines((Rectangle){bx, by, 48, 36}, 0.2f, 6, (Color){245, 195, 65, 255});
        DrawRectangle((int)bx, (int)(by + 13), 48, 4, (Color){245, 195, 65, 255});
        DrawRectangle((int)(it->pos.x - 5), (int)(by + 10), 10, 10, (Color){255, 230, 110, 255});
        topoY = by - 6.0f;
    }

    const char *txt = (it->tipo == TIPO_ITEM_ARMA_MULT)
                          ? TextFormat("x%d", it->multiplicador)
                          : TextFormat("+%d", it->bonusPoder);

    Color corTexto = (it->tipo == TIPO_ITEM_ARMA_MULT)
                         ? (Color){255, 225, 80, 255}
                         : (Color){115, 195, 255, 255};

    int tamFonte = 24;
    int largTxt = MeasureText(txt, tamFonte);
    int tx = (int)(it->pos.x - largTxt / 2.0f);
    int ty = (int)topoY - 24;

    DrawText(txt, tx + 2, ty + 2, tamFonte, BLACK);
    DrawText(txt, tx - 1, ty - 1, tamFonte, BLACK);
    DrawText(txt, tx, ty, tamFonte, corTexto);
}

static void desenharHeroi(const Heroi *heroi)
{
    MovimentoCorpo mov = calcularMovimentoPersonagem(1, heroi->movendo, heroi->direcaoX, 0.0f);

    float avancoGolpeX = 0.0f;
    if (heroi->timerAtaque > 0.0f)
    {
        float fator = sinf((heroi->timerAtaque / 0.30f) * PI);
        avancoGolpeX = heroi->direcaoX * fator * 26.0f;
        mov.inclinacao += heroi->direcaoX * fator * 14.0f;
    }

    // Aura de Level Up nos pés ao evoluir
    if (heroi->timerLevelUp > 0.0f)
    {
        float prog = 1.0f - (heroi->timerLevelUp / 1.05f);
        if (prog < 0.0f)
            prog = 0.0f;
        unsigned char alpha = (unsigned char)((1.0f - prog) * 235.0f);
        Color corAura = (heroi->estagioEvolucao >= 3) ? (Color){255, 215, 70, alpha} : (Color){110, 210, 255, alpha};
        DrawEllipseLines((int)(heroi->pos.x + avancoGolpeX), (int)heroi->pos.y,
                         (int)(36 + prog * 42), (int)(13 + prog * 16), corAura);
        DrawCircleGradient((int)(heroi->pos.x + avancoGolpeX), (int)(heroi->pos.y - 80.0f),
                           75.0f, (Color){corAura.r, corAura.g, corAura.b, (unsigned char)(alpha / 3)},
                           (Color){corAura.r, corAura.g, corAura.b, 0});
    }
    else if (heroi->estagioEvolucao >= 3)
    {
        DrawEllipseLines((int)(heroi->pos.x + avancoGolpeX), (int)heroi->pos.y,
                         46, 15, (Color){255, 210, 80, 175});
    }
    else if (heroi->estagioEvolucao >= 1)
    {
        DrawEllipseLines((int)(heroi->pos.x + avancoGolpeX), (int)heroi->pos.y,
                         35, 12, (Color){100, 200, 255, 155});
    }

    Color tint = (heroi->timerRecuo > 0.0f) ? (Color){255, 120, 120, 255} : WHITE;

    int raioSombra = (heroi->estagioEvolucao >= 3) ? 44 : 30;
    DrawEllipse((int)(heroi->pos.x + avancoGolpeX), (int)heroi->pos.y,
                (int)(raioSombra * mov.sombraEscala), (int)(11 * mov.sombraEscala), (Color){0, 0, 0, 195});

    float alturaAlvo = 155.0f;
    if (heroi->estagioEvolucao == 1)
        alturaAlvo = 164.0f;
    else if (heroi->estagioEvolucao == 2)
        alturaAlvo = 172.0f;
    else if (heroi->estagioEvolucao >= 3)
        alturaAlvo = 195.0f;

    float alturaTopo = heroi->pos.y - alturaAlvo + mov.passoY;

    // Seleciona o sprite correspondente ao estágio atual com fallback seguro
    Texture2D texAtiva = heroi->texHeroi;
    int temTexAtiva = heroi->temTexHeroi;

    if (heroi->estagioEvolucao >= 3 && heroi->temTexHeroiFinal)
    {
        texAtiva = heroi->texHeroiFinal;
        temTexAtiva = 1;
    }
    else if (heroi->estagioEvolucao >= 2 && heroi->temTexHeroiEspada2)
    {
        texAtiva = heroi->texHeroiEspada2;
        temTexAtiva = 1;
    }
    else if (heroi->estagioEvolucao >= 1 && heroi->temTexHeroiEspada1)
    {
        texAtiva = heroi->texHeroiEspada1;
        temTexAtiva = 1;
    }
    else if (heroi->estagioEvolucao >= 1 && heroi->temTexHeroiEspada2)
    {
        texAtiva = heroi->texHeroiEspada2;
        temTexAtiva = 1;
    }

    if (temTexAtiva)
    {
        float escalaBase = alturaAlvo / (float)texAtiva.height;
        float larg = texAtiva.width * escalaBase * mov.escalaX;
        float alt = texAtiva.height * escalaBase * mov.escalaY;

        float srcLarg = (heroi->direcaoX < 0) ? -(float)texAtiva.width : (float)texAtiva.width;
        Rectangle src = {0.0f, 0.0f, srcLarg, (float)texAtiva.height};

        Rectangle dst = {heroi->pos.x + avancoGolpeX, heroi->pos.y + mov.passoY, larg, alt};
        Vector2 pivoPes = {larg / 2.0f, alt};

        DrawTexturePro(texAtiva, src, dst, pivoPes, mov.inclinacao, tint);

        alturaTopo = heroi->pos.y - alt + mov.passoY;
    }

    if (heroi->timerAtaque > 0.0f)
    {
        float fx = heroi->pos.x + avancoGolpeX + (heroi->direcaoX * 44.0f);
        float fy = heroi->pos.y - 78.0f;
        Color corCorte = (heroi->estagioEvolucao >= 2) ? (Color){255, 220, 95, 235} : (Color){120, 210, 255, 215};
        DrawCircleGradient((int)fx, (int)fy, (heroi->estagioEvolucao >= 2) ? 46 : 34, corCorte, (Color){80, 160, 255, 0});
    }

    // Número flutuante acima da cabeça
    const char *txtPoder = TextFormat("%d", heroi->poder);
    int tamFonte = (heroi->estagioEvolucao >= 2) ? 31 : 28;
    int largTxt = MeasureText(txtPoder, tamFonte);

    int txtX = (int)(heroi->pos.x + avancoGolpeX - largTxt / 2.0f);
    int txtY = (int)alturaTopo - 28;

    Color corNumero = (heroi->estagioEvolucao >= 3)
                          ? (Color){255, 225, 95, 255}
                          : ((heroi->estagioEvolucao >= 1) ? (Color){115, 225, 255, 255} : (Color){80, 180, 255, 255});
    DrawText(txtPoder, txtX + 2, txtY + 2, tamFonte, BLACK);
    DrawText(txtPoder, txtX - 1, txtY - 1, tamFonte, BLACK);
    DrawText(txtPoder, txtX, txtY, tamFonte, corNumero);
}

static void desenharInimigo(const Inimigo *ini, int selecionado)
{
    if (ini->derrotado)
        return;

    MovimentoCorpo mov = calcularMovimentoPersonagem(0, 0, -1, ini->id * 1.3f);

    if (selecionado)
    {
        DrawEllipseLines((int)ini->pos.x, (int)ini->pos.y, ini->ehChefe ? 46 : 36, 14, (Color){255, 70, 70, 230});
    }

    DrawEllipse((int)ini->pos.x, (int)ini->pos.y, ini->ehChefe ? 38 : 28, 10, (Color){0, 0, 0, 195});

    float alturaBase = ini->ehChefe ? 175.0f : 145.0f;
    float alturaTopo = ini->pos.y - alturaBase;

    if (ini->temTexInimigo)
    {
        float escalaBase = alturaBase / (float)ini->texInimigo.height;
        float larg = ini->texInimigo.width * escalaBase * mov.escalaX;
        float alt = ini->texInimigo.height * escalaBase * mov.escalaY;

        Rectangle src = {0.0f, 0.0f, (float)ini->texInimigo.width, (float)ini->texInimigo.height};
        Rectangle dst = {ini->pos.x, ini->pos.y, larg, alt};
        Vector2 pivoPes = {larg / 2.0f, alt};

        DrawTexturePro(ini->texInimigo, src, dst, pivoPes, mov.inclinacao, WHITE);

        alturaTopo = ini->pos.y - alt;
    }

    const char *txtPoder = TextFormat("%d", ini->poder);
    int tamFonte = 28;
    int largTxt = MeasureText(txtPoder, tamFonte);

    int txtX = (int)(ini->pos.x - largTxt / 2.0f);
    int txtY = (int)alturaTopo - 28;

    DrawText(txtPoder, txtX + 2, txtY + 2, tamFonte, BLACK);
    DrawText(txtPoder, txtX - 1, txtY - 1, tamFonte, BLACK);
    DrawText(txtPoder, txtX, txtY, tamFonte, (Color){255, 65, 65, 255});
}

// Gera inimigos aleatórios com poder até 9999999 (sem nível), enquanto o herói sempre começa em 5
static void gerarInimigosAleatorios(Arena *arena, float largTela, float altTela)
{
    arena->vitoriaArena = 0;

    int poderInicial = PODER_INICIAL_HEROI; // Herói sempre começa em 5

    // Sorteia o teto de poder do chefe final (Gwyn) entre 35 e 9999999
    int faixa = GetRandomValue(1, 4);
    int alvoGwyn = 60;
    if (faixa == 1)
        alvoGwyn = sortearPoderAteMaximo(35, 300);
    else if (faixa == 2)
        alvoGwyn = sortearPoderAteMaximo(301, 25000);
    else if (faixa == 3)
        alvoGwyn = sortearPoderAteMaximo(25001, 750000);
    else
        alvoGwyn = sortearPoderAteMaximo(750001, PODER_MAX_INIMIGO);

    // 1. Hollow Faca sempre vencível logo de início pelo herói (poder 1 a 5)
    int poderFaca = GetRandomValue(1, poderInicial);

    // 2. Baú 1 + Hollow Tocha (vencível após Hollow Faca + Baú 1)
    int bau1 = GetRandomValue(3, 8);
    long long poderAposFacaEBau1 = (long long)poderInicial + poderFaca + bau1;
    int poderTocha = clampIntMax(poderAposFacaEBau1 * GetRandomValue(65, 92) / 100, poderFaca + 1, PODER_MAX_INIMIGO - 10);

    // 3. Baú 2 e Espada Multiplicadora escalam a partir do 5 inicial para permitir alcançar Sif e Gwyn até 9999999
    long long poderAposTocha = poderAposFacaEBau1 + poderTocha;
    int bau2 = GetRandomValue(6, 20);
    long long baseAntesMult = poderAposTocha + bau2;

    int multArma = 4;
    if (alvoGwyn > 300)
    {
        long long multNecessario = (alvoGwyn / (baseAntesMult * 2)) + GetRandomValue(2, 4);
        multArma = clampIntMax(multNecessario, 4, 350000);
    }
    else
    {
        multArma = GetRandomValue(3, 5);
    }

    long long poderPreSif = baseAntesMult;
    long long poderComEspada = poderPreSif * (long long)multArma;

    // 4. Sif: fica entre a forma pré-espada e a forma com espada, garantindo vitória na ordem certa
    long long minSif = poderPreSif + 2;
    long long maxSif = (poderComEspada * 65) / 100;
    if (maxSif < minSif)
        maxSif = minSif;
    int poderSif = clampIntMax(minSif + (long long)((double)(maxSif - minSif) * (GetRandomValue(20, 80) / 100.0)),
                               poderTocha + 2, PODER_MAX_INIMIGO - 5);

    // 5. Gwyn: chefe final (até 9999999), vencível após coletar os itens e derrotar o Sif
    long long poderMaxAntesGwyn = poderComEspada + poderSif;
    int poderGwyn = clampIntMax(poderMaxAntesGwyn * GetRandomValue(68, 86) / 100, poderSif + 5, PODER_MAX_INIMIGO);

    // Limiares de evolução visual:
    // Forma Suprema (3) ativa ao derrotar o Sif (unindo-se a ele) ou atingir 75% do poder do Gwyn antes de enfrentá-lo
    arena->limiarEspada1 = clampIntMax((long long)poderInicial + bau1, poderInicial + 1, PODER_MAX_INIMIGO);
    arena->limiarEspada2 = clampIntMax(poderPreSif, arena->limiarEspada1 + 1, PODER_MAX_INIMIGO);
    arena->limiarFormaFinal = clampIntMax((long long)poderGwyn * 75 / 100, arena->limiarEspada2 + 1, poderGwyn);

    Vector2 ocupados[32];
    int qtdOcupados = 0;

    arena->heroi.pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 125.0f);
    arena->heroi.destino = arena->heroi.pos;
    arena->heroi.movendo = 0;
    arena->heroi.direcaoX = 1;
    arena->heroi.velocidade = 360.0f;
    arena->heroi.inimigoAlvoIdx = -1;
    arena->heroi.itemAlvoIdx = -1;
    arena->heroi.timerAtaque = 0.0f;
    arena->heroi.timerRecuo = 0.0f;
    arena->heroi.timerLevelUp = 0.0f;
    arena->heroi.poder = PODER_INICIAL_HEROI; // Sempre 5
    arena->heroi.nivelArma = 0;
    arena->heroi.estagioEvolucao = 0;
    arena->heroi.derrotouSif = 0;
    arena->heroi.vivo = 1;

    arena->qtdItens = 3;

    arena->itens[0].id = 1;
    strcpy(arena->itens[0].nome, "Bau de Almas Pequeno");
    arena->itens[0].tipo = TIPO_ITEM_BAU;
    arena->itens[0].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 115.0f);
    arena->itens[0].bonusPoder = bau1;
    arena->itens[0].multiplicador = 1;
    arena->itens[0].coletado = 0;

    arena->itens[1].id = 2;
    strcpy(arena->itens[1].nome, "Bau de Almas Grande");
    arena->itens[1].tipo = TIPO_ITEM_BAU;
    arena->itens[1].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 115.0f);
    arena->itens[1].bonusPoder = bau2;
    arena->itens[1].multiplicador = 1;
    arena->itens[1].coletado = 0;

    arena->itens[2].id = 3;
    snprintf(arena->itens[2].nome, sizeof(arena->itens[2].nome), "Espada do Abismo x%d", multArma);
    arena->itens[2].tipo = TIPO_ITEM_ARMA_MULT;
    arena->itens[2].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 115.0f);
    arena->itens[2].bonusPoder = 0;
    arena->itens[2].multiplicador = multArma;
    arena->itens[2].coletado = 0;

    arena->qtdInimigos = 4;

    arena->inimigos[0].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 125.0f);
    arena->inimigos[0].poder = poderFaca;
    arena->inimigos[0].derrotado = 0;

    arena->inimigos[1].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 125.0f);
    arena->inimigos[1].poder = poderTocha;
    arena->inimigos[1].derrotado = 0;

    arena->inimigos[2].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 135.0f);
    arena->inimigos[2].poder = poderSif;
    arena->inimigos[2].derrotado = 0;

    arena->inimigos[3].pos = sortearPosicaoLivre(ocupados, &qtdOcupados, largTela, altTela, 145.0f);
    arena->inimigos[3].poder = poderGwyn;
    arena->inimigos[3].derrotado = 0;
}

void executarInterfaceGrafica(Banco *banco)
{
    (void)banco;
    srand((unsigned int)time(NULL));
    SetRandomSeed((unsigned int)time(NULL));

    InitWindow(0, 0, "FLAME CHAOS");
    if (!IsWindowFullscreen())
    {
        ToggleFullscreen();
    }
    SetTargetFPS(60);

    float largTela = (float)GetScreenWidth();
    float altTela = (float)GetScreenHeight();

    Arena arena = {0};

    // Cenário panorâmico
    const char *caminhosCenario[4] = {"assets/dark.png", "assets/dark.jpg", "../assets/dark.png", "dark.png"};
    for (int i = 0; i < 4; i++)
    {
        if (FileExists(caminhosCenario[i]))
        {
            arena.cenario.texCenario = LoadTexture(caminhosCenario[i]);
            if (arena.cenario.texCenario.id > 0)
            {
                arena.cenario.temCenario = 1;
                break;
            }
        }
    }

    // Estágio 0: Herói inicial (assets/hero.png)
    const char *caminhosHeroi[4] = {"assets/hero.png", "assets/hero.jpg", "../assets/hero.png", "hero.png"};
    for (int i = 0; i < 4; i++)
    {
        arena.heroi.texHeroi = carregarSpriteRecortado(caminhosHeroi[i], &arena.heroi.temTexHeroi);
        if (arena.heroi.temTexHeroi)
            break;
    }

    // Estágio 1: Herói com Espada 1 (assets/hero_espada1.png)
    const char *caminhosEspada1[3] = {"assets/hero_espada1.png", "../assets/hero_espada1.png", "hero_espada1.png"};
    for (int i = 0; i < 3; i++)
    {
        arena.heroi.texHeroiEspada1 = carregarSpriteRecortado(caminhosEspada1[i], &arena.heroi.temTexHeroiEspada1);
        if (arena.heroi.temTexHeroiEspada1)
            break;
    }

    // Estágio 2: Herói com Espada 2 (assets/hero_espada2.png)
    const char *caminhosEspada2[3] = {"assets/hero_espada2.png", "../assets/hero_espada2.png", "hero_espada2.png"};
    for (int i = 0; i < 3; i++)
    {
        arena.heroi.texHeroiEspada2 = carregarSpriteRecortado(caminhosEspada2[i], &arena.heroi.temTexHeroiEspada2);
        if (arena.heroi.temTexHeroiEspada2)
            break;
    }

    // Estágio 3: Forma Suprema (assets/hero_final.png)
    const char *caminhosFinal[3] = {"assets/hero_final.png", "../assets/hero_final.png", "hero_final.png"};
    for (int i = 0; i < 3; i++)
    {
        arena.heroi.texHeroiFinal = carregarSpriteRecortado(caminhosFinal[i], &arena.heroi.temTexHeroiFinal);
        if (arena.heroi.temTexHeroiFinal)
            break;
    }

    // Texturas fixas dos inimigos
    arena.inimigos[0].id = 1;
    strcpy(arena.inimigos[0].nome, "Hollow Faca");
    arena.inimigos[0].nivelDificuldade = 1;
    arena.inimigos[0].texInimigo = carregarSpriteRecortado("assets/Hollow faca.png", &arena.inimigos[0].temTexInimigo);

    arena.inimigos[1].id = 2;
    strcpy(arena.inimigos[1].nome, "Hollow Tocha");
    arena.inimigos[1].nivelDificuldade = 2;
    arena.inimigos[1].texInimigo = carregarSpriteRecortado("assets/Hollow Tocha.png", &arena.inimigos[1].temTexInimigo);

    arena.inimigos[2].id = 3;
    strcpy(arena.inimigos[2].nome, "Sif");
    arena.inimigos[2].nivelDificuldade = 4;
    arena.inimigos[2].ehChefe = 1;
    arena.inimigos[2].texInimigo = carregarSpriteRecortado("assets/sif.png", &arena.inimigos[2].temTexInimigo);

    arena.inimigos[3].id = 4;
    strcpy(arena.inimigos[3].nome, "Gwyn");
    arena.inimigos[3].nivelDificuldade = 5;
    arena.inimigos[3].ehChefe = 1;
    arena.inimigos[3].texInimigo = carregarSpriteRecortado("assets/Gwyn.png", &arena.inimigos[3].temTexInimigo);

    // Gera inimigos aleatórios (poder até 9999999) com herói começando em 5
    gerarInimigosAleatorios(&arena, largTela, altTela);

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            processarCliqueCenario(&arena, GetMousePosition());
        }

        atualizarHeroi(&arena, dt);

        BeginDrawing();
        ClearBackground(BLACK);

        if (arena.cenario.temCenario)
        {
            Rectangle src = {0.0f, 0.0f, (float)arena.cenario.texCenario.width, (float)arena.cenario.texCenario.height};
            Rectangle dst = {0.0f, 0.0f, largTela, altTela};
            DrawTexturePro(arena.cenario.texCenario, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        }

        for (int i = 0; i < arena.qtdItens; i++)
        {
            desenharItem(&arena.itens[i], (arena.heroi.itemAlvoIdx == i));
        }

        for (int i = 0; i < arena.qtdInimigos; i++)
        {
            desenharInimigo(&arena.inimigos[i], (arena.heroi.inimigoAlvoIdx == i));
        }

        desenharHeroi(&arena.heroi);

        atualizarEDesenharPopups(dt);

        if (arena.vitoriaArena)
        {
            const char *msg = "INIMIGOS DERROTADOS!";
            int largMsg = MeasureText(msg, 28);
            DrawRectangle((int)(largTela / 2 - largMsg / 2 - 20), 28, largMsg + 40, 48, (Color){10, 10, 15, 220});
            DrawText(msg, (int)(largTela / 2 - largMsg / 2), 38, 28, (Color){255, 215, 80, 255});
        }

        EndDrawing();
    }

    for (int i = 0; i < arena.qtdInimigos; i++)
    {
        if (arena.inimigos[i].temTexInimigo)
            UnloadTexture(arena.inimigos[i].texInimigo);
    }
    for (int i = 0; i < arena.qtdItens; i++)
    {
        if (arena.itens[i].temTexItem)
            UnloadTexture(arena.itens[i].texItem);
    }
    if (arena.heroi.temTexHeroi)
        UnloadTexture(arena.heroi.texHeroi);
    if (arena.heroi.temTexHeroiEspada1)
        UnloadTexture(arena.heroi.texHeroiEspada1);
    if (arena.heroi.temTexHeroiEspada2)
        UnloadTexture(arena.heroi.texHeroiEspada2);
    if (arena.heroi.temTexHeroiFinal)
        UnloadTexture(arena.heroi.texHeroiFinal);
    if (arena.cenario.temCenario)
        UnloadTexture(arena.cenario.texCenario);
    CloseWindow();
}