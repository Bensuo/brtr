

#include "brtr/brtr.hpp"
#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_sdl.h"
#include "renderer.hpp"
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_aligned.hpp>
#include <iostream>

const int screen_width = 1920;
const int screen_height = 1080;

brtr::mesh ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<brtr::vertex> vertices;
    brtr::mesh_material material;
    std::vector<int> indices;
    for (unsigned i = 0; i < mesh->mNumVertices; i++)
    {
        brtr::vertex vert;
        glm::aligned_vec3 vec;

        vec.x = mesh->mVertices[i].x;
        vec.y = mesh->mVertices[i].y;
        vec.z = mesh->mVertices[i].z;
        vert.position = vec;

        if (mesh->mNormals)
        {
            vec.x = mesh->mNormals[i].x;
            vec.y = mesh->mNormals[i].y;
            vec.z = mesh->mNormals[i].z;
            vert.normal = vec;
        }
        vertices.push_back(vert);
    }

    for (int i = 0; i < mesh->mNumFaces; ++i)
    {
        const auto face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; ++j)
        {
            indices.push_back(face.mIndices[j]);
        }
    }
    if (mesh->mMaterialIndex >= 0)
    {
        const auto ai_mat = scene->mMaterials[mesh->mMaterialIndex];

        aiColor3D ai_diffuse;
        glm::aligned_vec3 diffuse;
        if (AI_SUCCESS == ai_mat->Get(AI_MATKEY_COLOR_DIFFUSE, ai_diffuse))
        {
            diffuse.r = ai_diffuse.r;
            diffuse.g = ai_diffuse.g;
            diffuse.b = ai_diffuse.b;
        }

        aiColor3D ai_emissive;
        glm::aligned_vec3 emissive;
        if (AI_SUCCESS == ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, ai_emissive))
        {
            emissive.r = ai_emissive.r;
            emissive.g = ai_emissive.g;
            emissive.b = ai_emissive.b;
        }

        material.diffuse = diffuse;
        material.roughness = 0.82f;

        material.emissive = emissive;
    }

    return {vertices, indices, material};
}
void ProcessNode(const aiScene* scene, aiNode* node, std::vector<brtr::mesh>& meshes)
{
    for (unsigned i = 0; i < node->mNumMeshes; i++)
    {
        const auto mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(ProcessMesh(mesh, scene));
    }
    for (unsigned i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(scene, node->mChildren[i], meshes);
    }
}
void close()
{
}
int main(int argc, char* argv[])
{
    Assimp::Importer importer;
    auto platform = std::make_shared<brtr::platform>();
    brtr::camera camera;
    camera.set_fov(60.0f, (float)screen_width / (float)screen_height);
    camera.set_position(glm::vec3(-68.1f, 32.9f, -6.67f));
    camera.set_yaw(90.0f);

    camera.set_up(glm::vec3(0, 1, 0));
    glm::vec3 camera_target{20.0f, 5.5f, 25.0f};
    brtr::ray_tracer tracer{platform, camera, screen_width, screen_height};
    renderer render{screen_width, screen_height, tracer};
    std::vector<brtr::mesh> meshes;
    const char* path_to_scene = "../../test_app/Assets/cornell.obj";
    std::ifstream file;
    file.open(path_to_scene);
    if (!file)
    {
        std::cout << "File not found\n";
        return 0;
    }
    file.close();
    auto scene = importer.ReadFile(path_to_scene, aiProcess_Triangulate | aiProcess_GenNormals);

    if (!scene)
    {
        std::cout << importer.GetErrorString() << "\n";
    }
    if (scene->HasMeshes())
    {
        ProcessNode(scene, scene->mRootNode, meshes);
    }
    for (auto& mesh : meshes)
    {
        tracer.add_mesh(mesh);
    }
    brtr::directional_light light;
    light.direction = glm::vec3{1.5f, -1, 1};
    light.colour = glm::vec3(0.7f);
    tracer.set_directional_light(light);
    // While application is running
    bool quit = false;
    SDL_Event e;
    void* texture_ptr = nullptr;
    int pitch;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    std::chrono::high_resolution_clock::time_point start =
        std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point end =
        std::chrono::high_resolution_clock::now();
    float speed = 0.01f;
    int counter = 0;
    while (!quit)
    {
        std::chrono::high_resolution_clock::time_point end =
            std::chrono::high_resolution_clock::now();
        float delta = (end - start).count() / 1000000.0f;
        // Handle events on queue
        while (SDL_PollEvent(&e) != 0)
        {
            // User requests quit
            if (e.type == SDL_QUIT)
            {
                quit = true;
            }
            if (e.type == SDL_MOUSEMOTION)
            {
                camera.modify_pitch(e.motion.yrel * 0.001f * delta);
                camera.modify_yaw(e.motion.xrel * 0.001f * delta);
            }
            if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_UP)
                {
                    camera.set_position(camera.position() + glm::vec3(0, 0, -0.1f));
                }
                if (e.key.keysym.sym == SDLK_DOWN)
                {
                    camera.set_position(camera.position() + glm::vec3(0, 0, 0.1f));
                }
                if (e.key.keysym.sym == SDLK_LEFT)
                {
                    camera.set_position(camera.position() + glm::vec3(-0.1f, 0, 0));
                }
                if (e.key.keysym.sym == SDLK_RIGHT)
                {
                    camera.set_position(camera.position() + glm::vec3(0.1f, 0, 0));
                }
                if (e.key.keysym.sym == SDLK_w)
                {
                    camera.move_forward(speed * delta);
                }
                if (e.key.keysym.sym == SDLK_s)
                {
                    camera.move_forward(-speed * delta);
                }
                if (e.key.keysym.sym == SDLK_a)
                {
                    camera.move_right(-speed * delta);
                }
                if (e.key.keysym.sym == SDLK_d)
                {
                    camera.move_right(speed * delta);
                }
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
            }
            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        tracer.run();
        render.render(delta);

        start = end;
    }
    int timestamp = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    render.write_stats_csv("test" + std::to_string(timestamp) + ".csv");
    SDL_Quit();
    return 0;
}
