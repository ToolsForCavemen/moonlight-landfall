#pragma once

#include "SDL_compat.h"

// Landfall: files dropped on the stream window are reported to Landfall (the
// program that launched Moonlight) on stderr, as a single line:
//   LANDFALL-SOLTAR <x> <y> <path> [<path>...]
// x and y are thousandths of the width and height of the remote screen and each
// path is percent-encoded. Landfall copies the files to the remote PC and drops
// them there, at that same point.
namespace LandfallDrop
{
void start();
void stop();

// Returns true if the event was a drop event (and was consumed).
bool handleEvent(SDL_Event* event, SDL_Window* window, int streamWidth, int streamHeight);
}
