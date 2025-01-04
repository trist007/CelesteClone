#include "game.h"

#include "assets.h"

// #############################################################################
//                            Game Constants
// #############################################################################

// #############################################################################
//                            Game Structs
// #############################################################################

// #############################################################################
//                            Game Functions
// #############################################################################
bool just_pressed(GameInputType type)
{
    KeyMapping mapping = gameState->keyMappings[type];
    for(int idx = 0; idx < mapping.keys.count; idx++)
    {
        if(input->keys[mapping.keys[idx]].justPressed)
        {
            return true;
        }
    }

    return false;
}

bool is_down(GameInputType type)
{
    KeyMapping mapping = gameState->keyMappings[type];
    for(int idx = 0; idx < mapping.keys.count; idx++)
    {
        if(input->keys[mapping.keys[idx]].isDown)
        {
            return true;
        }
    }

    return false;
}

Tile* get_tile(int x, int y)
{
    Tile* tile = nullptr;

    if(x >= 0 && x < WORLD_GRID.x && y >= 0 && y < WORLD_GRID.y)
    {
        tile = &gameState->worldGrid[x][y];
    }

    return tile;
}

Tile* get_tile(IVec2 worldPos)
{
    int x = worldPos.x / TILESIZE;
    int y = worldPos.y / TILESIZE;

    return get_tile(x, y);
}
// #############################################################################
//                            Game Functions(exposed)
// #############################################################################
EXPORT_FN void update_game(GameState* gameStateIn, RenderData* renderDataIn, Input* inputIn)
{
    if(renderData != renderDataIn)
    {
        gameState = gameStateIn;
        renderData = renderDataIn;
        input = inputIn;
    }

    if(!gameState->initialized)
    {
        renderData->gameCamera.dimensions = {WORLD_WIDTH, WORLD_HEIGHT};
        gameState->initialized = true;

        // Tileset
        {
            IVec2 tilesPosition = {48, 0};

            for (int y = 0; y < 5; y++)
            {
                for (int x = 0; x < 4; x++)
                {
                    gameState->tileCoords.add({tilesPosition.x + x * 8, tilesPosition.y + y * 8});
                }
            }

            // Black inside
            gameState->tileCoords.add({tilesPosition.x, tilesPosition.y + 5 * 8});
        }

        // Key Mappings
        {
            gameState->keyMappings[MOVE_UP].keys.add(KEY_W);
            gameState->keyMappings[MOVE_UP].keys.add(KEY_UP);
            gameState->keyMappings[MOVE_LEFT].keys.add(KEY_A);
            gameState->keyMappings[MOVE_LEFT].keys.add(KEY_LEFT);
            gameState->keyMappings[MOVE_DOWN].keys.add(KEY_S);
            gameState->keyMappings[MOVE_DOWN].keys.add(KEY_DOWN);
            gameState->keyMappings[MOVE_RIGHT].keys.add(KEY_D);
            gameState->keyMappings[MOVE_RIGHT].keys.add(KEY_RIGHT);
            gameState->keyMappings[MOUSE_LEFT].keys.add(KEY_MOUSE_LEFT);
            gameState->keyMappings[MOUSE_RIGHT].keys.add(KEY_MOUSE_RIGHT);
        }

        renderData->gameCamera.position.x = 160;
        renderData->gameCamera.position.y = -90;
    }

    if(is_down(MOUSE_LEFT))
    {
        IVec2 mousePosWorld = input->mousePosWorld;
        Tile* tile = get_tile(mousePosWorld);
        if(tile)
        {
            tile->isVisible = true;
        }
    }

    if(is_down(MOUSE_RIGHT))
    {
        IVec2 mousePosWorld = input->mousePosWorld;
        Tile* tile = get_tile(mousePosWorld);
        if(tile)
        {
            tile->isVisible = false;
        }
    }

    // Drawing Tileset
    {
        // Neighboring Tiles        Top     Left    Bottom      Right 
        int neighborOffsets[24] = { 0,-1,  -1, 0,    1, 0,      0, 1,
        //                          Topleft Topright Bottomleft Bottomright
                                    -1,-1,  1, -1,  -1, 1,      1, 1, 
        //                          Top2     Left2   Right2     Bottom2
                                    0,-2,    -2, 0,   2, 0,     0, 2};   

        // Topleft      = BIT(4) = 16
        // Topright     = BIT(5) = 32
        // Bottomleft   = BIT(6) = 64
        // Bottomright  = BIT(7) = 128

        for(int y = 0; y < WORLD_GRID.y; y++)
        {
            for(int x = 0; x < WORLD_GRID.x; x++)
            {
                Tile* tile = get_tile(x, y);
                
                if(!tile->isVisible)
                {
                    continue;
                }

                tile->neighborMask = 0;
                int neighborCount = 0;
                int extendedNeighborCount = 0;
                int emptyNeighborSlot = 0;

                // Look at the surrounding 12 Neighbords
                for(int n = 0; n < 12; n++)
                {
                    Tile* neighbor = get_tile(x + neighborOffsets[n * 2],
                                              y + neighborOffsets[n * 2 + 1]);
                    
                    // No neighbor means the edge of the world
                    if(!neighbor || neighbor->isVisible)
                    {
                        tile->neighborMask |= BIT(n);
                        if(n < 8) // Counting direct neighbors
                        {
                            neighborCount++;
                        }
                        else
                        {
                            extendedNeighborCount++;
                        }
                    }
                    else if(n < 8)
                    {
                        emptyNeighborSlot = n;
                    }
                }

                if(neighborCount == 7 && emptyNeighborSlot >= 4) // We have a corner
                {
                    tile->neighborMask = 16 + (emptyNeighborSlot - 4);
                }
                else if(neighborCount == 8 && extendedNeighborCount == 4)
                {
                    tile->neighborMask = 20;
                }
                else
                {
                    tile->neighborMask  = tile->neighborMask & 0b1111;
                }

                // Draw Tile
                Transform transform = {};
                // Draw the Tile around the center
                transform.pos = {x * (float)TILESIZE, y * (float)TILESIZE};
                transform.size = {8, 8};
                transform.spriteSize = {8, 8};
                transform.atlasOffset = gameState->tileCoords[tile->neighborMask];
                draw_quad(transform);
            }
        }
    }

    draw_sprite(SPRITE_DICE, gameState->playerPos);

    if(is_down(MOVE_LEFT))
    {
        gameState->playerPos.x -= 1;
    }
    if(is_down(MOVE_RIGHT))
    {
        gameState->playerPos.x += 1;
    }
    if(is_down(MOVE_UP))
    {
        gameState->playerPos.y -= 1;
    }
    if(is_down(MOVE_DOWN))
    {
        gameState->playerPos.y += 1;
    }
}