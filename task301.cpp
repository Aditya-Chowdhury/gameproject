#include <bits/stdc++.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <vector>
#include <cstdlib>

using namespace std;

// Constants
const int screen_width = 700;
const int screen_height = 500;
const int block_size = 20;

// Structures
struct SnakeSegment {
    int x, y;
};

// Obstacles
SDL_Rect obstacle1{100, 120, 200, 20};
SDL_Rect obstacle2{420, 360, 200, 20};
SDL_Rect obstacle3{300, 240, 100, 20};

// SDL Variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
TTF_Font* font = nullptr;
SDL_Texture* scoreTexture = nullptr;
SDL_Texture* backgroundTexture = nullptr;

// Audio
Mix_Music* bgMusic = nullptr;
Mix_Chunk* eatSound = nullptr;
Mix_Chunk* gameOverSound = nullptr;

// Score and State
int score = 0;
SDL_Rect scoreRect = {30, 30, 0, 0};
bool running = true;

// Function prototypes
void render(const vector<SnakeSegment>& snake, const SDL_Point& food, const SDL_Point& bonusFood);
void update(vector<SnakeSegment>& snake, SDL_Point& food, SDL_Point& bonusFood, SDL_Keycode& direction, bool& bonusFoodActive);
bool checkCollision(const vector<SnakeSegment>& snake, int x, int y);
bool handleObstacleCollision();
void spawnBonusFood(SDL_Point& bonusFood, const vector<SnakeSegment>& snake, const SDL_Point& food);
void updateScoreTexture();
void displayGameOver();
void cleanup();

int main(int argc, char* argv[]) {
    // Initialize SDL, SDL_ttf, and SDL_mixer
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0 || TTF_Init() != 0 || Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        cerr << "Initialization failed: " << SDL_GetError() << endl;
        return 1;
    }

    // Create window and renderer
    window = SDL_CreateWindow("Snake Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width, screen_height, SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Load font
    font = TTF_OpenFont("arial.ttf", 24);
    if (!font) {
        cerr << "Failed to load font: " << TTF_GetError() << endl;
        cleanup();
        return 1;
    }

    // Load background image
    SDL_Surface* backgroundSurface = SDL_LoadBMP("47412.bmp");
    if (!backgroundSurface) {
        cerr << "Failed to load background image: " << SDL_GetError() << endl;
        cleanup();
        return 1;
    }
    backgroundTexture = SDL_CreateTextureFromSurface(renderer, backgroundSurface);
    SDL_FreeSurface(backgroundSurface);

    // Load audio
    bgMusic = Mix_LoadMUS("audio.mp3");
    eatSound = Mix_LoadWAV("eating-sound-effect-36186.mp3");
    gameOverSound = Mix_LoadWAV("game-over-arcade-6435.mp3");
    if (!bgMusic || !eatSound || !gameOverSound) {
        cerr << "Failed to load audio: " << Mix_GetError() << endl;
        cleanup();
        return 1;
    }

    // Play background music
    Mix_PlayMusic(bgMusic, -1);

    // Initialize game objects
    vector<SnakeSegment> snake{{15, 15}};
    SDL_Point food = {10, 10};
    SDL_Point bonusFood = {-1, -1};
    SDL_Keycode direction = SDLK_RIGHT;
    bool bonusFoodActive = false;

    // Initialize score texture
    updateScoreTexture();

    // Main game loop
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP: if (direction != SDLK_DOWN) direction = SDLK_UP; break;
                    case SDLK_DOWN: if (direction != SDLK_UP) direction = SDLK_DOWN; break;
                    case SDLK_LEFT: if (direction != SDLK_RIGHT) direction = SDLK_LEFT; break;
                    case SDLK_RIGHT: if (direction != SDLK_LEFT) direction = SDLK_RIGHT; break;
                }
            }
        }

        update(snake, food, bonusFood, direction, bonusFoodActive);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
        render(snake, food, bonusFood);
        SDL_RenderCopy(renderer, scoreTexture, nullptr, &scoreRect);
        SDL_RenderPresent(renderer);

        SDL_Delay(100);
    }

    cleanup();
    return 0;
}

void render(const vector<SnakeSegment>& snake, const SDL_Point& food, const SDL_Point& bonusFood) {
    // Render snake
    for (const auto& segment : snake) {
        SDL_Rect rect = {segment.x * block_size, segment.y * block_size, block_size, block_size};
        SDL_SetRenderDrawColor(renderer, 0, 102, 204, 255);
        SDL_RenderFillRect(renderer, &rect);
    }

    // Render food
    SDL_Rect foodRect = {food.x * block_size, food.y * block_size, block_size, block_size};
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderFillRect(renderer, &foodRect);

    // Render bonus food
    if (bonusFood.x != -1 && bonusFood.y != -1) {
        SDL_Rect bonusFoodRect = {bonusFood.x * block_size, bonusFood.y * block_size, block_size, block_size};
        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRect(renderer, &bonusFoodRect);
    }

    // Render obstacles
    SDL_SetRenderDrawColor(renderer, 0, 51, 0, 255);
    SDL_RenderFillRect(renderer, &obstacle1);
    SDL_RenderFillRect(renderer, &obstacle2);
    SDL_RenderFillRect(renderer, &obstacle3);
}

void update(vector<SnakeSegment>& snake, SDL_Point& food, SDL_Point& bonusFood, SDL_Keycode& direction, bool& bonusFoodActive) {
    // Get the current position of the snake's head
    int headX = snake.front().x;
    int headY = snake.front().y;

    // Determine the new position based on the current direction
    switch (direction) {
        case SDLK_UP: headY--; break;
        case SDLK_DOWN: headY++; break;
        case SDLK_LEFT: headX--; break;
        case SDLK_RIGHT: headX++; break;
    }

    // Wrap around the screen
    if (headX < 0) headX = screen_width / block_size - 1;
    if (headX >= screen_width / block_size) headX = 0;
    if (headY < 0) headY = screen_height / block_size - 1;
    if (headY >= screen_height / block_size) headY = 0;

    // Collision with the snake itself
    if (checkCollision(snake, headX, headY)) {
        Mix_PlayChannel(-1, gameOverSound, 0);
        displayGameOver();
        running = false;
        return;
    }

    // Define the snake's head rectangle for collision checks
    SDL_Rect snakeHeadRect = {headX * block_size, headY * block_size, block_size, block_size};

    // Check collision with obstacles
    if (SDL_HasIntersection(&snakeHeadRect, &obstacle1) ||
        SDL_HasIntersection(&snakeHeadRect, &obstacle2) ||
        SDL_HasIntersection(&snakeHeadRect, &obstacle3)) {
        if (!handleObstacleCollision()) { // Handle pause decision
            running = false;
            return;
        }
    }

    // Create a new head segment for the snake
    SnakeSegment newHead = {headX, headY};

    // Check collision with food
    if (headX == food.x && headY == food.y) {
        Mix_PlayChannel(-1, eatSound, 0);
        snake.insert(snake.begin(), newHead); // Grow the snake
        food.x = rand() % (screen_width / block_size);
        food.y = rand() % (screen_height / block_size);
        score++; // Increase the score
        updateScoreTexture();

        // Spawn bonus food every 5 points
        if (score % 5 == 0 && !bonusFoodActive) {
            spawnBonusFood(bonusFood, snake, food);
            bonusFoodActive = true;
        }
    } else if (bonusFoodActive && headX == bonusFood.x && headY == bonusFood.y) {
        Mix_PlayChannel(-1, eatSound, 0);
        snake.insert(snake.begin(), newHead); // Grow the snake
        score += 10; // Increase the score significantly
        updateScoreTexture();
        bonusFoodActive = false; // Deactivate bonus food
        bonusFood = {-1, -1}; // Remove bonus food
    } else {
        snake.insert(snake.begin(), newHead); // Move the snake
        snake.pop_back(); // Remove the tail
    }
}

        

bool handleObstacleCollision() {
    bool resolved = false;
    SDL_Event event;

    // Render "Game Paused" message
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, "Game Paused! Press Y to continue (-10 points), N to Quit", white);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {screen_width / 4, screen_height / 2, surface->w, surface->h};
    SDL_FreeSurface(surface);
    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);

    while (!resolved) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return false;
            } else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_y) {
                    score -= 10;
                    updateScoreTexture();
                    resolved = true;
                } else if (event.key.keysym.sym == SDLK_n) {
                    return false;
                }
            }
        }
    }
    return true;
}

void spawnBonusFood(SDL_Point& bonusFood, const vector<SnakeSegment>& snake, const SDL_Point& food) {
    bool valid = false;
    while (!valid) {
        bonusFood.x = rand() % (screen_width / block_size);
        bonusFood.y = rand() % (screen_height / block_size);
        valid = !checkCollision(snake, bonusFood.x, bonusFood.y) && !(bonusFood.x == food.x && bonusFood.y == food.y);
    }
}

void updateScoreTexture() {
    if (scoreTexture) SDL_DestroyTexture(scoreTexture);

    SDL_Color white = {255, 255, 255, 255};
    string scoreText = "Score: " + to_string(score);
    SDL_Surface* surface = TTF_RenderText_Solid(font, scoreText.c_str(), white);
    scoreTexture = SDL_CreateTextureFromSurface(renderer, surface);
    scoreRect.w = surface->w;
    scoreRect.h = surface->h;
    SDL_FreeSurface(surface);
}

bool checkCollision(const vector<SnakeSegment>& snake, int x, int y) {
    for (const auto& segment : snake) {
        if (segment.x == x && segment.y == y) {
            return true;
        }
    }
    return false;
}

void displayGameOver() {
    SDL_Color white = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, "Game Over! Press Any Key to Exit", white);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_Rect rect = {screen_width / 4, screen_height / 2, surface->w, surface->h};
    SDL_FreeSurface(surface);

    SDL_RenderCopy(renderer, texture, nullptr, &rect);
    SDL_RenderPresent(renderer);
    SDL_DestroyTexture(texture);

    SDL_Event event;
    while (true) {
        SDL_WaitEvent(&event);
        if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN) {
            break;
        }
    }
}

void cleanup() {
    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (font) TTF_CloseFont(font);
    if (bgMusic) Mix_FreeMusic(bgMusic);
    if (eatSound) Mix_FreeChunk(eatSound);
    if (gameOverSound) Mix_FreeChunk(gameOverSound);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    Mix_Quit();
    SDL_Quit();
}
