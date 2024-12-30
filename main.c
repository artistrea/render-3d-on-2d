#include <SDL2/SDL.h>
#include <unistd.h>
// #include <SDL2/SDL_image.h>
#include <stdint.h>
#include <math.h>

#define PI 3.14159265

struct Vec3 {
  float x;
  float y;
  float z;
};

struct Vec2 {
  float x;
  float y;
};

struct Square {
  struct Vec3 pos;
  float width;
};

struct Vec3 Vec3_add(struct Vec3 v1, struct Vec3 v2) {
  return (struct Vec3){ .x=v1.x+v2.x, .y=v1.y+v2.y, .z=v1.z+v2.z };
}

struct Vec3 Vec3_sub(struct Vec3 v1, struct Vec3 v2) {
  return (struct Vec3){ .x=v1.x-v2.x, .y=v1.y-v2.y, .z=v1.z-v2.z };
}

float Vec3_dot(struct Vec3 v1, struct Vec3 v2) {
  return v1.x*v2.x + v1.y*v2.y + v1.z * v2.z;
}

float Vec3_mod(struct Vec3 v) {
  float l1 = sqrt(v.x*v.x + v.y*v.y);
  return sqrt(l1*l1 + v.z*v.z);
}

float Vec2_dot(struct Vec2 v1, struct Vec2 v2) {
  return v1.x*v2.x + v1.y*v2.y;
}

float Vec2_mod(struct Vec2 v) {
  return sqrt(v.x*v.x + v.y*v.y);
}

float Vec2_angle(struct Vec2 v1, struct Vec2 v2) {
  return acos(Vec2_dot(v1, v2) / (Vec2_mod(v1) * Vec2_mod(v2)));
}

float Vec3_xy_angle(struct Vec3 v1, struct Vec3 v2) {
  return Vec2_angle((struct Vec2){.x=v1.x,.y=v1.y}, (struct Vec2){.x=v2.x,.y=v2.y});
}

float Vec3_yz_angle(struct Vec3 v1, struct Vec3 v2) {
  return Vec2_angle((struct Vec2){.x=v1.y,.y=v1.z}, (struct Vec2){.x=v2.y,.y=v2.z});
}

struct Vec2 Vec2_rotate(struct Vec2 v, float angle) {
  return (struct Vec2){ .x=v.x * cos(angle) - v.y * sin(angle), .y=v.y * cos(angle) + v.x * sin(angle) };
}

struct Vec3 Vec3_xy_rotate(struct Vec3 v1, float angle) {
  struct Vec2 v2 = Vec2_rotate((struct Vec2){.x=v1.x,.y=v1.y}, angle);
  return (struct Vec3){ .x=v2.x, .y=v2.y, .z=v1.z };
}

struct Vec3 Vec3_local_coord(
  struct Vec3 zero,
  float zero_azimuth,
  float zero_elevation,
  struct Vec3 point
) {
  return Vec3_xy_rotate(Vec3_sub(point, zero), zero_azimuth);
}

// transform with Vec3_local_coord before calling this
SDL_Point Vec3_get_screen_projection(
  struct Vec3 position_rel_to_camera,
  float vfov,
  float hfov,
  int window_height,
  int window_width
) {
  double angle_between = Vec3_xy_angle(position_rel_to_camera, (struct Vec3){.x=0,.y=1,.z=0});
  double angle_between2 = Vec3_yz_angle(position_rel_to_camera, (struct Vec3){.x=0,.y=1,.z=0});

  int proj_x = -100;
  if (angle_between > 0 && position_rel_to_camera.x < 0) angle_between = -angle_between;
  else if (angle_between > 0 && position_rel_to_camera.y < 0) angle_between = angle_between - PI;
  // x / dist_y * window_width
  // w + tan(wfov) * y
  proj_x = window_width * angle_between / hfov / 2 + (float)window_width / 2;

  int proj_y = -100;
  if (angle_between2 > 0 && position_rel_to_camera.z < 0) angle_between2 = -angle_between2;
  printf("position_rel_to_camera x: %f, y: %f, z: %f\n", position_rel_to_camera.x, position_rel_to_camera.y, position_rel_to_camera.z);
  printf("angle_between: %f\n", angle_between);
  printf("angle_between2: %f\n", angle_between2);
  proj_y = window_height * angle_between2 / vfov / 2 + (float)window_height / 2;
  // proj_y = (float)window_height * position_rel_to_camera.z / position_rel_to_camera.y + (float)window_height / 2;
  printf("proj x: %d, y: %d, \n", proj_x, proj_y);

  return (SDL_Point){ .x=proj_x, .y= proj_y};
}

void Square_render(
  struct Vec3 camera,
  float camera_azimuth,
  float camera_elevation,
  float hfov,
  float vfov,
  SDL_Renderer *renderer,
  struct Square square
) {
  SDL_Point points[5];

  points[0] = Vec3_get_screen_projection(
    Vec3_local_coord(
      camera,
      camera_azimuth,
      camera_elevation,
      square.pos
    ),
    vfov,
    hfov,
    1200,
    1800
  );
  points[1] = Vec3_get_screen_projection(
    Vec3_local_coord(
      camera,
      camera_azimuth,
      camera_elevation,
      Vec3_add(square.pos, (struct Vec3){.x=square.width,.y=0,.z=0})
    ),
    vfov,
    hfov,
    1200,
    1800
  );
  points[2] = Vec3_get_screen_projection(
    Vec3_local_coord(
      camera,
      camera_azimuth,
      camera_elevation,
      Vec3_add(square.pos, (struct Vec3){.x=square.width,.y=square.width,.z=0})
    ),
    vfov,
    hfov,
    1200,
    1800
  );
  points[3] = Vec3_get_screen_projection(
    Vec3_local_coord(
      camera,
      camera_azimuth,
      camera_elevation,
      Vec3_add(square.pos, (struct Vec3){.x=0,.y=square.width,.z=0})
    ),
    vfov,
    hfov,
    1200,
    1800
  );
  points[4] = points[0];
  SDL_RenderDrawLines(renderer, points, 5);
}

int main() {
  SDL_Init(SDL_INIT_EVERYTHING);

  SDL_Window *window = SDL_CreateWindow(
    "opa",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    1800,
    1200,
    SDL_WINDOW_MINIMIZED
  );

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  int over = 0;

  struct Square square = {
    .pos={.x=1,.y=1,.z=-10},
    .width=15
  };
  struct Square square2 = {
    .pos={.x=1,.y=1,.z=1},
    .width=10
  };

  struct Vec3 camera = {
    .x=1,.y=-20,.z=0
  };
  float camera_angle=0.0f;
  float vfov = 0.9f, hfov=0.9f;


  uint32_t t = SDL_GetTicks();
  uint32_t prev_t = SDL_GetTicks();
  uint32_t dt;
  float camera_speed = 0.005;

  uint8_t keys_pressed[SDL_NUM_SCANCODES] = {0};

  while (!over) {
    t = SDL_GetTicks();
    dt = t - prev_t;
    prev_t = t;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          over = 1;
          break;
        case SDL_KEYDOWN: {
          keys_pressed[event.key.keysym.scancode] = 1;
          break;
        }
        case SDL_KEYUP: {
          keys_pressed[event.key.keysym.scancode] = 0;
          break;
        }
      }
    }
    if (keys_pressed[SDL_SCANCODE_Q]) {
      over=1;
      continue;
    }
    if (keys_pressed[SDL_SCANCODE_D]) {
      camera.x+=dt*camera_speed*cos(camera_angle);
      camera.y-=dt*camera_speed*sin(camera_angle);
    }
    if (keys_pressed[SDL_SCANCODE_A]) {
      camera.x-=dt*camera_speed*cos(camera_angle);
      camera.y+=dt*camera_speed*sin(camera_angle);
    }
    if (keys_pressed[SDL_SCANCODE_W]) {
      camera.x+=dt*camera_speed*sin(camera_angle);
      camera.y+=dt*camera_speed*cos(camera_angle);
    }
    if (keys_pressed[SDL_SCANCODE_S]) {
      camera.x-=dt*camera_speed*sin(camera_angle);
      camera.y-=dt*camera_speed*cos(camera_angle);
    }

    if (keys_pressed[SDL_SCANCODE_SPACE]) {
      camera.z-=dt*camera_speed;
    }
    if (keys_pressed[SDL_SCANCODE_LSHIFT]) {
      camera.z+=dt*camera_speed;
    }

    if (keys_pressed[SDL_SCANCODE_LEFT]) {
      camera_angle-=dt*0.0008;
      if (camera_angle < PI) camera_angle += 2*PI;
    }
    if (keys_pressed[SDL_SCANCODE_RIGHT]) {
      camera_angle+=dt*0.0008;
      if (camera_angle > PI) camera_angle -= 2*PI;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
    struct Vec2 camera_dir = Vec2_rotate((struct Vec2){ .x=1, .y=0 }, camera_angle);

    Square_render(
      camera,
      camera_angle,
      0,
      hfov,
      vfov,
      renderer,
      square
    );
    Square_render(
      camera,
      camera_angle,
      0,
      hfov,
      vfov,
      renderer,
      (struct Square){
        .pos=(struct Vec3){.x=0,.y=1,.z=-20},
        .width=2
      }
    );
    Square_render(
      camera,
      camera_angle,
      0,
      hfov,
      vfov,
      renderer,
      (struct Square){
        .pos=(struct Vec3){.x=0,.y=1,.z=-20},
        .width=3
      }
    );
    Square_render(
      camera,
      camera_angle,
      0,
      hfov,
      vfov,
      renderer,
      (struct Square){
        .pos=(struct Vec3){.x=0,.y=1,.z=-20},
        .width=4
      }
    );
    Square_render(
      camera,
      camera_angle,
      0,
      hfov,
      vfov,
      renderer,
      (struct Square){
        .pos=(struct Vec3){.x=0,.y=1,.z=-20},
        .width=5
      }
    );
    Square_render(
      camera,
      camera_angle,
      0,
      hfov,
      vfov,
      renderer,
      square2
    );
    // for (int i=0;i<square2.width;i++) {
    //   for (int j=0;j<square2.width;j++) {
    //     struct Vec3 relative_pos = Vec3_xy_rotate(Vec3_sub(Vec3_add(square2.pos, (struct Vec3){.x=j,.y=i,.z=0}), camera), camera_angle);
    //     double angle_between = Vec3_xy_angle(relative_pos, (struct Vec3){.x=0,.y=1,.z=0});
    //     double angle_between2 = Vec3_yz_angle(relative_pos, (struct Vec3){.x=0,.y=1,.z=0});
    //     int proj_x = -100;
    //     if (angle_between > 0 && relative_pos.x < 0) angle_between = -angle_between;
    //     proj_x = 1800/(2*hfov) * (hfov+angle_between);
    //     int proj_y = -100;
    //     if (angle_between2 > 0 && relative_pos.z < 0) angle_between2 = -angle_between2;
    //     proj_y = 1200/(2*vfov) * (vfov+angle_between2);
    //     if (i==0 & j==0) {
    //       // SDL_Log("pos: x: %i, y: %i, z: %i", relative_pos.x, relative_pos.y, relative_pos.z);
    //       // SDL_Log("x: %i, y: %i",proj_x, proj_y);
    //     }
    //     SDL_RenderDrawPoint(renderer, proj_x, proj_y);
    //   }
    // }
    SDL_RenderPresent(renderer);

    SDL_Delay(33);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  SDL_Quit();

  return 0;
}

// oq é um processo? Um programa com contexto e permissões
// oq é um SO?
// pode cair pseudocódigo perguntando se o código resolve o problema. Ex: jantar dos filósofos
// Sistema multitarefa
//  alterna a

