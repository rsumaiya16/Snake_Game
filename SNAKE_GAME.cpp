#include <SDL2/SDL.h>
#include "SDL_ttf.h"
#include <iostream>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <string>

// Screen dimension constants
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int SNAKE_SIZE = 25;
const int OBSTACLE_SIZE = 50; // New obstacle size

// Direction enum
enum Direction { UP, DOWN, LEFT, RIGHT };

// Game state enum
enum GameState { MENU, PLAYING, GAME_OVER, PAUSED, LEVEL_UP, COUNTDOWN };


// Snake segment structure
struct SnakeSegment {
    int x, y;
};

struct Color {
    Uint8 r, g, b, a;
};

// New snake structure for the random-moving snake
struct RandomSnake {
    std::vector<SnakeSegment> segments;
    Direction direction;
    Uint32 lastMoveTime;
    int moveInterval;
};

SDL_Texture* backgroundTexture = nullptr;
SDL_Texture* appleTexture = nullptr;
SDL_Texture* gameOverBackgroundTexture = nullptr;
SDL_Texture* pauseBackgroundTexture = nullptr;
SDL_Texture* startBackgroundTexture = nullptr; // New start background texture
SDL_Texture* stoneTexture = nullptr; // New stone texture
SDL_Texture* bananaTexture = nullptr; // New banana texture

Color startColor = {0, 204, 0, 255}; // Green
Color endColor = {0, 102, 0, 255};   // Darker Green

Color calculateGradientColor(const Color& start, const Color& end, float t) {
    Color result;
    result.r = start.r + t * (end.r - start.r);
    result.g = start.g + t * (end.g - start.g);
    result.b = start.b + t * (end.b - start.b);
    result.a = start.a + t * (end.a - start.a);
    return result;
}

// Inline max function
inline int customMax(int a, int b) {
    return (a > b) ? a : b;
}

// Function to initialize SDL
bool init(SDL_Window*& window, SDL_Renderer*& renderer, TTF_Font*& font) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }

    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_Quit();
        return false;
    }

    window = SDL_CreateWindow("Snake Game by sumuuu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        TTF_Quit();
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        TTF_Quit();
        return false;
    }

    font = TTF_OpenFont("/Library/Fonts/Arial Unicode.ttf", 24);
    if (!font) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        TTF_Quit();
        return false;
    }

    return true;
}

SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path) {
    SDL_Surface* loadedSurface = SDL_LoadBMP(path.c_str());
    if (!loadedSurface) {
        std::cerr << "Unable to load image " << path << "! SDL Error: " << SDL_GetError() << std::endl;
        return nullptr;
    }
    SDL_Texture* newTexture = SDL_CreateTextureFromSurface(renderer, loadedSurface);
    SDL_FreeSurface(loadedSurface);
    return newTexture;
}

bool loadMedia(SDL_Renderer* renderer) {
    // Load BMP texture for background
    backgroundTexture = loadTexture(renderer, "background.bmp");
    if (!backgroundTexture) {
        return false;
    }
    // Load apple texture
    appleTexture = loadTexture(renderer, "apple.bmp");
    if (!appleTexture) {
        return false;
    }
    // Load game over background texture
    gameOverBackgroundTexture = loadTexture(renderer, "background2.bmp");
    if (!gameOverBackgroundTexture) {
        return false;
    }
    // Load pause background texture
    pauseBackgroundTexture = loadTexture(renderer, "background2.bmp");
    if (!pauseBackgroundTexture) {
        return false;
    }
    // Load start background texture
    startBackgroundTexture = loadTexture(renderer, "background2.bmp");
    if (!startBackgroundTexture) {
        return false;
    }
    // Load stone texture
    stoneTexture = loadTexture(renderer, "stone.bmp");
    if (!stoneTexture) {
        return false;
    }
    // Load banana texture
    bananaTexture = loadTexture(renderer, "banana.bmp");
    if (!bananaTexture) {
        return false;
    }
    return true;
}

void close(SDL_Window* window, SDL_Renderer* renderer, TTF_Font* font) {
    TTF_CloseFont(font);
    SDL_DestroyTexture(backgroundTexture);
    SDL_DestroyTexture(appleTexture);
    SDL_DestroyTexture(gameOverBackgroundTexture);
    SDL_DestroyTexture(pauseBackgroundTexture);
    SDL_DestroyTexture(startBackgroundTexture);
    SDL_DestroyTexture(stoneTexture);
    SDL_DestroyTexture(bananaTexture); // Destroy banana texture
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}

// Function to render the snake with gradient color, border, eye, and tongue
void renderSnake(SDL_Renderer* renderer, const std::vector<SnakeSegment>& snake) {
    int numSegments = snake.size();
    for (int i = 0; i < numSegments; ++i) {
        float t = static_cast<float>(i) / (numSegments - 1);
        Color currentColor = calculateGradientColor(startColor, endColor, t);

        // Draw segment with gradient color
        SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);
        SDL_Rect fillRect = { snake[i].x, snake[i].y, SNAKE_SIZE, SNAKE_SIZE };
        SDL_RenderFillRect(renderer, &fillRect);

        // Draw border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black color for border
        SDL_RenderDrawRect(renderer, &fillRect);

        // Draw eye on the head
        if (i == 0) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for eye
            SDL_Rect eyeRect = { snake[i].x + SNAKE_SIZE / 4, snake[i].y + SNAKE_SIZE / 4, SNAKE_SIZE / 5, SNAKE_SIZE / 5 };
            SDL_RenderFillRect(renderer, &eyeRect);

            // Draw the tongue
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for tongue
            SDL_Rect tongueRect = { snake[i].x + SNAKE_SIZE / 2, snake[i].y + SNAKE_SIZE, SNAKE_SIZE / 5, SNAKE_SIZE / 2 };
            SDL_RenderFillRect(renderer, &tongueRect);
        }
    }
}

// Function to render the food
void renderFood(SDL_Renderer* renderer, int foodX, int foodY) {
    SDL_Rect destRect = { foodX, foodY, SNAKE_SIZE, SNAKE_SIZE };
    SDL_RenderCopy(renderer, appleTexture, nullptr, &destRect);
}

// Function to render the banana
void renderBanana(SDL_Renderer* renderer, int bananaX, int bananaY) {
    SDL_Rect destRect = { bananaX, bananaY, SNAKE_SIZE, SNAKE_SIZE };
    SDL_RenderCopy(renderer, bananaTexture, nullptr, &destRect);
}

// Function to render the obstacles
void renderObstacles(SDL_Renderer* renderer, const std::vector<SDL_Rect>& obstacles) {
    for (const auto& obstacle : obstacles) {
        SDL_RenderCopy(renderer, stoneTexture, nullptr, &obstacle);
    }
}

// Function to render the score
void renderScore(SDL_Renderer* renderer, TTF_Font* font, int score) {
    SDL_Color textColor = { 0, 0, 0, 255 }; // Black color
    std::string scoreText = "Score: " + std::to_string(score);
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, scoreText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    SDL_FreeSurface(textSurface);
    SDL_Rect renderQuad = { 10, 10, textWidth, textHeight };
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
    SDL_DestroyTexture(textTexture);
}

// Function to render game over message
void renderGameOver(SDL_Renderer* renderer, TTF_Font* font, int score) {
    SDL_Color textColor = { 0, 0, 0, 255 }; // Black color for text
    std::string gameOverText = "Game Over!! Final Score: " + std::to_string(score);

    // Create text surface and texture
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, gameOverText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    SDL_FreeSurface(textSurface);

    // Define the rectangle for the game over text box background
    int boxWidth = textWidth + 60;
    int boxHeight = textHeight + 60;
    SDL_Rect backgroundQuad = { (SCREEN_WIDTH - boxWidth) / 2, (SCREEN_HEIGHT - boxHeight) / 2, boxWidth, boxHeight };

    // Define the rectangle for the text
    SDL_Rect renderQuad = { (SCREEN_WIDTH - textWidth) / 2, (SCREEN_HEIGHT - textHeight) / 2, textWidth, textHeight };

    // Render the custom background
    SDL_RenderCopy(renderer, gameOverBackgroundTexture, nullptr, &backgroundQuad);

    // Render the text on top of the custom background
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);

    // Destroy the texture
    SDL_DestroyTexture(textTexture);
}

// Function to render pause message
void renderPause(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color textColor = { 0, 0, 0, 255 }; // Black color for text
    std::string pauseText = "Game Paused. Press 'P' to resume.";

    // Create text surface and texture
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, pauseText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    SDL_FreeSurface(textSurface);

    // Define the rectangle for the pause text box background
    int boxWidth = textWidth + 76;
    int boxHeight = textHeight + 76;
    SDL_Rect backgroundQuad = { (SCREEN_WIDTH - boxWidth) / 2, (SCREEN_HEIGHT - boxHeight) / 2, boxWidth, boxHeight };

    // Define the rectangle for the text
    SDL_Rect renderQuad = { (SCREEN_WIDTH - textWidth) / 2, (SCREEN_HEIGHT - textHeight) / 2, textWidth, textHeight };

    // Render the custom background
    SDL_RenderCopy(renderer, pauseBackgroundTexture, nullptr, &backgroundQuad);

    // Render the text on top of the custom background
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);

    // Destroy the texture
    SDL_DestroyTexture(textTexture);
}

// Function to render the start screen
void renderStartScreen(SDL_Renderer* renderer, TTF_Font* font) {
    SDL_Color textColor = { 0, 0, 0, 255 }; // Black color for text
    std::string startText = "Press 'Enter' to Start";

    // Create text surface and texture
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, startText.c_str(), textColor);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    int textWidth = textSurface->w;
    int textHeight = textSurface->h;
    SDL_FreeSurface(textSurface);

    // Define the rectangle for the start text box background
    int boxWidth = textWidth + 60;
    int boxHeight = textHeight + 60;
    SDL_Rect backgroundQuad = { (SCREEN_WIDTH - boxWidth) / 2, (SCREEN_HEIGHT - boxHeight) / 2, boxWidth, boxHeight };

    // Define the rectangle for the text
    SDL_Rect renderQuad = { (SCREEN_WIDTH - textWidth) / 2, (SCREEN_HEIGHT - textHeight) / 2, textWidth, textHeight };

    // Render the custom background
    SDL_RenderCopy(renderer, startBackgroundTexture, nullptr, &backgroundQuad);

    // Render the text on top of the custom background
    SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);

    // Destroy the texture
    SDL_DestroyTexture(textTexture);
}

void renderLevelUp(SDL_Renderer* renderer, TTF_Font* font, const std::string& message) {
    SDL_Color textColor = { 0, 0, 0, 255 }; // Black color for text
    std::string levelUpText1 = "Congo!! You are on " + message;
    std::string levelUpText2 = message == "level 2" ? "Be aware of the RUSSELL's VIPER SNAKE." : "Be aware of the stone.";

    // Create text surface and texture for first line
    SDL_Surface* textSurface1 = TTF_RenderText_Blended(font, levelUpText1.c_str(), textColor);
    SDL_Texture* textTexture1 = SDL_CreateTextureFromSurface(renderer, textSurface1);
    int textWidth1 = textSurface1->w;
    int textHeight1 = textSurface1->h;

    // Create text surface and texture for second line
    SDL_Surface* textSurface2 = TTF_RenderText_Blended(font, levelUpText2.c_str(), textColor);
    SDL_Texture* textTexture2 = SDL_CreateTextureFromSurface(renderer, textSurface2);
    int textWidth2 = textSurface2->w;
    int textHeight2 = textSurface2->h;

    // Define the rectangle for the level up text box background
    int boxWidth = customMax(textWidth1, textWidth2) + 80;
    int boxHeight = textHeight1 + textHeight2 + 80;
    SDL_Rect backgroundQuad = { (SCREEN_WIDTH - boxWidth) / 2, (SCREEN_HEIGHT - boxHeight) / 2, boxWidth, boxHeight };

    // Define the rectangles for the text
    SDL_Rect renderQuad1 = { (SCREEN_WIDTH - textWidth1) / 2, (SCREEN_HEIGHT - boxHeight) / 2 + 20, textWidth1, textHeight1 };
    SDL_Rect renderQuad2 = { (SCREEN_WIDTH - textWidth2) / 2, (SCREEN_HEIGHT - boxHeight) / 2 + textHeight1 + 40, textWidth2, textHeight2 };

    // Render the custom background box
    SDL_RenderCopy(renderer, gameOverBackgroundTexture, nullptr, &backgroundQuad);

    // Render the text on top of the custom background box
    SDL_RenderCopy(renderer, textTexture1, nullptr, &renderQuad1);
    SDL_RenderCopy(renderer, textTexture2, nullptr, &renderQuad2);

    // Destroy the textures and surfaces
    SDL_DestroyTexture(textTexture1);
    SDL_FreeSurface(textSurface1);
    SDL_DestroyTexture(textTexture2);
    SDL_FreeSurface(textSurface2);
}

// Function to render countdown timer
void renderCountdownTimer(SDL_Renderer* renderer, TTF_Font* font, Uint32 countdownStartTime, Uint32 countdownDuration) {
    Uint32 currentTime = SDL_GetTicks();
    Uint32 elapsedTime = currentTime - countdownStartTime;
    Uint32 remainingTime = countdownDuration - elapsedTime;

    if (remainingTime > 0) {
        SDL_Color textColor = { 0, 0, 0, 255 }; // black color for timer
        std::string timerText = "Resuming in: " + std::to_string(remainingTime / 1000) + "s";
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, timerText.c_str(), textColor);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        int textWidth = textSurface->w;
        int textHeight = textSurface->h;
        SDL_FreeSurface(textSurface);
        SDL_Rect renderQuad = { (SCREEN_WIDTH - textWidth) / 2, (SCREEN_HEIGHT - textHeight) / 2, textWidth, textHeight };
        SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
        SDL_DestroyTexture(textTexture);
    }
}



// Function to render banana timer
void renderBananaTimer(SDL_Renderer* renderer, TTF_Font* font, Uint32 bananaSpawnTime, Uint32 bananaLifetime) {
    Uint32 currentTime = SDL_GetTicks();
    Uint32 elapsedTime = currentTime - bananaSpawnTime;
    Uint32 remainingTime = bananaLifetime - elapsedTime;

    if (remainingTime > 0) {
        SDL_Color textColor = { 0, 0, 0, 255 }; // black color for timer
        std::string timerText = "Banana disappears in: " + std::to_string(remainingTime / 1000) + "s";
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, timerText.c_str(), textColor);
        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        int textWidth = textSurface->w;
        int textHeight = textSurface->h;
        SDL_FreeSurface(textSurface);
        SDL_Rect renderQuad = { SCREEN_WIDTH - textWidth - 10, 10, textWidth, textHeight };
        SDL_RenderCopy(renderer, textTexture, nullptr, &renderQuad);
        SDL_DestroyTexture(textTexture);
    }
}

// Function to render the new random-moving snake
void renderRandomSnake(SDL_Renderer* renderer, const RandomSnake& randomSnake) {
    int numSegments = randomSnake.segments.size();
    for (int i = 0; i < numSegments; ++i) {
        float t = static_cast<float>(i) / (numSegments - 1);
        Color currentColor = calculateGradientColor({255, 165, 0, 255}, {255, 140, 0, 255}, t); // Gradient from orange to darker orange

        // Draw segment with gradient color
        SDL_SetRenderDrawColor(renderer, currentColor.r, currentColor.g, currentColor.b, currentColor.a);
        SDL_Rect fillRect = { randomSnake.segments[i].x, randomSnake.segments[i].y, SNAKE_SIZE, SNAKE_SIZE };
        SDL_RenderFillRect(renderer, &fillRect);

        // Draw border
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black color for border
        SDL_RenderDrawRect(renderer, &fillRect);

        // Draw eye on the head
        if (i == 0) {
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for eye
            SDL_Rect eyeRect = { randomSnake.segments[i].x + SNAKE_SIZE / 4, randomSnake.segments[i].y + SNAKE_SIZE / 4, SNAKE_SIZE / 5, SNAKE_SIZE / 5 };
            SDL_RenderFillRect(renderer, &eyeRect);

            // Draw the tongue
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for tongue
            SDL_Rect tongueRect = { randomSnake.segments[i].x + SNAKE_SIZE / 2, randomSnake.segments[i].y + SNAKE_SIZE, SNAKE_SIZE / 5, SNAKE_SIZE / 2 };
            SDL_RenderFillRect(renderer, &tongueRect);
        }
    }
}

// Function to update the snake's position
void updateSnake(std::vector<SnakeSegment>& snake, Direction direction, bool& grow) {
    SnakeSegment newHead = snake.front();
    switch (direction) {
        case UP: newHead.y -= SNAKE_SIZE; break;
        case DOWN: newHead.y += SNAKE_SIZE; break;
        case LEFT: newHead.x -= SNAKE_SIZE; break;
        case RIGHT: newHead.x += SNAKE_SIZE; break;
    }
    snake.insert(snake.begin(), newHead);
    if (!grow) {
        snake.pop_back();
    } else {
        grow = false;
    }
}

// Function to update the random-moving snake's position
void updateRandomSnake(RandomSnake& randomSnake, const std::vector<SDL_Rect>& obstacles, Uint32 currentTime) {
    if (currentTime - randomSnake.lastMoveTime > randomSnake.moveInterval) {
        // Randomly change direction with some probability
        if (rand() % 4 == 0) { // Adjust this value to change the frequency of direction changes
            randomSnake.direction = static_cast<Direction>(rand() % 4);
        }

        // Update the snake's position based on the current direction
        SnakeSegment newHead = randomSnake.segments.front();
        switch (randomSnake.direction) {
            case UP: newHead.y -= SNAKE_SIZE; break;
            case DOWN: newHead.y += SNAKE_SIZE; break;
            case LEFT: newHead.x -= SNAKE_SIZE; break;
            case RIGHT: newHead.x += SNAKE_SIZE; break;
        }

        // Check for collisions with obstacles and screen boundaries
        if (newHead.x < 0) newHead.x = SCREEN_WIDTH - SNAKE_SIZE;
        else if (newHead.x >= SCREEN_WIDTH) newHead.x = 0;
        if (newHead.y < 0) newHead.y = SCREEN_HEIGHT - SNAKE_SIZE;
        else if (newHead.y >= SCREEN_HEIGHT) newHead.y = 0;

        bool collision = false;
        for (const auto& obstacle : obstacles) {
            if (newHead.x < obstacle.x + obstacle.w && newHead.x + SNAKE_SIZE > obstacle.x &&
                newHead.y < obstacle.y + obstacle.h && newHead.y + SNAKE_SIZE > obstacle.y) {
                collision = true;
                break;
            }
        }

        if (!collision) {
            randomSnake.segments.insert(randomSnake.segments.begin(), newHead);
            randomSnake.segments.pop_back();
            randomSnake.lastMoveTime = currentTime;
        }
    }
}


// Function to check collision with the food
bool checkFoodCollision(int foodX, int foodY, const SnakeSegment& head) {
    return head.x == foodX && head.y == foodY;
}

// Function to check collision with the banana
bool checkBananaCollision(int bananaX, int bananaY, const SnakeSegment& head) {
    return head.x == bananaX && head.y == bananaY;
}

// Function to check collision with the random-moving snake
bool checkRandomSnakeCollision(const std::vector<SnakeSegment>& snake, const RandomSnake& randomSnake) {
    const SnakeSegment& head = snake.front();
    for (const auto& segment : randomSnake.segments) {
        if (head.x == segment.x && head.y == segment.y) {
            return true;
        }
    }
    return false;
}

bool checkCollision(const std::vector<SnakeSegment>& snake, const std::vector<SDL_Rect>& obstacles) {
    const SnakeSegment& head = snake.front();
    if (head.x < 0 || head.x >= SCREEN_WIDTH || head.y < 0 || head.y >= SCREEN_HEIGHT) {
        return true;
    }
    for (size_t i = 1; i < snake.size(); ++i) {
        if (head.x == snake[i].x && head.y == snake[i].y) {
            return true;
        }
    }
    for (const auto& obstacle : obstacles) {
        // Check if the snake's head is within the bounds of the obstacle
        if (head.x < obstacle.x + obstacle.w && head.x + SNAKE_SIZE > obstacle.x &&
            head.y < obstacle.y + obstacle.h && head.y + SNAKE_SIZE > obstacle.y) {
            return true;
        }
    }
    return false;
}

// Function to generate food in a random position
void generateFood(int& foodX, int& foodY, const std::vector<SnakeSegment>& snake, const std::vector<SDL_Rect>& obstacles, const RandomSnake& randomSnake) {
    bool validPosition = false;
    while (!validPosition) {
        validPosition = true;
        foodX = (rand() % (SCREEN_WIDTH / SNAKE_SIZE)) * SNAKE_SIZE;
        foodY = (rand() % (SCREEN_HEIGHT / SNAKE_SIZE)) * SNAKE_SIZE;
        for (const auto& segment : snake) {
            if (segment.x == foodX && segment.y == foodY) {
                validPosition = false;
                break;
            }
        }
        for (const auto& obstacle : obstacles) {
            if (obstacle.x == foodX && obstacle.y == foodY) {
                validPosition = false;
                break;
            }
        }
        for (const auto& segment : randomSnake.segments) {
            if (segment.x == foodX && segment.y == foodY) {
                validPosition = false;
                break;
            }
        }
    }
}

// Function to generate banana in a random position
void generateBanana(int& bananaX, int& bananaY, const std::vector<SnakeSegment>& snake, const std::vector<SDL_Rect>& obstacles, const RandomSnake& randomSnake) {
    bool validPosition = false;
    while (!validPosition) {
        validPosition = true;
        bananaX = (rand() % (SCREEN_WIDTH / SNAKE_SIZE)) * SNAKE_SIZE;
        bananaY = (rand() % (SCREEN_HEIGHT / SNAKE_SIZE)) * SNAKE_SIZE;
        for (const auto& segment : snake) {
            if (segment.x == bananaX && segment.y == bananaY) {
                validPosition = false;
                break;
            }
        }
        for (const auto& obstacle : obstacles) {
            if (obstacle.x == bananaX && obstacle.y == bananaY) {
                validPosition = false;
                break;
            }
        }
        for (const auto& segment : randomSnake.segments) {
            if (segment.x == bananaX && segment.y == bananaY) {
                validPosition = false;
                break;
            }
        }
    }
}

// Function to generate obstacles
void generateObstacles(std::vector<SDL_Rect>& obstacles, const std::vector<SnakeSegment>& snake) {
    int numObstacles = 3; // Number of obstacles to generate
    obstacles.clear();
    for (int i = 0; i < numObstacles; ++i) {
        SDL_Rect newObstacle;
        bool validPosition = false;
        while (!validPosition) {
            validPosition = true;
            newObstacle.x = (rand() % (SCREEN_WIDTH / OBSTACLE_SIZE)) * OBSTACLE_SIZE;
            newObstacle.y = (rand() % (SCREEN_HEIGHT / OBSTACLE_SIZE)) * OBSTACLE_SIZE;
            newObstacle.w = OBSTACLE_SIZE;
            newObstacle.h = OBSTACLE_SIZE;
            for (const auto& segment : snake) {
                if (segment.x == newObstacle.x && segment.y == newObstacle.y) {
                    validPosition = false;
                    break;
                }
            }
            for (const auto& obstacle : obstacles) {
                if (obstacle.x == newObstacle.x && obstacle.y == newObstacle.y) {
                    validPosition = false;
                    break;
                }
            }
        }
        obstacles.push_back(newObstacle);
    }
}

// Function to handle events
void handleEvents(SDL_Event& e, Direction& direction, bool& quit, GameState& state) {
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit = true;
        } else if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_UP: if (direction != DOWN) direction = UP; break;
                case SDLK_DOWN: if (direction != UP) direction = DOWN; break;
                case SDLK_LEFT: if (direction != RIGHT) direction = LEFT; break;
                case SDLK_RIGHT: if (direction != LEFT) direction = RIGHT; break;
                case SDLK_p: if (state == PLAYING) state = PAUSED; else if (state == PAUSED) state = PLAYING; break; // Toggle pause state
                case SDLK_RETURN: if (state == MENU) state = PLAYING; break; // Start game from menu
            }
        }
    }
}

int main(int argc, char* args[]) {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    TTF_Font* font = nullptr;

    if (!init(window, renderer, font)) {
        std::cerr << "Failed to initialize!" << std::endl;
        return 1;
    }

    if (!loadMedia(renderer)) {
        std::cerr << "Failed to load media!" << std::endl;
        close(window, renderer, font);
        return 1;
    }

    srand(static_cast<unsigned int>(time(nullptr)));

    // Initialize game variables
    std::vector<SnakeSegment> snake = { {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2} };
    Direction direction = RIGHT;
    bool grow = false;
    int foodX, foodY;
    int bananaX, bananaY;
    Uint32 bananaSpawnTime = 0;
    bool bananaActive = false;
    const Uint32 bananaLifetime = 5000; // Banana lifetime in milliseconds
    std::vector<SDL_Rect> obstacles; // Vector to store obstacles
    int score = 0;
    bool quit = false;
    GameState state = MENU; // Start in the MENU state
    int initialSnakeSpeed = 130; // Initial snake speed
    int snakeSpeed = initialSnakeSpeed;
    int maxSnakeSpeed = 50; // Minimum delay (maximum speed)
    Uint32 levelUpStartTime = 0; // Time when the level up message was shown
    SDL_Event e;
    int pointsSinceLastBanana = 0; // Points since the last banana appeared
    bool levelUpTriggered = false; // Track if level up has been triggered
    std::string currentLevel = "level 1"; // Track the current level

    // Initialize random snake with random positions
    RandomSnake randomSnake;
    randomSnake.segments.clear();
    int startX = (rand() % (SCREEN_WIDTH / SNAKE_SIZE)) * SNAKE_SIZE;
    int startY = (rand() % (SCREEN_HEIGHT / SNAKE_SIZE)) * SNAKE_SIZE;
    for (int i = 0; i < 3; ++i) {
        randomSnake.segments.push_back({ startX + i * SNAKE_SIZE, startY });
    }
    randomSnake.direction = static_cast<Direction>(rand() % 4);
    randomSnake.lastMoveTime = SDL_GetTicks();
    randomSnake.moveInterval = 500; // Interval between movements in milliseconds
    bool randomSnakeActive = false;


    // Timer variables
    Uint32 countdownStartTime = 0;
    const Uint32 countdownDuration = 3000; // Countdown duration in milliseconds
    bool countdownActive = false;

    // Generate initial food position
    generateFood(foodX, foodY, snake, obstacles, randomSnake);
    std::cout << "Initial Food Position: (" << foodX << ", " << foodY << ")\n"; // Debug print

    // Main game loop
    while (!quit) {
        handleEvents(e, direction, quit, state);

        if (state == PLAYING) {
            // Update game logic
            Uint32 currentTime = SDL_GetTicks();
            updateSnake(snake, direction, grow);

            if (checkFoodCollision(foodX, foodY, snake.front())) {
                grow = true;
                score++;
                pointsSinceLastBanana++;
                generateFood(foodX, foodY, snake, obstacles, randomSnake);
                std::cout << "New Food Position: (" << foodX << ", " << foodY << ")\n"; // Debug print

                // Trigger level up at specific scores
                if (!levelUpTriggered && score >= 8 && currentLevel == "level 1") {
                    state = LEVEL_UP;
                    levelUpStartTime = SDL_GetTicks();
                    levelUpTriggered = true;
                    currentLevel = "level 2"; // Move to level 2
                } else if (!levelUpTriggered && score >= 15 && currentLevel == "level 2") {
                    state = LEVEL_UP;
                    levelUpStartTime = SDL_GetTicks();
                    generateObstacles(obstacles, snake); // Generate obstacles on level up
                    levelUpTriggered = true;
                    currentLevel = "level 3"; // Move to level 3
                }

                // Activate random snake at level 2
                if (currentLevel == "level 2") {
                    randomSnakeActive = true;
                }
            }

            if (bananaActive && checkBananaCollision(bananaX, bananaY, snake.front())) {
                grow = true;
                score += 3;
                bananaActive = false; // Remove banana after being eaten
                pointsSinceLastBanana = 0;
            }

            if (checkCollision(snake, obstacles) || (randomSnakeActive && checkRandomSnakeCollision(snake, randomSnake))) {
                state = GAME_OVER;
            }

            // Adjust snake speed based on its length
            snakeSpeed = customMax(maxSnakeSpeed, initialSnakeSpeed - (snake.size() - 1) * 5);

            // Generate banana if score is 5 and banana is not active
            if (score >= 5 && pointsSinceLastBanana >= 3 && !bananaActive) {
                generateBanana(bananaX, bananaY, snake, obstacles, randomSnake);
                bananaSpawnTime = SDL_GetTicks();
                bananaActive = true;
            }

            // Remove banana after 5 seconds
            if (bananaActive && SDL_GetTicks() - bananaSpawnTime >= bananaLifetime) {
                bananaActive = false;
            }

            // Update random snake
            if (randomSnakeActive) {
                updateRandomSnake(randomSnake, obstacles, currentTime);
            }

            // Render game
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
            renderSnake(renderer, snake);
            renderFood(renderer, foodX, foodY);
            if (bananaActive) {
                renderBanana(renderer, bananaX, bananaY); // Render banana if active
                renderBananaTimer(renderer, font, bananaSpawnTime, bananaLifetime); // Render banana timer if active
            }
            renderObstacles(renderer, obstacles); // Render obstacles
            renderScore(renderer, font, score);
            if (randomSnakeActive) {
                renderRandomSnake(renderer, randomSnake); // Render random snake if active
            }
            SDL_RenderPresent(renderer); // Ensure rendering during PLAYING state

        } else if (state == LEVEL_UP) {
            // Render level up message
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
            renderLevelUp(renderer, font, currentLevel);
            SDL_RenderPresent(renderer);

            // Start the countdown timer after displaying the level-up message
            if (!countdownActive) {
                countdownStartTime = SDL_GetTicks();
                countdownActive = true;
            }

            // Check if the level up message should disappear
            Uint32 currentTime = SDL_GetTicks();
            if (countdownActive && currentTime - countdownStartTime >= 3000) { // Show level-up message for 3 seconds
                countdownActive = false;
                state = COUNTDOWN; // Move to the countdown state
                countdownStartTime = SDL_GetTicks(); // Restart the countdown for the new state
            }

        } else if (state == COUNTDOWN) {
            // Render game elements
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
            renderSnake(renderer, snake);
            renderFood(renderer, foodX, foodY);
            if (bananaActive) {
                renderBanana(renderer, bananaX, bananaY); // Render banana if active
            }
            renderObstacles(renderer, obstacles); // Render obstacles
            renderScore(renderer, font, score);
            if (randomSnakeActive) {
                renderRandomSnake(renderer, randomSnake); // Render random snake if active
            }

            // Render the countdown timer
            renderCountdownTimer(renderer, font, countdownStartTime, countdownDuration);
            SDL_RenderPresent(renderer);

            // Check if the countdown has finished
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - countdownStartTime >= countdownDuration) { // Countdown duration in milliseconds
                state = PLAYING;
                levelUpTriggered = false; // Reset level-up trigger
            }

        } else if (state == GAME_OVER) {
            // Render game over text box over the main background
            SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
            renderSnake(renderer, snake);
            renderFood(renderer, foodX, foodY);
            renderScore(renderer, font, score);
            renderGameOver(renderer, font, score);
            SDL_RenderPresent(renderer); // Ensure rendering during GAME_OVER state

        } else if (state == PAUSED) {
            // Render pause text box over the main background
            SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
            renderSnake(renderer, snake);
            renderFood(renderer, foodX, foodY);
            renderScore(renderer, font, score);
            renderPause(renderer, font);
            SDL_RenderPresent(renderer); // Ensure rendering during PAUSED state

        } else if (state == MENU) {
            // Render start screen
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, backgroundTexture, nullptr, nullptr);
            renderStartScreen(renderer, font);
            SDL_RenderPresent(renderer); // Ensure rendering during MENU state
        }

        SDL_Delay(snakeSpeed); // Adjust snake speed based on length
    }

    close(window, renderer, font);
    return 0;
}
