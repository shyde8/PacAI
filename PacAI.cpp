/*******************************************************************************************
*
*   
*
*   
*   
*
*   Copyright (c) 2021 Steven Hyde
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h" 
#include "math.h"
#include <stdio.h>
#include <time.h>
#include <string>
#include <limits>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 900
#define MAZE_ORIGIN_X 50
#define MAZE_ORIGIN_Y 50
#define MAZE_SCALE 2
#define PIXELS_PER_TILE 8
#define NUM_TILES_HORIZONTAL 28
#define NUM_TILES_VERTICAL 31
#define STARTING_ROW 23
#define STARTING_COLUMN 13

typedef enum Orientation { 
    up,
    down,
    left,
    right,
    none
} Orientation;

typedef enum GhostState {
    chase,
    scatter,
    frightened,
} GhostState;

typedef struct Actor {
    Vector2 centroid;
    int width;
    int height;
    int currentTileX;
    int currentTileY;
    Orientation orientation;
    float speed;
} Actor;

typedef struct Ghost : Actor {
    int nextTileX;
    int nextTileY;
    int nextNextTileX;
    int nextNextTileY;
    Vector2 pendingPosition;
    Orientation pendingDirection;
    int targetTileX;
    int targetTileY;    
} Ghost;

typedef struct Coordinate {
    int x;
    int y;
} Coordinate;

// Custom logging funtion
void LogCustom(int msgType, const char *text, va_list args)
{
    char timeStr[64] = { 0 };
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s] ", timeStr);

    switch (msgType)
    {
        case LOG_INFO: printf("[INFO] : "); break;
        case LOG_ERROR: printf("[ERROR]: "); break;
        case LOG_WARNING: printf("[WARN] : "); break;
        case LOG_DEBUG: printf("[DEBUG]: "); break;
        default: break;
    }

    vprintf(text, args);
    printf("\n");
}

Vector2 CalculatePositionBasedOnTile(int row, int column, float cellSize){
    return Vector2{MAZE_ORIGIN_X + (column * cellSize) + (cellSize / 2), MAZE_ORIGIN_Y + (row * cellSize) + (cellSize / 2)};
}

void SetCurrentTileForActor(Actor &actor, float cellSize) {
    actor.currentTileX = floor((actor.centroid.x - MAZE_ORIGIN_X) / cellSize);
    actor.currentTileY = floor((actor.centroid.y - MAZE_ORIGIN_Y) / cellSize);    
}

bool IsTraversable(const Actor &actor, Orientation direction, float deltaTime, float cellSize, const int (&grid)[NUM_TILES_VERTICAL][NUM_TILES_HORIZONTAL]) {
    
    float theoreticalPositionX;
    float theoreticalPositionY;
    int targetTileX;
    int targetTileY; 
    
    if (direction == left) {
        theoreticalPositionX = actor.centroid.x - actor.speed * deltaTime;
        theoreticalPositionY = actor.centroid.y;
        targetTileX = floor((theoreticalPositionX - MAZE_ORIGIN_X) / cellSize);
        targetTileY = actor.currentTileY;
        
        // verify target tile is within grid bounds //
        if (targetTileX > -1 && targetTileX < NUM_TILES_HORIZONTAL) {
            
            // actor can only move to a tile which is not an obstacle //
            if (grid[targetTileY][targetTileX] == 1) {
                
                if (actor.orientation == left || actor.orientation == right || actor.orientation == none) {
                    
                    // if actor is approaching a barrier, we don't want it to proceed past the centroid of its target tile //
                    if (!(grid[targetTileY][targetTileX - 1] == 0 && actor.centroid.x <= MAZE_ORIGIN_X + targetTileX * cellSize + (cellSize / 2))) {
                        return true;
                    }                    
                }
                
                // if moving vertically, only allow horizontal turn if actor is level with target tile's centroid //
                else if (theoreticalPositionY - (MAZE_ORIGIN_Y + targetTileY * cellSize + (cellSize / 2)) < 1) {
                    
                    
                    if (targetTileX - 1 > 0 && grid[targetTileY][targetTileX - 1] == 1) {     
                        return true;
                    }
                }
            }
        }
    }
    else if (direction == right) {
        theoreticalPositionX = actor.centroid.x + actor.speed * deltaTime;
        theoreticalPositionY = actor.centroid.y;
        targetTileX = floor((theoreticalPositionX - MAZE_ORIGIN_X) / cellSize);
        targetTileY = actor.currentTileY;
        
        // verify target tile is within grid bounds //
        if (targetTileX > -1 && targetTileX < NUM_TILES_HORIZONTAL) {
            
            // actor can only move to a tile which is not an obstacle //
            if (grid[targetTileY][targetTileX] == 1) {
                
                if (actor.orientation == left || actor.orientation == right || actor.orientation == none) {
                    
                    // if actor is approaching a barrier, we don't want it to proceed past the centroid of its target tile //
                    if (!(grid[targetTileY][targetTileX + 1] == 0 && actor.centroid.x >= MAZE_ORIGIN_X + targetTileX * cellSize + (cellSize / 2))) {
                        return true;
                    }                    
                }
                
                // if moving vertically, only allow horizontal turn if actor is level with target tile's centroid //
                else if (theoreticalPositionY - (MAZE_ORIGIN_Y + targetTileY * cellSize + (cellSize / 2)) < 1) {                   
                    
                    if (targetTileX + 1 < NUM_TILES_HORIZONTAL && grid[targetTileY][targetTileX + 1] == 1) {     
                        return true;
                    }
                }
            }
        }
    }
    else if (direction == up) {
        theoreticalPositionX = actor.centroid.x;
        theoreticalPositionY = actor.centroid.y - actor.speed * deltaTime;
        targetTileX = actor.currentTileX;
        targetTileY = floor((theoreticalPositionY - MAZE_ORIGIN_Y) / cellSize);
                
        // verify target tile is within grid bounds //
        if (targetTileY > -1 && targetTileY < NUM_TILES_VERTICAL) {
            
            // actor can only move to a tile which is not an obstacle //
            if (grid[targetTileY][targetTileX] == 1) {
                
                if (actor.orientation == up || actor.orientation == down || actor.orientation == none) {
                    
                    // if actor is approaching a barrier, we don't want it to proceed past the centroid of its target tile //
                    if (!(grid[targetTileY - 1][targetTileX] == 0 && actor.centroid.y <= MAZE_ORIGIN_Y + targetTileY * cellSize + (cellSize / 2))) {
                        return true;
                    }                            
                }
                
                // if moving horizontally, only allow vertical turn if actor is level with target tile's centroid //
                else if (theoreticalPositionX - (MAZE_ORIGIN_X + targetTileX * cellSize + (cellSize / 2)) < 1) {
                            
                        if (targetTileY - 1 > 0 && grid[targetTileY - 1][targetTileX] == 1) { 
                            return true;
                        }
                }
            }
        }
    }
    else if (direction == down) {
        theoreticalPositionX = actor.centroid.x;
        theoreticalPositionY = actor.centroid.y + actor.speed * deltaTime;
        targetTileX = actor.currentTileX;
        targetTileY = floor((theoreticalPositionY - MAZE_ORIGIN_Y) / cellSize);
                
        // verify target tile is within grid bounds //
        if (targetTileY > -1 && targetTileY < NUM_TILES_VERTICAL) {
            
            // actor can only move to a tile which is not an obstacle //
            if (grid[targetTileY][targetTileX] == 1) {
                
                if (actor.orientation == up || actor.orientation == down || actor.orientation == none) {
                    
                    // if actor is approaching a barrier, we don't want it to proceed past the centroid of its target tile //
                    if (!(grid[targetTileY + 1][targetTileX] == 0 && actor.centroid.y >= MAZE_ORIGIN_Y + targetTileY * cellSize + (cellSize / 2))) {
                        return true;
                    }                            
                }
                
                // if moving horizontally, only allow vertical turn if actor is level with target tile's centroid //
                else if (theoreticalPositionX - (MAZE_ORIGIN_X + targetTileX * cellSize + (cellSize / 2)) < 1) {
                            
                        if (targetTileY + 1 < NUM_TILES_VERTICAL && grid[targetTileY + 1][targetTileX] == 1) { 
                            return true;
                        }
                }
            }
        }
    }
  
    return false; 
}

void MoveActor(Actor &actor, Orientation orientation, float deltaTime, float cellSize) {
    if (orientation == left)
        actor.centroid.x = actor.centroid.x - actor.speed * deltaTime;
    else if (orientation == right)
        actor.centroid.x = actor.centroid.x + actor.speed * deltaTime;
    else if (orientation == up)
        actor.centroid.y = actor.centroid.y - actor.speed * deltaTime;
    else if (orientation == down)
        actor.centroid.y = actor.centroid.y + actor.speed * deltaTime;
    actor.orientation = orientation;
    SetCurrentTileForActor(actor, cellSize);
}

bool IsReversal(Orientation actorDirection, Orientation newDirection) {
    if (actorDirection == left && newDirection == right)
        return true;
    else if (actorDirection == right && newDirection == left)
        return true;
    else if (actorDirection == up && newDirection == down)
        return true;
    else if (actorDirection == down && newDirection == up)
        return true;    
    return false;
}

int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "PacAI");
    SetTargetFPS(60);                    
    
    Texture2D maze = LoadTexture("resources/maze.png");
    Texture2D pacman = LoadTexture("resources/pacman.png");
    Texture2D blinky_png = LoadTexture("resources/blinky.png");    
    
    // Initialize Grid //
    const Vector2 mazeOrigin = Vector2{MAZE_ORIGIN_X, MAZE_ORIGIN_Y};
    float cellWidth = PIXELS_PER_TILE * MAZE_SCALE;
    float cellHeight = PIXELS_PER_TILE * MAZE_SCALE;
    float cellSize = PIXELS_PER_TILE * MAZE_SCALE;
    
    int grid[NUM_TILES_VERTICAL][NUM_TILES_HORIZONTAL] = {
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},  
        {0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {0,1,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,1,0},
        {0,1,1,1,1,1,1,0,0,1,1,1,1,0,0,1,1,1,1,0,0,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0},
        {1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,1,1,1,1,1,1,1,1,1,1,0,0,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0},
        {0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0},
        {0,1,0,0,0,0,1,0,0,0,0,0,1,0,0,1,0,0,0,0,0,1,0,0,0,0,1,0},
        {0,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,0},
        {0,0,0,1,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,1,0,0,0},
        {0,0,0,1,0,0,1,0,0,1,0,0,0,0,0,0,0,0,1,0,0,1,0,0,1,0,0,0},
        {0,1,1,1,1,1,1,0,0,1,1,1,1,0,0,1,1,1,1,0,0,1,1,1,1,1,1,0},
        {0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0},
        {0,1,0,0,0,0,0,0,0,0,0,0,1,0,0,1,0,0,0,0,0,0,0,0,0,0,1,0},
        {0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    }; 
    
    // Initialize Player //
    Actor player;    
    player.centroid = CalculatePositionBasedOnTile(STARTING_ROW, STARTING_COLUMN, cellSize);
    player.width = pacman.width * MAZE_SCALE;
    player.height = pacman.height * MAZE_SCALE;
    player.currentTileX = STARTING_COLUMN;
    player.currentTileY = STARTING_ROW;
    player.orientation = left;
    player.speed = 100;

    // Initialize Ghosts //
    Ghost blinky;
    blinky.centroid = CalculatePositionBasedOnTile(11, 13, cellSize);
    blinky.width = blinky_png.width * MAZE_SCALE;
    blinky.height = blinky_png.height * MAZE_SCALE;
    blinky.currentTileX = 13;
    blinky.currentTileY = 11;
    blinky.nextTileX = 12;
    blinky.nextTileY = 11;
    blinky.pendingPosition = CalculatePositionBasedOnTile(blinky.nextTileY, blinky.nextTileX, cellSize);
    blinky.orientation = left;
    blinky.pendingDirection = none;
    blinky.speed = 100;
    
    // Initialize AI //
    GhostState ghostState = chase;
    Orientation directions[4] { up, left, down, right };
    
  
    // Main game loop
    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();      

        // Process Input
        //----------------------------------------------------------------------------------
        Orientation inp;
        if (IsKeyDown(KEY_LEFT))
            inp = left;
        else if (IsKeyDown(KEY_RIGHT))
            inp = right;
        else if (IsKeyDown(KEY_UP))
            inp = up;
        else if (IsKeyDown(KEY_DOWN))
            inp = down;
        
        // Update Player Location
        //----------------------------------------------------------------------------------
        
        if (IsTraversable(player, inp, deltaTime, cellSize, grid))
            MoveActor(player, inp, deltaTime, cellSize);
        else if (IsTraversable(player, player.orientation, deltaTime, cellSize, grid))
            MoveActor(player, player.orientation, deltaTime, cellSize);
        else
            player.orientation = none;
     
        // Artificial Intelligence
        //----------------------------------------------------------------------------------
        blinky.targetTileX = player.currentTileX;
        blinky.targetTileY = player.currentTileY;
        
        if (blinky.pendingDirection == none) {            
            
            float minDistance = std::numeric_limits<float>::max();           
            for (int i = 0; i < 4; i++) {
                if (IsReversal(blinky.orientation, directions[i])) {
                    continue;
                }
                
                if (directions[i] == left) {
                    if (grid[blinky.nextTileY][blinky.nextTileX - 1] == 1) {                        
                        //float dist = Vector2Distance(CalculatePositionBasedOnTile(blinky.nextTileY, blinky.nextTileX - 1, cellSize), Vector2{blinky.targetTileX, blinky.targetTileY});
                        float dist = abs(blinky.nextTileY - blinky.targetTileY) + abs(blinky.nextTileX - 1 - blinky.targetTileX);
                        if (dist < minDistance) {
                            minDistance = dist;
                            blinky.pendingDirection = left;
                            blinky.nextNextTileX = blinky.nextTileX - 1;
                            blinky.nextNextTileY = blinky.nextTileY;
                            LogCustom(LOG_DEBUG, "left", NULL);
                        }
                    }
                }
                else if (directions[i] == right) {
                    if (grid[blinky.nextTileY][blinky.nextTileX + 1] == 1) {
                        //float dist = Vector2Distance(CalculatePositionBasedOnTile(blinky.nextTileY, blinky.nextTileX + 1, cellSize), Vector2{blinky.targetTileX, blinky.targetTileY});
                        float dist = abs(blinky.nextTileY - blinky.targetTileY) + abs(blinky.nextTileX + 1 - blinky.targetTileX);
                        if (dist < minDistance) {
                            minDistance = dist;
                            blinky.pendingDirection = right;
                            blinky.nextNextTileX = blinky.nextTileX + 1;
                            blinky.nextNextTileY = blinky.nextTileY;
                            LogCustom(LOG_DEBUG, "right", NULL);
                        }
                    }
                }
                else if (directions[i] == up) {
                    if (grid[blinky.nextTileY - 1][blinky.nextTileX] == 1) {
                        //float dist = Vector2Distance(CalculatePositionBasedOnTile(blinky.nextTileY - 1, blinky.nextTileX, cellSize), Vector2{blinky.targetTileX, blinky.targetTileY});
                        float dist = abs(blinky.nextTileY - 1 - blinky.targetTileY) + abs(blinky.nextTileX - blinky.targetTileX);
                        if (dist < minDistance) {
                            minDistance = dist;
                            blinky.pendingDirection = up;
                            blinky.nextNextTileX = blinky.nextTileX;
                            blinky.nextNextTileY = blinky.nextTileY - 1;
                            LogCustom(LOG_DEBUG, "up", NULL);
                        }
                    }
                }
                else if (directions[i] == down) {
                    if (grid[blinky.nextTileY + 1][blinky.nextTileX] == 1) {
                        //float dist = Vector2Distance(CalculatePositionBasedOnTile(blinky.nextTileY + 1, blinky.nextTileX, cellSize), Vector2{blinky.targetTileX, blinky.targetTileY});
                        float dist = abs(blinky.nextTileY + 1 - blinky.targetTileY) + abs(blinky.nextTileX - blinky.targetTileX);
                        if (dist < minDistance) {
                            minDistance = dist;
                            blinky.pendingDirection = down;
                            blinky.nextNextTileX = blinky.nextTileX;
                            blinky.nextNextTileY = blinky.nextTileY + 1;
                            LogCustom(LOG_DEBUG, "down", NULL);
                        }
                    }
                }                              
            }
                        
        }
        
        if (abs(blinky.pendingPosition.x - blinky.centroid.x) < 1 && abs(blinky.pendingPosition.y - blinky.centroid.y) < 1) {
        //if (Vector2Distance(blinky.pendingPosition, blinky.centroid) < 1) {
            if (IsTraversable(blinky, blinky.pendingDirection, deltaTime, cellSize, grid)) {                
                MoveActor(blinky, blinky.pendingDirection, deltaTime, cellSize);
                blinky.pendingDirection = none;
                blinky.nextTileX = blinky.nextNextTileX;
                blinky.nextTileY = blinky.nextNextTileY;
                blinky.pendingPosition = CalculatePositionBasedOnTile(blinky.nextTileY, blinky.nextTileX, cellSize);
            }
            else
                if (IsTraversable(blinky, blinky.orientation, deltaTime, cellSize, grid))
                    MoveActor(blinky, blinky.orientation, deltaTime, cellSize);
                //LogCustom(LOG_DEBUG, "trying to move, but can't!", NULL);            
        }
        else
            if (IsTraversable(blinky, blinky.orientation, deltaTime, cellSize, grid))
                MoveActor(blinky, blinky.orientation, deltaTime, cellSize);

        // Render
        //----------------------------------------------------------------------------------
        BeginDrawing();
        
        ClearBackground(BLACK);
        
        DrawTextureEx(maze, mazeOrigin, 0, MAZE_SCALE, WHITE);
        
        for (int i = 0; i < NUM_TILES_VERTICAL; i++) {
            for (int j = 0; j < NUM_TILES_HORIZONTAL; j++) {
                float x = mazeOrigin.x + (cellWidth * j);
                float y = mazeOrigin.y + (cellHeight * i);
                
                if (grid[i][j] == 1)
                    DrawRectangleLines(x, y, cellWidth, cellHeight, GREEN);
                // else
                    // DrawRectangleLines(x, y, cellWidth, cellHeight, RED);                    
                
            }
        }
                
        DrawRectangleLines(mazeOrigin.x + (cellWidth * player.currentTileX), mazeOrigin.y + (cellHeight * player.currentTileY), cellWidth, cellHeight, RED);
        DrawTextureEx(pacman, Vector2{player.centroid.x - player.width / 2, player.centroid.y - player.height / 2}, 0, MAZE_SCALE, WHITE);       
        DrawTextureEx(blinky_png, Vector2{blinky.centroid.x - blinky.width / 2, blinky.centroid.y - blinky.height / 2}, 0, MAZE_SCALE, WHITE);       
    
        EndDrawing();
        
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
