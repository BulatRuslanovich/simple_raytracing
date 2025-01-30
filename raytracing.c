#include <math.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <SDL2/SDL_ttf.h>

/* ========================== Конфигурация приложения ======================== */

#define SCREEN_WIDTH         1800    // Ширина окна
#define SCREEN_HEIGHT        800     // Высота окна
#define MAX_OBSTACLES        5       // Максимальное количество препятствий
#define BASE_RAY_COLOR       0xFFD43BFF  // Базовый цвет лучей
#define RAY_GLOW_RADIUS      60      // Радиус свечения
#define RAY_GLOW_INTENSITY   0.3f    // Интенсивность свечения

/* ============================ Структуры данных ============================= */

typedef struct {
    double x; // X-координата центра
    double y; // Y-координата центра
    double radius; // Радиус объекта
    double velocity_x; // Горизонтальная скорость
    double velocity_y; // Вертикальная скорость
    SDL_Color color; // Цвет отображения
    double radius_squared; // Квадрат радиуса (оптимизация вычислений)
} Obstacle;

typedef struct {
    double origin_x; // X-координата начала луча
    double origin_y; // Y-координата начала луча
    double direction_x; // X-компонент направления луча (нормализованный)
    double direction_y; // Y-компонент направления луча (нормализованный)
} Ray;

/* ========================== Глобальные настройки =========================== */

static struct {
    // Состояние приложения
    int obstacle_count; // Текущее количество препятствий
    int ray_count; // Количество генерируемых лучей
    int show_fps; // Флаг отображения FPS
    int show_help; // Флаг отображения справки
    int is_paused; // Флаг паузы симуляции

    // Системные ресурсы
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;

    // Кэш текстуры текста
    SDL_Texture *text_cache[5];
    char cached_text[5][256];
} app_state = {
    .obstacle_count = 2,
    .ray_count = 360,
    .show_fps = 1,
    .show_help = 1,
    .is_paused = 0
};

/* ========================== Вспомогательные функции ======================== */

// Генерация случайного цвета
static SDL_Color generate_random_color() {
    return (SDL_Color){
        .r = rand() % 200,
        .g = rand() % 200,
        .b = rand() % 200,
        .a = 255
    };
}

// Инициализация объекта препятствия
static void obstacle_init(Obstacle *obj) {
    // Базовые параметры
    obj->radius = 30 + rand() % 50;
    obj->radius_squared = obj->radius * obj->radius;
    obj->velocity_x = (rand() % 10 - 5) / 2.0;
    obj->velocity_y = (rand() % 10 - 5) / 2.0;
    obj->color = generate_random_color();

    obj->x = rand() % (SCREEN_WIDTH - 100) + 50;
    obj->y = rand() % (SCREEN_HEIGHT - 100) + 50;
}


/* ======================== Основная логика приложения ======================= */

// Обновление состояния препятствий
static void update_obstacles(Obstacle* obstacles, int count) {
    if (count > MAX_OBSTACLES || count < 0 || app_state.is_paused) return;

    for (int i = 0; i < count; ++i) {
        Obstacle* current = &obstacles[i];

        // Обновление позиции
        current->x += current->velocity_x;
        current->y += current->velocity_y;

        // Обработка столкновений с границами
        if (current->y < current->radius) {
            current->velocity_y = fabs(current->velocity_y);
        } else if (current->y > SCREEN_HEIGHT - current->radius) {
            current->velocity_y = -fabs(current->velocity_y);
        }

        if (current->x < current->radius) {
            current->velocity_x = fabs(current->velocity_x);
        } else if (current->x > SCREEN_WIDTH - current->radius) {
            current->velocity_x = -fabs(current->velocity_x);
        }

        // Проверка столкновений между объектами
        for (int j = i + 1; j < count; ++j) {
            Obstacle* other = &obstacles[j];
            const double dx = current->x - other->x;
            const double dy = current->y - other->y;
            const double min_distance = current->radius + other->radius;

            if ((dx*dx + dy*dy) < (min_distance * min_distance)) {
                // Обмен скоростями при столкновении
                const double temp_vx = current->velocity_x;
                const double temp_vy = current->velocity_y;
                current->velocity_x = other->velocity_x;
                current->velocity_y = other->velocity_y;
                other->velocity_x = temp_vx;
                other->velocity_y = temp_vy;
            }
        }
    }
}

// Генерация лучей из центрального объекта
static void generate_rays(const Obstacle* source, Ray* rays) {
    const double angle_step = 2 * M_PI / app_state.ray_count;

    for (int i = 0; i < app_state.ray_count; ++i) {
        const double angle = i * angle_step;
        rays[i].direction_x = cos(angle);
        rays[i].direction_y = sin(angle);
        rays[i].origin_x = source->x;
        rays[i].origin_y = source->y;
    }
}

/* ============================ Графические функции ========================== */

// Отрисовка заполненного круга
static void draw_filled_circle(const Obstacle* obj) {
    filledCircleColor(
        app_state.renderer,
        (int)obj->x,
        (int)obj->y,
        (int)obj->radius,
        SDL_MapRGBA(
            SDL_AllocFormat(SDL_PIXELFORMAT_RGBA8888),
            obj->color.r,
            obj->color.g,
            obj->color.b,
            obj->color.a
        )
    );
}

// Отрисовка текста с кэшированием
static void draw_text(const char* text, int x, int y) {
    static int cache_index = 0;
    SDL_Color white = {255, 255, 255, 255};

    // Поиск в кэше
    for (int i = 0; i < 5; ++i) {
        if (strcmp(app_state.cached_text[i], text) == 0 && app_state.text_cache[i]) {
            SDL_Rect rect = {x, y, 0, 0};
            SDL_QueryTexture(app_state.text_cache[i], NULL, NULL, &rect.w, &rect.h);
            SDL_RenderCopy(app_state.renderer, app_state.text_cache[i], NULL, &rect);
            return;
        }
    }

    // Создание новой текстуры
    SDL_Surface* surface = TTF_RenderText_Solid(app_state.font, text, white);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(app_state.renderer, surface);

    // Обновление кэша
    strncpy(app_state.cached_text[cache_index], text, 255);
    if (app_state.text_cache[cache_index]) {
        SDL_DestroyTexture(app_state.text_cache[cache_index]);
    }
    app_state.text_cache[cache_index] = texture;
    cache_index = (cache_index + 1) % 5;

    // Отрисовка
    SDL_Rect rect = {x, y, surface->w, surface->h};
    SDL_RenderCopy(app_state.renderer, texture, NULL, &rect);
    SDL_FreeSurface(surface);
}

// Отрисовка лучей
static void draw_rays(const Ray* rays, const Obstacle* obstacles) {
    for (int i = 0; i < app_state.ray_count; ++i) {
        const Ray* ray = &rays[i];
        double closest_intersection = INFINITY;

        // Поиск ближайшего пересечения с препятствиями
        for (int j = 0; j < app_state.obstacle_count; ++j) {
            const Obstacle* obj = &obstacles[j];
            const double dx = ray->origin_x - obj->x;
            const double dy = ray->origin_y - obj->y;

            // Решение квадратного уравнения
            const double a = ray->direction_x * ray->direction_x +
                           ray->direction_y * ray->direction_y;
            const double b = 2 * (dx * ray->direction_x + dy * ray->direction_y);
            const double c = dx*dx + dy*dy - obj->radius_squared;

            const double discriminant = b*b - 4*a*c;
            if (discriminant < 0) continue;

            const double sqrt_disc = sqrt(discriminant);
            const double t1 = (-b - sqrt_disc) / (2*a);
            const double t2 = (-b + sqrt_disc) / (2*a);

            if (t1 > 0 && t1 < closest_intersection) closest_intersection = t1;
            if (t2 > 0 && t2 < closest_intersection) closest_intersection = t2;
        }

        // Определение границ экрана
        const double screen_x = (ray->direction_x > 0)
            ? (SCREEN_WIDTH - ray->origin_x) / ray->direction_x
            : -ray->origin_x / ray->direction_x;

        const double screen_y = (ray->direction_y > 0)
            ? (SCREEN_HEIGHT - ray->origin_y) / ray->direction_y
            : -ray->origin_y / ray->direction_y;

        const double t = fmin(fmin(screen_x, screen_y), closest_intersection);
        if (t <= 0 || t > 10000) continue;

        // Отрисовка луча с градиентом
        const int step_size = 5;
        const int total_steps = (int)t;

        for (int step = 0; step < total_steps; step += step_size) {
            const int end_step = (step + step_size > total_steps)
                               ? total_steps
                               : step + step_size;

            const double ratio = (double)step / t;
            const Uint8 alpha = 255 * (1 - ratio);

            lineColor(
                app_state.renderer,
                ray->origin_x + ray->direction_x * step,
                ray->origin_y + ray->direction_y * step,
                ray->origin_x + ray->direction_x * end_step,
                ray->origin_y + ray->direction_y * end_step,
                (alpha << 24) | 0x00D43B
            );
        }
    }
}

/* ========================== Основная функция =============================== */

int main() {
    // Инициализация SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    if (TTF_Init() == -1) {
        fprintf(stderr, "SDL_ttf initialization failed: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    // Создание окна и рендерера
    app_state.window = SDL_CreateWindow(
        "Ray Tracing Demo",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!app_state.window) {
        fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    app_state.renderer = SDL_CreateRenderer(
        app_state.window,
        -1,
        SDL_RENDERER_ACCELERATED |
        SDL_RENDERER_PRESENTVSYNC |
        SDL_RENDERER_TARGETTEXTURE
    );

    if (!app_state.renderer) {
        fprintf(stderr, "Renderer creation failed: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    // Загрузка шрифта
    app_state.font = TTF_OpenFont(
        "/usr/share/fonts/liberation-sans-fonts/LiberationSans-Regular.ttf",
        24
    );

    if (!app_state.font) {
        fprintf(stderr, "Font loading failed: %s\n", TTF_GetError());
        exit(EXIT_FAILURE);
    }

    // Инициализация объектов
    Obstacle central = {
        .x = SCREEN_WIDTH / 2,
        .y = SCREEN_HEIGHT / 2,
        .radius = 40,
        .radius_squared = 1600,
        .color = {255, 255, 255, 255}
    };

    Obstacle obstacles[MAX_OBSTACLES];
    for (int i = 0; i < MAX_OBSTACLES; ++i) {
        obstacle_init(&obstacles[i]);
    }

    Ray* rays = malloc(app_state.ray_count * sizeof(Ray));
    if (!rays) {
        fprintf(stderr, "Memory allocation failed for rays\n");
        exit(EXIT_FAILURE);
    }

    generate_rays(&central, rays);

    Uint32 last_frame_time = SDL_GetTicks();
    Uint32 frame_counter = 0;
    char fps_text[32];
    int is_running = 1;

     while (is_running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                is_running = 0;
            }
            else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE: is_running = 0; break;
                    case SDLK_SPACE: app_state.is_paused ^= 1; break;
                    case SDLK_h: app_state.show_help ^= 1; break;
                    case SDLK_f: app_state.show_fps ^= 1; break;
                    case SDLK_EQUALS:
                        if (app_state.obstacle_count < MAX_OBSTACLES)
                            app_state.obstacle_count++;
                        break;
                    case SDLK_MINUS:
                        if (app_state.obstacle_count > 0)
                            app_state.obstacle_count--;
                        break;
                    case SDLK_r:
                        for (int i = 0; i < app_state.obstacle_count; ++i)
                            obstacle_init(&obstacles[i]);
                        break;
                }
            }
            else if (event.type == SDL_MOUSEMOTION && event.motion.state) {
                central.x = fmax(40, fmin(event.motion.x, SCREEN_WIDTH - 40));
                central.y = fmax(40, fmin(event.motion.y, SCREEN_HEIGHT - 40));
                generate_rays(&central, rays);
            }
        }

        // Обновление счетчика FPS
        const Uint32 current_time = SDL_GetTicks();
        frame_counter++;

        if (current_time - last_frame_time >= 1000) {
            const double fps = frame_counter / ((current_time - last_frame_time) / 1000.0);
            snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", fps);
            last_frame_time = current_time;
            frame_counter = 0;
        }

        // Очистка экрана
        SDL_SetRenderDrawColor(app_state.renderer, 0, 0, 0, 255);
        SDL_RenderClear(app_state.renderer);

        // Отрисовка объектов
        draw_rays(rays, obstacles);
        draw_filled_circle(&central);
        for (int i = 0; i < app_state.obstacle_count; ++i) {
            draw_filled_circle(&obstacles[i]);
        }

        // Отрисовка интерфейса
        if (app_state.show_fps) {
            draw_text(fps_text, 10, 10);
        }
        if (app_state.show_help) {
            draw_text("[+/-] Change obstacles count", 10, 40);
            draw_text("[R] Reset obstacles", 10, 70);
            draw_text("[SPACE] Pause simulation", 10, 100);
            draw_text("[F] Toggle FPS", 10, 130);
            draw_text("[H] Toggle help", 10, 160);
        }

        // Обновление состояния
        update_obstacles(obstacles, app_state.obstacle_count);
        SDL_RenderPresent(app_state.renderer);
    }

    // Освобождение ресурсов
    free(rays);
    TTF_CloseFont(app_state.font);
    SDL_DestroyRenderer(app_state.renderer);
    SDL_DestroyWindow(app_state.window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
