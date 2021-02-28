#include <iostream>
#include <SDL2/SDL.h>
#include <ctime>
#include <cairo/cairo.h>
#include <exception>

double G = 6.67408e-11;
double dt = 10;
long long sim_time = 0;
int speed = 60 * 60;

cairo_surface_t *bgnd;

struct Body {
    double x;
    double y;
    double ux;
    double uy;
    double r;
    double m;
    const char *name;
    cairo_surface_t * surf;
};

Body earth {
            .x = 147.10e9,
            .y = 0,
            .ux = 0,
            .uy = 30.29e3,
            .r = 6378e3,
            .m = 5.9722e24,
            .name = "Earth",
            .surf = nullptr,
};

Body sun {
          .x = 0,
          .y = 0,
          .ux = 0,
          .uy = 0,
          .r = 695700e3,
          .m = 1.9885e30,
          .name = "Sol",
          .surf = nullptr,
};

Body ship {
          .x = earth.x + earth.r + 1000,
          .y = 0,
          .ux = 0,
          .uy = earth.uy + 8e3,
          .r = 100,
          .m = 1000,
          .name = "Ship",
          .surf = nullptr,
};

Body moon {
          .x = earth.x + earth.r + 382500e3,
          .y = 0,
          .ux = 0,
          .uy = earth.uy + 1023,
          .r = 1736e3,
          .m = 0.07e24,
          .name = "Moon",
          .surf = nullptr,
};

Body *center = &sun;

double scale = 1e-9;
void draw_planet(cairo_t *cr, const Body &body) {
    int w = cairo_image_surface_get_width(body.surf);
    int h = cairo_image_surface_get_height(body.surf);
    cairo_save(cr);
    cairo_translate(cr, body.x, body.y);
    cairo_scale(cr, 2*body.r/w, 2*body.r/w);
    double x = w, y = w;
    cairo_user_to_device_distance(cr, &x, &y);
    if (x > 1) {
        cairo_set_source_surface(cr, body.surf, -w/2, -h/2);
        cairo_paint(cr);
    }
    x = y = 0;
    cairo_user_to_device(cr, &x, &y);
    cairo_identity_matrix(cr);
    cairo_set_source_rgba(cr, 1, 1, 1, 1.0);
    cairo_rectangle(cr, x - 5, y - 5, 10, 10);

    cairo_stroke(cr);

    cairo_text_extents_t te;
    cairo_set_source_rgb (cr, 1, 1, 0.0);
    cairo_select_font_face (cr, "Courier",
        CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size (cr, 16);
    cairo_text_extents (cr, body.name, &te);
    cairo_move_to (cr, x + 0.5 - te.width / 2 - te.x_bearing,
        y + 7 - te.y_bearing);
    cairo_show_text (cr, body.name);

    cairo_restore(cr);
}

void draw(SDL_Renderer *ren, cairo_t *cr) {
    int renderer_width;
    int renderer_height;
    SDL_GetRendererOutputSize(ren, &renderer_width, &renderer_height);

    cairo_identity_matrix(cr);
    cairo_set_source_rgba(cr, 0, 0, 0, 1.0);
    cairo_rectangle(cr, 0, 0, renderer_width, renderer_height);
    cairo_fill(cr);

    // tail the sky background
    cairo_save(cr);
    cairo_translate(cr, 0, 0);
    cairo_scale(cr, 1, 1);
    int w = cairo_image_surface_get_width(bgnd);
    int h = cairo_image_surface_get_height(bgnd);
    for (int x = 0; x < renderer_width; x += w) {
        for (int y = 0; y < renderer_height; y +=h ) {
            cairo_set_source_surface (cr, bgnd, x, y);
            cairo_paint (cr);
        }
    }
    cairo_restore(cr);

    {
        char buffer[512];
        int min = 60;
        int hour = 60 * min;
        int day = 24 * hour;
        int year = 365 * day; 

        long long rest = sim_time;
        int y = rest / year;
        rest -= y * year;
        int d = rest / day;
        rest -= d * day;
        int h = rest / hour;
        rest -= h * hour;
        int m = rest / min;
        int s = rest % min;
        sprintf(buffer, "Passed %d years %d days %d:%d:%d", y, d, h, m, s);
        cairo_text_extents_t te;
        cairo_set_source_rgb (cr, 1, 1, 0.0);
        cairo_select_font_face (cr, "Calibri", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size (cr, 16);
        cairo_text_extents (cr, buffer, &te);
        cairo_move_to (cr, 0, te.height - te.y_bearing);
        cairo_show_text (cr, buffer);
    }

    cairo_save(cr);
    cairo_translate(cr, renderer_width / 2, renderer_height / 2);
    cairo_scale(cr, scale, scale);
    cairo_translate(cr, -center->x, -center->y);
    draw_planet(cr, sun);
    draw_planet(cr, moon);
    draw_planet(cr, earth);
    draw_planet(cr, ship);
    cairo_restore(cr);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0){
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *win = SDL_CreateWindow("Orbit Simulator", 100, 100, 2000, 1200, SDL_WINDOW_SHOWN);
    if (win == nullptr){
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr){
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    int window_width;
    int window_height;
    SDL_GetWindowSize(win, &window_width, &window_height);

    int renderer_width;
    int renderer_height;
    SDL_GetRendererOutputSize(ren, &renderer_width, &renderer_height);

    sun.surf = cairo_image_surface_create_from_png ("sun.png");
    if (!sun.surf) {
        std::cout << "Failed to load background: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    ship.surf = cairo_image_surface_create_from_png ("sputnik.png");
    if (!ship.surf) {
        std::cout << "Failed to load background: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    earth.surf = cairo_image_surface_create_from_png ("earth.png");
    if (!earth.surf) {
        std::cout << "Failed to load background: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    moon.surf = cairo_image_surface_create_from_png ("moon.png");
    if (!moon.surf) {
        std::cout << "Failed to load background: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    bgnd = cairo_image_surface_create_from_png ("night.png");
    if (!bgnd) {
        std::cout << "Failed to load background: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Surface *sdl_surface = SDL_CreateRGBSurface(0,
                                                    renderer_width,
                                                    renderer_height,
                                                    32,
                                                    0x00ff0000,
                                                    0x0000ff00,
                                                    0x000000ff,
                                                    0x00000000);

    cairo_surface_t *cr_surface =
        cairo_image_surface_create_for_data((unsigned char *)sdl_surface->pixels,
                                            CAIRO_FORMAT_ARGB32,
                                            sdl_surface->w,
                                            sdl_surface->h,
                                            sdl_surface->pitch);

    cairo_t *cr = cairo_create(cr_surface);
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                done = true;
                break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_1:
                        center = &sun;
                        break;
                    case SDLK_2:
                        center = &earth;
                        break;
                    case SDLK_3:
                        center = &moon;
                        break;
                    case SDLK_4:
                        center = &ship;
                        break;
                    case SDLK_i:
                        scale *= 2;
                        break;
                    case SDLK_o:
                        scale /= 2;
                        break;
                    case SDLK_f:
                        speed *= 2;
                        break;
                    case SDLK_s:
                        speed /= 2;
                        if (speed == 0) speed = 1;
                        break;
                }
            default:
                break;
            }
        }

        for (int x = 0; x < speed; ++ x) {
            sim_time += dt;
            { // Earth  
                double r = std::sqrt(earth.x * earth.x + earth.y * earth.y);
                double a = G * sun.m / r / r;
                double ax = - a * earth.x / r;
                double ay = - a * earth.y / r;
                earth.ux += ax * dt;
                earth.uy += ay * dt;
                earth.x += earth.ux * dt;
                earth.y += earth.uy * dt;
            }

            { // Moon  
                
                // force and acceleration because of Sun
                double r = std::sqrt(moon.x * moon.x + moon.y * moon.y);
                double a = G * sun.m / r / r;
                double ax = - a * moon.x / r;
                double ay = - a * moon.y / r;

                // force and acceleration because of Earth
                double x = moon.x - earth.x;
                double y = moon.y - earth.y;
                r = sqrt(x*x + y*y);
                a = G * earth.m / r / r;
                ax = ax - a * x / r;
                ay = ay - a * y / r;
                // update speed and coordinates
                moon.ux += ax * dt;
                moon.uy += ay * dt;
                moon.x += moon.ux * dt;
                moon.y += moon.uy * dt;
            }

            { // Ship  
                
                // force and acceleration because of Sun
                double r = std::sqrt(ship.x * ship.x + ship.y * ship.y);
                double a = G * sun.m / r / r;
                double ax = - a * ship.x / r;
                double ay = - a * ship.y / r;

                // force and acceleration because of Earth
                double x = ship.x - earth.x;
                double y = ship.y - earth.y;
                r = sqrt(x*x + y*y);
                a = G * earth.m / r / r;
                ax = ax - a * x / r;
                ay = ay - a * y / r;

                // force and acceleration because of Moon
                x = ship.x - moon.x;
                y = ship.y - moon.y;
                r = sqrt(x*x + y*y);
                a = G * moon.m / r / r;
                ax = ax - a * x / r;
                ay = ay - a * y / r;
             
                // update speed and coordinates
                ship.ux += ax * dt;
                ship.uy += ay * dt;
                ship.x += ship.ux * dt;
                ship.y += ship.uy * dt;
            }
        }

        SDL_Delay(1);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
        SDL_RenderClear(ren);
        draw(ren, cr);
        SDL_Texture *texture = SDL_CreateTextureFromSurface(ren, sdl_surface);

        SDL_RenderCopy(ren, texture, NULL, NULL);
        SDL_DestroyTexture(texture);
        SDL_RenderPresent(ren);

    }

    cairo_destroy(cr);
    cairo_surface_destroy(cr_surface);

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);

    SDL_Quit();
}
