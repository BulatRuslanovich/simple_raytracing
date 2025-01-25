#include <SDL2/SDL.h>
#include <math.h>
#include <stdio.h>

#define WIDTH 1800
#define HEIGHT 800
#define COLOR_WHITE 0xffffffff
#define COLOR_BLACK 0x00000000
#define COLOR_GRAY_LIGHT 0xbbbbbb
#define COLOR_RAY 0xffd43b
#define RAYS_NUMBER 1000
#define RAY_THICKNESS 1
#define COUNT_OF_SHADOW_CIRCLES 5

typedef struct {
  double x, y;
  double radius;
  double dx, dy;
  Uint32 color;
} Circle;

typedef struct {
  double x, y;
  double angle;
} Ray;

Uint32 random_color() {
  return (rand() % 256 << 16) | (rand() % 256 << 8) | (rand() % 256);
}

void FillCircle(SDL_Surface* surface, Circle circle) {
    int x = circle.radius;
    int y = 0;
    int radiusError = 1 - x;

    while (x >= y) {
        for (int i = -x; i <= x; i++) {
            SDL_Rect pixel = {circle.x + i, circle.y + y, 1, 1};
            SDL_FillRect(surface, &pixel, circle.color);
            pixel.y = circle.y - y;
            SDL_FillRect(surface, &pixel, circle.color);
        }
        for (int i = -y; i <= y; i++) {
            SDL_Rect pixel = {circle.x + i, circle.y + x, 1, 1};
            SDL_FillRect(surface, &pixel, circle.color);
            pixel.y = circle.y - x;
            SDL_FillRect(surface, &pixel, circle.color);
        }
        y++;

        if (radiusError < 0) {
            radiusError += 2 * y + 1;
        } else {
            x--;
            radiusError += 2 * (y - x + 1);
        }
    }
}


int check_collision(Circle a, Circle b) {
  double distance_sq = (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
  double radius_sum = a.radius + b.radius;
  return distance_sq <= (radius_sum * radius_sum);
}

void update_shadow_circles(Circle circles[COUNT_OF_SHADOW_CIRCLES]) {
  for (int i = 0; i < COUNT_OF_SHADOW_CIRCLES; i++) {
    circles[i].x += circles[i].dx;
    circles[i].y += circles[i].dy;

    if (circles[i].y - circles[i].radius < 0 ||
        circles[i].y + circles[i].radius > HEIGHT) {
      circles[i].dy *= -1;
    }

    if (circles[i].x - circles[i].radius < 0 ||
        circles[i].x + circles[i].radius > WIDTH) {
      circles[i].dx *= -1;
    }

    for (int j = i + 1; j < COUNT_OF_SHADOW_CIRCLES; j++) {
      if (check_collision(circles[i], circles[j])) {
        circles[i].dx *= -1;
        circles[i].dy *= -1;
        circles[j].dx *= -1;
        circles[j].dy *= -1;
      }
    }
  }
}

void generate_rays(Circle circle, Ray rays[RAYS_NUMBER]) {
  for (int i = 0; i < RAYS_NUMBER; i++) {
    rays[i].angle = (double)i / RAYS_NUMBER * 2 * M_PI;
    rays[i].x = circle.x;
    rays[i].y = circle.y;
  }
}

void FillRays(SDL_Surface* surface, Ray rays[RAYS_NUMBER], Uint32 color,
              Circle circles[COUNT_OF_SHADOW_CIRCLES]) {
  double radius_sqs[COUNT_OF_SHADOW_CIRCLES];

  for (int i = 0; i < COUNT_OF_SHADOW_CIRCLES; i++) {
    radius_sqs[i] = circles[i].radius * circles[i].radius;
  }

  for (int i = 0; i < RAYS_NUMBER; i++) {
    Ray ray = rays[i];
    double x_draw = ray.x;
    double y_draw = ray.y;
    double step = 1;
    int is_end = 0;

    while (!is_end) {
      x_draw += step * cos(ray.angle);
      y_draw += step * sin(ray.angle);

      if (x_draw < 0 || x_draw > WIDTH || y_draw < 0 || y_draw > HEIGHT) {
        is_end = 1;
        continue;
      }

      SDL_Rect ray_point =
          (SDL_Rect){x_draw, y_draw, RAY_THICKNESS, RAY_THICKNESS};
      SDL_FillRect(surface, &ray_point, color);

      for (int j = 0; j < COUNT_OF_SHADOW_CIRCLES; j++) {
        double distance_sq = (x_draw - circles[j].x) * (x_draw - circles[j].x) +
                             (y_draw - circles[j].y) * (y_draw - circles[j].y);
        if (distance_sq < radius_sqs[j]) {
          is_end = 1;
          break;
        }
      }
    }
  }
}

int main() {
  SDL_Init(SDL_INIT_VIDEO);
  SDL_Window* window =
      SDL_CreateWindow("Raytracing", SDL_WINDOWPOS_CENTERED,
                       SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
  SDL_Surface* surface = SDL_GetWindowSurface(window);

  Circle circle = {WIDTH / 2, HEIGHT / 2, 40, 0, 0, COLOR_WHITE};
  Circle shadow_circles[COUNT_OF_SHADOW_CIRCLES];

  for (int i = 0; i < COUNT_OF_SHADOW_CIRCLES; i++) {
    shadow_circles[i].x = rand() % 800;
    shadow_circles[i].y = rand() % 600;
    shadow_circles[i].radius = 30 + rand() % 20;
    shadow_circles[i].dx = (rand() % 10) - 5;
    shadow_circles[i].dy = (rand() % 10) - 5;
    shadow_circles[i].color = random_color();
  }

  Ray rays[RAYS_NUMBER];
  SDL_Rect erase_rect = (SDL_Rect){0, 0, WIDTH, HEIGHT};
  int simlulation_running = 1;
  SDL_Event event;

  generate_rays(circle, rays);

  while (simlulation_running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        simlulation_running = 0;
      }

      if (event.type == SDL_MOUSEMOTION && event.motion.state != 0) {
        circle.x =
            fmax(circle.radius, fmin(event.motion.x, WIDTH - circle.radius));
        circle.y =
            fmax(circle.radius, fmin(event.motion.y, HEIGHT - circle.radius));

        generate_rays(circle, rays);
      }
    }

    SDL_FillRect(surface, &erase_rect, COLOR_BLACK);
    FillRays(surface, rays, COLOR_RAY, shadow_circles);
    FillCircle(surface, circle);

    for (int i = 0; i < COUNT_OF_SHADOW_CIRCLES; i++) {
      FillCircle(surface, shadow_circles[i]);
    }

    update_shadow_circles(shadow_circles);

    SDL_UpdateWindowSurface(window);
    SDL_Delay(16);
  }

  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}