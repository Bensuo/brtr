#pragma once
#include "ray_tracer.hpp"
#include <GL/glew.h>
#include <SDL.h>
#include <chrono>

class renderer
{
public:
    renderer(int w, int h, brtr::ray_tracer& tracer);
    ~renderer();
    void render(float last_frame_time);

private:
    int screen_width;
    int screen_height;
    SDL_Window* window;
    SDL_Window* imgui_window;
    SDL_GLContext gl_context;
    SDL_Renderer* sdl_renderer;
    SDL_Texture* output_texture;
    brtr::ray_tracer& tracer;
};
