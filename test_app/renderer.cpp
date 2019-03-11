#include "renderer.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <chrono>
#include <iostream>

renderer::renderer(int w, int h, brtr::ray_tracer& tracer)
    : screen_width(w), screen_height(h), tracer(tracer)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "Could not initialise SDL\n";
    }

    window = SDL_CreateWindow(
        "BRTR Test App",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        screen_width,
        screen_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    imgui_window = SDL_CreateWindow(
        "BRTR Test App",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        screen_width / 2,
        screen_height / 2,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Create window with graphics context
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    SDL_DisplayMode current;
    SDL_GetCurrentDisplayMode(0, &current);
    gl_context = SDL_GL_CreateContext(imgui_window);
    sdl_renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    // glewExperimental = true;
    bool err = glewInit() != 0;

    if (err)
    {
        std::cout << "Failed to initialise GLEW\n";
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui_ImplSDL2_InitForOpenGL(imgui_window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImGui::StyleColorsDark();

    output_texture = SDL_CreateTexture(
        sdl_renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);

    /*glGenTextures(1, &output_texture);
    glBindTexture(GL_TEXTURE_2D, output_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::vector<GLubyte> empty(screen_width * screen_height * 3, 0);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        screen_width,
        screen_height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        empty.data());
    glBindTexture(GL_TEXTURE_2D, 0);*/
}

renderer::~renderer()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_DestroyWindow(imgui_window);
    // Free loaded image
    SDL_DestroyTexture(output_texture);
    output_texture = NULL;

    // Destroy window
    SDL_DestroyRenderer(sdl_renderer);

    window = NULL;
    sdl_renderer = NULL;
    imgui_window = NULL;

    // Quit SDL subsystems
    SDL_Quit();
}

void renderer::render(float last_frame_time)
{
    bool active = false;
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    /*SDL_LockTexture(output_texture, NULL, &texture_ptr, &pitch);
       std::memcpy(texture_ptr, tracer.result_buffer().data,
       tracer.result_buffer().size); SDL_UnlockTexture(output_texture);*/
    SDL_UpdateTexture(
        output_texture,
        NULL,
        tracer.result_buffer()->data,
        screen_width * tracer.result_buffer()->stride);
    SDL_RenderCopyEx(
        sdl_renderer, output_texture, NULL, NULL, 0, NULL, SDL_RendererFlip::SDL_FLIP_VERTICAL);
    // Clear screen
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame(imgui_window);
    ImGui::NewFrame();
    bool win = true;
    auto stats = tracer.get_stats();
    ImGui::Begin("BRTR Stats");
    ImGui::Text("Overall frame time: %.2fms", last_frame_time);
    ImGui::Text("Overall GPU: %.2fms", stats.total_overall);
    ImGui::Text("Overall BVH: %.2fms", stats.total_bvh);
    ImGui::Text("BVH Construction time: %.2fms", stats.bvh_construction);
    ImGui::Text("Bvh GPU: %.2fms", stats.bvh_gpu);
    ImGui::Text("Raytrace kernel time: %.2fms", stats.total_raytrace);
    ImGui::Text("Denoise kernel time: %.2fms", stats.denoise);
    ImGui::Text("Average overall time: %.2fms", stats.lifetime_overall / stats.num_iterations);
    ImGui::Text("Average raytrace kernel: %.2fms", stats.lifetime_kernel / stats.num_iterations);
    ImGui::Text("Average bvh gpu: %.2fms", stats.lifetime_bvh_gpu / stats.num_iterations);
    ImGui::Text(
        "Average bvh construction: %.2fms",
        stats.lifetime_bvh_construction / stats.num_iterations);
    ImGui::End();

    ImGui::Render();
    // SDL_GL_MakeCurrent(window, gl_context);
    glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
    // glClearColor(clear_color.x, clear_color.y, clear_color.z,
    // clear_color.w); glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(imgui_window);
    SDL_RenderPresent(sdl_renderer);
    // Update screen
}
