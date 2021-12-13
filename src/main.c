/* main.c - Game of Life
 *
 * Game of Life Simulation, created with C and SDL
 * Made by - OmegaLol21
 *
 * This is a simulation of John Conway's Game of Life written in C.
 * This program is able to do the following:
 *  - Simulate cells living and dying
 *  - Pause and start a simulation
 *  - Select cells to live/die before starting a simulation
 *  - Able to save a simulation state into a file, and load a custom
 *    simulation as well.
 *  - Use the SDL backend
 *  - Change the speed of the simulation
 *  - Uses https://github.com/btzy/nativefiledialog-extended for
 *    file browsing dialogs
 *  - Cross platform!
*/

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <nfd.h>

// Bit locations, each cell will be stored in a 8 bit integer.
// Starting from the least significant bit
// 0 - Cell is alive and well
// 1 - Cell will be revived next tick
// 2 - Cell will die next tick
#define CELL_ALIVE 0x1
#define CELL_REVIVE 0x2
#define CELL_DIE 0x4

// 720p
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

// How big the grid is in pixels
#define PIXEL_SIZE 5

// 144p
#define GRID_WIDTH 256
#define GRID_HEIGHT 144

// Define booleans since C does not have booleans
#define bool int
#define true 1
#define false 0

// Create custom buttons for saving
const SDL_MessageBoxButtonData save_buttons[] =
{
    { 0, 0, "Previous" },
    { 0, 1, "Current" }
};

const SDL_MessageBoxButtonData yesno_buttons[] =
{
    { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "No" },
    { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "Yes" }
};

// Create message box for saving/loading
const SDL_MessageBoxData save_msg_data =
{
    SDL_MESSAGEBOX_INFORMATION,
    NULL,
    "Saving",
    "Would you like to save the current or previous state?",
    SDL_arraysize(save_buttons),
    save_buttons,
    NULL
};

// Allocate memory for the cell grid and screen buffer
uint8_t cells[GRID_WIDTH * GRID_HEIGHT];
uint32_t buffer[WINDOW_WIDTH * WINDOW_HEIGHT];

uint8_t previous_simul[GRID_WIDTH * GRID_HEIGHT];

int main(int argc, char** argv)
{
    // If SDL is unable to initialize, return
    if (SDL_Init(SDL_INIT_EVERYTHING))
    {
        return -1;
    }

    // Simulation state
    bool s_started = false;

    // This is used to track how fast the simulation is going.
    // The speed can be changed with the scroll wheel
    int tick = 0;
    int max_tick = 1;

    // Initialize SDL windows, renderers, buffers, etc
    SDL_Window* window = SDL_CreateWindow("Game of Life", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture* window_buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

    NFD_Init();

    SDL_Event event;

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Drawing to the screen will be done by indirectly modifying the screen buffer.
    // We are creating a texture with streaming access, window_buffer, so that way we can change
    // the pixels in the texture individually. The buffer is the screen data, which
    // we copy to the window_buffer which then we copy to the renderer, which
    // renders the data.

    bool quit = false;

    // Might not be needed
    bool uselocktexture = false;

    // Keep track if the mouse buttons are down or up
    bool add_btn_down = false;
    bool rem_btn_down = false;
    
    // Main window loop
    while (!quit)
    {
        // Process events
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                quit = true;
                break;
            case SDL_KEYUP:
                // If key pressed is F1, start/stop simulation
                if (event.key.keysym.sym == SDLK_F1)
                {
                    s_started = !s_started;

                    // Save the current state, so we can save it to a file later
                    // if we so choose
                    if (s_started)
                    {
                        memcpy(previous_simul, cells, sizeof(cells));
                    }
                }
                else if (!s_started)
                {
                    // If key is F2, and the simulation hasnt started, clear all the cells
                    if (event.key.keysym.sym == SDLK_F2)
                    {
                        for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++)
                        {
                            cells[i] = 0;
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_F3)
                    {
                        // Ask user if they want to save the previous or current state
                        int btn;
                        
                        if (SDL_ShowMessageBox(&save_msg_data, &btn) < 0)
                        {
                            printf("Unable to display message box!\n");
                        }
                        else
                        {
                            // If saving current state, ask where to save it
                            nfdchar_t *save_path = NULL;
                            nfdfilteritem_t filter_items[1] = { { "Game of Life Simulation", "gol" } };
                            nfdresult_t result = NFD_SaveDialog(&save_path, filter_items, 1, NULL, "Simulation");

                            if (result == NFD_OKAY)
                            {
                                // Create/open the file
                                FILE *fp = fopen(save_path, "w+b");

                                // Write either the previous or current state, depending on what the
                                // user chose.
                                if (btn == 1)
                                    fwrite(cells, sizeof(uint8_t), sizeof(cells) / sizeof(uint8_t), fp);
                                else
                                    fwrite(previous_simul, sizeof(uint8_t), sizeof(previous_simul) / sizeof(uint8_t), fp);

                                // Close it to prevent any issues
                                fclose(fp);
                            }
                            
                            NFD_FreePath(save_path);
                        }
                    }

                    else if (event.key.keysym.sym == SDLK_F4)
                    {
                        // Create the dialog
                        nfdchar_t *simul_path = NULL;
                        nfdfilteritem_t filter_items[1] = { { "Game of Life Simulation", "gol" } };
                        nfdresult_t result = NFD_OpenDialog(&simul_path, filter_items, 1, NULL);

                        if (result == NFD_OKAY)
                        {
                            // Open the file, load the contents into cells, and close the file
                            FILE *fp = fopen(simul_path, "rb");

                            fread(cells, sizeof(uint32_t), sizeof(cells) / sizeof(uint32_t), fp);

                            fclose(fp);
                        }
                    }
                    else if (event.key.keysym.sym == SDLK_F5)
                    {
                        // Show help for the game
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Help", "F1 - Pause/start the simulation\nF2 - Clear the entire screen\nF3 - Save simulation to a file\nF4 - Load simulation from a file\n\nLeft Mouse - Draw cell\nRight Mouse - Remove cell\n\nScroll Wheel Up - Increase simulation speed\nScroll Wheel Down - Decrease simulation speed", window);
                    }
                }
                break;
            case SDL_MOUSEBUTTONDOWN:
                // If buttons are down, set their respective state true
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    add_btn_down = true;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    rem_btn_down = true;
                }
                break;
            case SDL_MOUSEBUTTONUP:
                // If buttons are up, set their respective state false
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    add_btn_down = false;
                }
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    rem_btn_down = false;
                }
                break;
            case SDL_MOUSEWHEEL:
                // Mouse wheel up
                if (event.wheel.y < 0)
                {
                    // Increase speed
                    max_tick++;
                }
                // Mouse wheel down
                else if (event.wheel.y > 0)
                {
                    // Decrease speed, prevent it from going negative
                    max_tick--;

                    if (max_tick <= 0)
                    {
                        max_tick = 1;
                    }
                }
                break;
            }
        }

        // Draw every cell if it is alive. Each cell is 5x5 pixels
        for (int i = 0; i < GRID_WIDTH * GRID_HEIGHT; i++)
        {
            int cx = (i % GRID_WIDTH) * PIXEL_SIZE;
            int cy = (i / GRID_WIDTH) * PIXEL_SIZE;

            for (int x = cx; x < cx + PIXEL_SIZE; x++)
            {
                for (int y = cy; y < cy + PIXEL_SIZE; y++)
                {
                    if (cells[i] & CELL_ALIVE)
                    {
                        buffer[x + WINDOW_WIDTH * y] = UINT32_MAX;
                    }
                    else
                    {
                        buffer[(x + WINDOW_WIDTH * y)] = 0;
                    }
                }
            }
        }

        // Draw cells if buttons are pressed
        if (!s_started)
        {   
            // Get x, y pos
            int x, y;
            SDL_GetMouseState(&x, &y);

            // Get grid position from window position
            int cx, cy;
            cx = (x / PIXEL_SIZE);
            cy = (y / PIXEL_SIZE);

            if (add_btn_down)
            {
                // Draw cells if the left mouse button is pressed
                cells[cx + GRID_WIDTH * cy] = CELL_ALIVE;
            }
            else if (rem_btn_down)
            {
                // Delete cells if the right mouse button is presesd
                cells[cx + GRID_WIDTH * cy] = 0;
            }
        }

        // Update simulation
        if (s_started)
        {
            // Used for setting simulation speed
            if (tick > max_tick)
            {
                // Looping through each cell
                for (int y = 0; y < GRID_HEIGHT; y++)
                {
                    for (int x = 0; x < GRID_WIDTH; x++)
                    {
                        uint8_t live_neighbors = 0;

                        // Check for any live neighbors, if any.
                        // The reason for a lot of if statements is for border-checking
                        if (x != 0 && y != 0)
                            if (cells[(x - 1) + GRID_WIDTH * (y - 1)] & CELL_ALIVE) live_neighbors++;
                        if (y != 0)
                            if (cells[(x) + GRID_WIDTH * (y - 1)] & CELL_ALIVE) live_neighbors++;
                        if (y != 0 && x != GRID_WIDTH - 1)
                            if (cells[(x + 1) + GRID_WIDTH * (y - 1)] & CELL_ALIVE) live_neighbors++;

                        if (x != 0)
                            if (cells[(x - 1) + GRID_WIDTH * (y)] & CELL_ALIVE) live_neighbors++;
                        if (x != GRID_WIDTH - 1)
                            if (cells[(x + 1) + GRID_WIDTH * (y)] & CELL_ALIVE) live_neighbors++;

                        if (x != 0 && y != GRID_HEIGHT - 1)
                            if (cells[(x - 1) + GRID_WIDTH * (y + 1)] & CELL_ALIVE) live_neighbors++;
                        if (y != GRID_HEIGHT - 1)
                            if (cells[(x) + GRID_WIDTH * (y + 1)] & CELL_ALIVE) live_neighbors++;
                        if (x != GRID_WIDTH - 1 && y != GRID_HEIGHT - 1)
                            if (cells[(x + 1) + GRID_WIDTH * (y + 1)] & CELL_ALIVE) live_neighbors++;

                        // If there are 3 neighbors around current cell, is revived
                        // If there are 2 neighbors, and the cell is alive, cell is healthy
                        if (live_neighbors == 2 || live_neighbors == 3)
                        {
                            if (cells[x + GRID_WIDTH * y] & CELL_ALIVE)
                            {
                                continue;
                            }

                            if (live_neighbors == 3)
                            {
                                cells[x + GRID_WIDTH * y] |= CELL_REVIVE;
                            }
                        }
                        // Die of under/over population
                        else
                        {
                            cells[x + GRID_WIDTH * y] |= CELL_DIE;
                        }
                    }
                }

                // Update cells
                for (int y = 0; y < GRID_HEIGHT; y++)
                {
                    for (int x = 0; x < GRID_WIDTH; x++)
                    {
                        if (cells[x + GRID_WIDTH * y] & CELL_REVIVE)
                        {
                            cells[x + GRID_WIDTH * y] = CELL_ALIVE;
                        }
                        else if (cells[x + GRID_WIDTH * y] & CELL_DIE)
                        {
                            cells[x + GRID_WIDTH * y] = 0;
                        }
                    }
                }

                // Reset ticks
                tick = 0;
            }
        }

        // Copy the screen buffer to the window buffer
        uint32_t* locked_pixels;
        int pitch;

        SDL_LockTexture(window_buffer, NULL, (void**)&locked_pixels, &pitch);
        SDL_memcpy(locked_pixels, buffer, WINDOW_WIDTH * WINDOW_HEIGHT * 4);
        SDL_UnlockTexture(window_buffer);

        // Copy the window buffer to the renderer to render it
        SDL_RenderCopy(renderer, window_buffer, NULL, NULL);
        SDL_RenderPresent(renderer);
        
        // Finally, update ticks
        tick++;
    }

    // Clean up
    SDL_DestroyTexture(window_buffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    NFD_Quit();

    return 0;
}