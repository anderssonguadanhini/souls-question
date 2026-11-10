#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "flameChaos.h"

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