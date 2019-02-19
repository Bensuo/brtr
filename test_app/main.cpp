

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
#include <glm/gtc/type_aligned.hpp>
#include <iostream>

const int screen_width = 1280;
const int screen_height = 720;

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

        // aiColor3D ai_specular;
        // glm::aligned_vec3 specular;
        // if (AI_SUCCESS == ai_mat->Get(AI_MATKEY_COLOR_SPECULAR, ai_specular))
        //{
        //    specular.r = ai_specular.r;
        //    specular.g = ai_specular.g;
        //    specular.b = ai_specular.b;
        //}

        aiColor3D ai_emissive;
        glm::aligned_vec3 emissive;
        if (AI_SUCCESS == ai_mat->Get(AI_MATKEY_COLOR_EMISSIVE, ai_emissive))
        {
            emissive.r = ai_emissive.r;
            emissive.g = ai_emissive.g;
            emissive.b = ai_emissive.b;
        }

        material.diffuse = diffuse;
        material.roughness = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
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
    camera.set_fov(90.0f, (float)screen_width / (float)screen_height);
    camera.set_position(glm::vec3(0.0f, 0.5f, 5.0f));
    camera.set_look_at(glm::vec3(0, 0, -1));
    camera.set_up(glm::vec3(0, 1, 0));
    brtr::ray_tracer tracer{platform, camera, screen_width, screen_height, 1};
    renderer render{screen_width, screen_height, tracer};
    std::vector<brtr::mesh> meshes;
    const char* path_to_scene = "../../test_app/Assets/many_squares_walls.obj";
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
    brtr::point_light light;
    light.position = glm::vec3{2, -1, 5};
    light.colour = glm::vec3(50);
    light.radius = 4.0f;
    // While application is running
    bool quit = false;
    SDL_Event e;
    void* texture_ptr = nullptr;
    int pitch;
    SDL_SetRelativeMouseMode(SDL_TRUE);
    while (!quit)
    {
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
                camera.rotate(e.motion.yrel * 0.01f, glm::vec3(1, 0, 0));
                camera.rotate(e.motion.xrel * 0.01f, glm::vec3(0, 1, 0));
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
                    // camera.set_position(camera.position() + glm::vec3(0, 0.1f, 0));
                    camera.move_forward(0.2f);
                }
                if (e.key.keysym.sym == SDLK_s)
                {
                    // camera.set_position(camera.position() + glm::vec3(0, -0.1f, 0));
                    camera.move_forward(-0.2f);
                }
                if (e.key.keysym.sym == SDLK_a)
                {
                    // camera.set_position(camera.position() + glm::vec3(0, 0.1f, 0));
                    camera.move_right(-0.2f);
                }
                if (e.key.keysym.sym == SDLK_d)
                {
                    // camera.set_position(camera.position() + glm::vec3(0, -0.1f, 0));
                    camera.move_right(0.2f);
                }
                if (e.key.keysym.sym == SDLK_ESCAPE)
                {
                    quit = true;
                }
                // camera.set_look_at(camera.position() + glm::vec3(0, 0, -1));
            }
            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        tracer.run();
        render.render();
    }

    SDL_Quit();
    return 0;
}
