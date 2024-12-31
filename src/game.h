#pragma once

#include "input.h"
#include "schnitzel_lib.h"
#include "render_interface.h"

// ################################################################
//                    Game Structs
// ################################################################
struct GameState
{
    bool initialized = false;
    IVec2 playerPos;
};

// ################################################################
//                    Game Globals
// ################################################################
static GameState* gameState;
constexpr int tset = 5;

// ################################################################
//                    Game Functions (Exported)
// ################################################################
extern "C"
{
    EXPORT_FN void update_game(GameState* gameStateIn, RenderData* renderDataIn, Input* inputIn);
}