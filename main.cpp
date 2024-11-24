#include <bits/stdc++.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
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
    int headX = snake.front().x;
    int headY = snake.front().y;

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

    if (checkCollision(snake, headX, headY)) {
        Mix_PlayChannel(-1, gameOverSound, 0);
        displayGameOver();
        running = false;
        return;
    }

    SnakeSegment newHead = {headX, headY};
    if (headX == food.x && headY == food.y) {
        Mix_PlayChannel(-1, eatSound, 0);
        snake.insert(snake.begin(), newHead);
        food.x = rand() % (screen_width / block_size);
        food.y = rand() % (screen_height / block_size);
        score++;
        updateScoreTexture();

        if (score % 5 == 0 && !bonusFoodActive) {
            spawnBonusFood(bonusFood, snake, food);
            bonusFoodActive = true;
        }
    } else if (bonusFoodActive && headX == bonusFood.x && headY == bonusFood.y) {
        Mix_PlayChannel(-1, eatSound, 0);
        snake.insert(snake.begin(), newHead);
        score += 10;
        updateScoreTexture();
        bonusFoodActive = false;
        bonusFood = {-1, -1};
    } else {
        snake.insert(snake.begin(), newHead);
        snake.pop_back();
    }
}

bool checkCollision(const vector<SnakeSegment>& snake, int x, int y) {
    for (size_t i = 1; i < snake.size(); ++i) {
        if (snake[i].x == x && snake[i].y == y) return true;
    }
    SDL_Rect headRect = {x * block_size, y * block_size, block_size, block_size};
    if (SDL_HasIntersection(&headRect, &obstacle1) ||
        SDL_HasIntersection(&headRect, &obstacle2) ||
        SDL_HasIntersection(&headRect, &obstacle3)) return true;
    return false;
}

void spawnBonusFood(SDL_Point& bonusFood, const vector<SnakeSegment>& snake, const SDL_Point& food) {
    bool valid;
    do {
        valid = true;
        bonusFood.x = rand() % (screen_width / block_size);
        bonusFood.y = rand() % (screen_height / block_size);

        for (const auto& segment : snake) {
            if (segment.x == bonusFood.x && segment.y == bonusFood.y) {
                valid = false;
                break;
            }
        }

        if (bonusFood.x == food.x && bonusFood.y == food.y) {
            valid = false;
        }

        SDL_Rect bonusRect = {bonusFood.x * block_size, bonusFood.y * block_size, block_size, block_size};
        if (SDL_HasIntersection(&bonusRect, &obstacle1) ||
            SDL_HasIntersection(&bonusRect, &obstacle2) ||
            SDL_HasIntersection(&bonusRect, &obstacle3)) {
            valid = false;
        }
    } while (!valid);
}

void updateScoreTexture() {
    string scoreText = "Score: " + to_string(score);
    SDL_Color textColor = {51, 51, 0, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, scoreText.c_str(), textColor);
    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    scoreTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    SDL_QueryTexture(scoreTexture, nullptr, nullptr, &scoreRect.w, &scoreRect.h);
}

void displayGameOver() {
    SDL_Color textColor = {255, 0, 0, 255};
    SDL_Surface* surface = TTF_RenderText_Solid(font, "GAME OVER", textColor);
    SDL_Texture* gameOverTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    int w, h;
    SDL_QueryTexture(gameOverTexture, nullptr, nullptr, &w, &h);
    SDL_Rect rect = {(screen_width - w) / 2, (screen_height - h) / 2, w, h};

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, gameOverTexture, nullptr, &rect);
    SDL_RenderPresent(renderer);

    SDL_Delay(3000);
    SDL_DestroyTexture(gameOverTexture);
}

void cleanup() {
    if (scoreTexture) SDL_DestroyTexture(scoreTexture);
    if (backgroundTexture) SDL_DestroyTexture(backgroundTexture);
    if (bgMusic) Mix_FreeMusic(bgMusic);
    if (eatSound) Mix_FreeChunk(eatSound);
    if (gameOverSound) Mix_FreeChunk(gameOverSound);
    if (font) TTF_CloseFont(font);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    TTF_Quit();
    Mix_CloseAudio();
    SDL_Quit();
}
