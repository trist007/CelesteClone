#pragma once

#include "input.h"
#include "schnitzel_lib.h"
#include "render_interface.h"

// ################################################################
//                    Game Globals
// ################################################################
constexpr int tset = 5;

// ################################################################
//                    Game Structs
// ################################################################
struct GameState
{
    IVec2 playerPos;
};

// ################################################################
//                    Game Functions (Exported)
// ################################################################
extern "C"
{
    EXPORT_FN void update_game(RenderData* renderDataIn, Input* inputIn);
}