#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "raylib.h"
#include "interface.h"

// Carrega o sprite do herói removendo o fundo preto puro
static Texture2D carregarHeroiRecortado(const char *caminho, int *carregado)
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

    for (int i = 0; i < total; i++)
    {
        if (pixels[i].r < 12 && pixels[i].g < 12 && pixels[i].b < 12)
        {
            pixels[i].a = 0;
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

static void desenharHeroi(Vector2 pos, int poder, Texture2D texHeroi, int temTexHeroi, float pulso)
{
    // Sombra oval no chão isométrico
    DrawEllipse((int)pos.x, (int)pos.y + 4, 30, 11, (Color){0, 0, 0, 185});

    float alturaTopo = pos.y - 120.0f + pulso;

    if (temTexHeroi)
    {
        float escala = 155.0f / (float)texHeroi.height;
        float larg = texHeroi.width * escala;
        float alt = texHeroi.height * escala;

        Rectangle src = {0.0f, 0.0f, (float)texHeroi.width, (float)texHeroi.height};
        Rectangle dst = {pos.x - larg / 2.0f, pos.y - alt + 8.0f + pulso, larg, alt};
        DrawTexturePro(texHeroi, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        alturaTopo = pos.y - alt + 2.0f + pulso;
    }

    // Número azul flutuante acima da cabeça (estilo Hero Wars)
    const char *txtPoder = TextFormat("%d", poder);
    int tamFonte = 28;
    int largTxt = MeasureText(txtPoder, tamFonte);

    int txtX = (int)(pos.x - largTxt / 2.0f);
    int txtY = (int)alturaTopo - 28;

    DrawText(txtPoder, txtX + 2, txtY + 2, tamFonte, BLACK);
    DrawText(txtPoder, txtX - 1, txtY - 1, tamFonte, BLACK);
    DrawText(txtPoder, txtX, txtY, tamFonte, (Color){80, 180, 255, 255});
}

void executarInterfaceGrafica(Banco *banco)
{
    (void)banco;

    InitWindow(0, 0, "FLAME CHAOS");
    if (!IsWindowFullscreen())
    {
        ToggleFullscreen();
    }
    SetTargetFPS(60);

    // Carrega o cenário panorâmico em tela cheia
    Texture2D texCenario = {0};
    int temCenario = 0;
    const char *caminhosCenario[4] = {"assets/dark.png", "assets/dark.jpg", "../assets/dark.png", "dark.png"};
    for (int i = 0; i < 4; i++)
    {
        if (FileExists(caminhosCenario[i]))
        {
            texCenario = LoadTexture(caminhosCenario[i]);
            if (texCenario.id > 0)
            {
                temCenario = 1;
                break;
            }
        }
    }

    // Carrega apenas o sprite do Artorias (assets/hero.png)
    Texture2D texHeroi = {0};
    int temTexHeroi = 0;
    const char *caminhosHeroi[4] = {"assets/hero.png", "assets/hero.jpg", "../assets/hero.png", "hero.png"};
    for (int i = 0; i < 4; i++)
    {
        texHeroi = carregarHeroiRecortado(caminhosHeroi[i], &temTexHeroi);
        if (temTexHeroi)
            break;
    }

    Vector2 posHeroi = {GetScreenWidth() * 0.50f, GetScreenHeight() * 0.68f};
    int poderHeroi = 9;

    while (!WindowShouldClose())
    {
        float pulso = sinf((float)GetTime() * 3.5f) * 2.5f;

        // Clica com o mouse para posicionar o Artorias onde quiser na arena
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            posHeroi = GetMousePosition();
        }

        BeginDrawing();
        ClearBackground(BLACK);

        if (temCenario)
        {
            Rectangle src = {0.0f, 0.0f, (float)texCenario.width, (float)texCenario.height};
            Rectangle dst = {0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()};
            DrawTexturePro(texCenario, src, dst, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
        }

        desenharHeroi(posHeroi, poderHeroi, texHeroi, temTexHeroi, pulso);

        EndDrawing();
    }

    if (temTexHeroi)
        UnloadTexture(texHeroi);
    if (temCenario)
        UnloadTexture(texCenario);
    CloseWindow();
}