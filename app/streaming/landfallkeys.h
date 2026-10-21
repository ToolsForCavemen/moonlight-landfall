#pragma once

// Landfall: reglas de teclas del que esta trabajando. Cada combinacion va ALLA (al host, lo normal) o se queda ACA
// (en el equipo local). Landfall escribe las reglas de la conexion en el archivo que dice $LANDFALL_TECLAS, una por
// linea: "aca Super+Space" o "alla Ctrl+Alt+Delete" (nombres de tecla de SDL_GetScancodeFromName).
// Cuando una combinacion ACA llega con el teclado tomado, Moonlight no la manda, suelta el teclado y escribe en su
// salida "LANDFALL-TECLA <combinacion>"; Landfall la vuelve a apretar en el equipo local.

#include <SDL.h>
#include <QString>
#include <QVector>

#define SDL_CODE_LANDFALL_REGRAB 199

namespace LandfallKeys {

enum {
    MOD_CTRL = 1,
    MOD_ALT = 2,
    MOD_SHIFT = 4,
    MOD_SUPER = 8,
};

struct Rule {
    int mods;
    SDL_Scancode key;
    bool local;
    QByteArray text;
};

// Lee $LANDFALL_TECLAS si cambio desde la ultima vez (se llama al abrir el video y al recibir el foco).
void load();

// La regla de esa tecla con esos modificadores (exactos), o nullptr.
const Rule* match(SDL_Scancode key, Uint16 sdlMod);

// Los modificadores de SDL (izquierdo o derecho, da igual) como MOD_*.
int modsOf(Uint16 sdlMod);

}
