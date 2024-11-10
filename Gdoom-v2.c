#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAP_WIDTH 24
#define MAP_HEIGHT 24
#define FOV 3.14159 / 3  // 60 degrees field of view
#define MOVESPEED 0.1f   // Movement speed
#define ROTATESPEED 0.05f // Rotation speed

// Map representation (1 = wall, 0 = empty space)
int worldMap[MAP_WIDTH][MAP_HEIGHT] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,1,1,1,1,1,1,1,1,0,0,0,0,0,1,0,0,0,1,1,0,0,1},
    {1,0,1,0,0,0,0,0,1,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1},
    {1,0,1,0,1,1,0,0,1,0,0,0,1,1,1,1,1,0,0,0,1,1,0,1},
    {1,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,0,1,1,0,1,1,0,1,1,1,1,0,1,0,0,1,0,1},
    {1,0,0,0,1,1,1,1,0,0,0,1,0,0,1,0,0,0,0,0,1,0,0,1},
    {1,0,1,1,0,1,0,1,1,0,1,0,0,0,0,1,0,0,0,0,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,1,1,1,1,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,1,0,0,0,0,0,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1},
    {1,0,0,1,0,0,0,0,1,0,0,0,1,1,1,1,1,1,0,0,0,0,0,1},
    {1,0,1,1,1,0,1,0,1,0,1,1,1,1,1,0,1,1,1,0,0,1,1,1},
    {1,0,0,0,1,0,0,0,0,1,0,0,1,0,1,0,0,1,1,1,0,1,1,1},
    {1,0,1,0,0,0,0,0,1,1,0,1,1,1,1,0,0,0,1,1,1,0,0,1},
    {1,0,1,1,1,0,1,0,0,0,1,0,1,0,0,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,1,1,0,0,0,1,0,0,1,1,1,1,1,0,1,0,0,1},
    {1,1,1,1,1,1,1,0,1,1,1,0,0,0,1,1,0,1,0,1,0,1,1,1},
    {1,0,0,0,0,0,1,0,0,0,1,1,0,0,0,1,0,0,0,0,0,0,1,1},
    {1,0,1,0,1,0,1,0,1,1,0,0,1,0,0,1,0,1,0,1,1,1,1,1},
    {1,0,1,0,1,0,0,1,0,0,1,1,0,1,0,1,0,1,1,1,1,0,0,0},
    {1,0,1,0,1,1,0,0,0,1,1,1,0,0,0,1,0,0,0,0,1,1,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

// Player position and direction
float posX = 22, posY = 12;  // Player start position
float dirX = -1, dirY = 0;    // Initial direction vector
float planeX = 0, planeY = 0.66; // Camera plane

// Function to generate a simple wall texture
void generateWallTexture(SDL_Renderer* renderer, SDL_Texture* texture, int width, int height) {
    SDL_Surface* surface = SDL_CreateRGBSurface(0, width, height, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF);
    Uint32* pixels = (Uint32*)surface->pixels;

    // Fill the surface with some color pattern (e.g., a simple gradient or stripe pattern)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int color = (x + y) % 2 == 0 ? SDL_MapRGB(surface->format, 255, 255, 255) : SDL_MapRGB(surface->format, 100, 100, 100);
            pixels[y * width + x] = color;
        }
    }

    SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);
    SDL_FreeSurface(surface);
}

// Function to render the scene with wall textures
void renderScene(SDL_Renderer* renderer, SDL_Texture* wallTexture) {
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        // Calculate ray position and direction
        float cameraX = 2 * x / (float)SCREEN_WIDTH - 1;  // Camera space X
        float rayDirX = dirX + planeX * cameraX;
        float rayDirY = dirY + planeY * cameraX;

        // Which box of the map we're in
        int mapX = (int)posX;
        int mapY = (int)posY;

        // Length of ray from current position to next x or y side
        float sideDistX, sideDistY;

        // Length of ray from one side to next in world space
        float deltaDistX = fabs(1 / rayDirX);
        float deltaDistY = fabs(1 / rayDirY);
        float perpWallDist;

        // Which direction to step in x or y (either +1 or -1)
        int stepX, stepY;
        int hit = 0;  // was there a wall hit?
        int side;     // was a NS or EW wall hit?

        // Calculate step and initial sideDist
        if (rayDirX < 0) {
            stepX = -1;
            sideDistX = (posX - mapX) * deltaDistX;
        } else {
            stepX = 1;
            sideDistX = (mapX + 1.0 - posX) * deltaDistX;
        }
        if (rayDirY < 0) {
            stepY = -1;
            sideDistY = (posY - mapY) * deltaDistY;
        } else {
            stepY = 1;
            sideDistY = (mapY + 1.0 - posY) * deltaDistY;
        }

        // Perform DDA
        while (hit == 0) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = 0;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = 1;
            }

            if (worldMap[mapX][mapY] > 0) hit = 1;
        }

        // Calculate distance projected on camera direction (perpendicular distance)
        if (side == 0) {
            perpWallDist = (mapX - posX + (1 - stepX) / 2) / rayDirX;
        } else {
            perpWallDist = (mapY - posY + (1 - stepY) / 2) / rayDirY;
        }

        // Calculate height of line to draw on screen
        int lineHeight = (int)(SCREEN_HEIGHT / perpWallDist);

        // Calculate draw start and end positions
        int drawStart = -lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawStart < 0) drawStart = 0;
        int drawEnd = lineHeight / 2 + SCREEN_HEIGHT / 2;
        if (drawEnd >= SCREEN_HEIGHT) drawEnd = SCREEN_HEIGHT - 1;

        // Texture mapping
        int texX = (int)(perpWallDist * 64) % 64;
        if (side == 0 && rayDirX > 0) texX = 64 - texX - 1;
        if (side == 1 && rayDirY < 0) texX = 64 - texX - 1;

        SDL_Rect srcRect = { texX * 2, 0, 2, 64 };
        SDL_Rect dstRect = { x, drawStart, 1, drawEnd - drawStart };

        SDL_RenderCopy(renderer, wallTexture, &srcRect, &dstRect);
    }
}

// Handle player input for movement and rotation
void handleInput(SDL_Event* e) {
    const float moveSpeed = MOVESPEED;  
    const float rotSpeed = ROTATESPEED; 

    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
            case SDLK_w:  
                if (worldMap[(int)(posX + dirX * moveSpeed)][(int)(posY)] == 0) posX += dirX * moveSpeed;
                if (worldMap[(int)(posX)][(int)(posY + dirY * moveSpeed)] == 0) posY += dirY * moveSpeed;
                break;
            case SDLK_s:  
                if (worldMap[(int)(posX - dirX * moveSpeed)][(int)(posY)] == 0) posX -= dirX * moveSpeed;
                if (worldMap[(int)(posX)][(int)(posY - dirY * moveSpeed)] == 0) posY -= dirY * moveSpeed;
                break;
            case SDLK_a:  
                {
                    float oldDirX = dirX;
                    dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
                    dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
                    float oldPlaneX = planeX;
                    planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
                    planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
                }
                break;
            case SDLK_d:  
                {
                    float oldDirX = dirX;
                    dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
                    dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
                    float oldPlaneX = planeX;
                    planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
                    planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
                }
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Window* window = SDL_CreateWindow("Doom-like Raycasting",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return -1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture* wallTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 64, 64);

    generateWallTexture(renderer, wallTexture, 64, 64);

    int running = 1;
    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = 0;
            } else {
                handleInput(&e);  // Handle input
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Set background color
        SDL_RenderClear(renderer);
        renderScene(renderer, wallTexture);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(wallTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

