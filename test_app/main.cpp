#include <assimp/Importer.hpp>
#include <iostream>
#include "brtr/brtr.hpp"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <fstream>
#include <gl/glew.h>
#include <SDL2/SDL.h>
#include <glm/gtc/type_aligned.hpp>

const int screen_width = 1280;
const int screen_height = 720;

SDL_Window* window = NULL;

SDL_Renderer* renderer = NULL;

SDL_Texture* output_texture = NULL;

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

		aiColor3D ai_specular;
		glm::aligned_vec3 specular;
		if (AI_SUCCESS == ai_mat->Get(AI_MATKEY_COLOR_SPECULAR, ai_specular))
		{
			specular.r = ai_specular.r;
			specular.g = ai_specular.g;
			specular.b = ai_specular.b;
		}

		material.diffuse = diffuse;
		material.specular = specular;
	}


	return { vertices, indices, material };
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
	//Free loaded image
	SDL_DestroyTexture(output_texture);
	output_texture = NULL;

	//Destroy window    
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	window = NULL;
	renderer = NULL;

	//Quit SDL subsystems
	SDL_Quit();
}
int main(int argc, char *argv[])
{

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cout << "Could not initialise SDL\n";
		return 0;
	}
	else
	{
		window = SDL_CreateWindow("BRTR Test App", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, screen_width, screen_height, SDL_WINDOW_SHOWN);
		if (!window)
		{
			std::cout << "Could not create windowz\n";
			return 0;
		}
		else
		{
			//Create renderer for window
			renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
			if (renderer == NULL)
			{
				printf("Renderer could not be created! SDL Error: %s\n", SDL_GetError());
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

			}
		}
	}

	output_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, screen_width, screen_height);
	Assimp::Importer importer;
	auto platform = std::make_shared<brtr::platform>();
	brtr::camera camera;
	camera.set_fov(90.0f, (float)screen_width/(float)screen_height);
	camera.set_position(glm::vec3(0.0f, 0.5f, 5.0f));
	camera.set_look_at(glm::vec3(0, 0, 0));
	camera.set_up(glm::vec3(0, 1, 0));
	brtr::ray_tracer tracer{platform,camera,screen_width, screen_height, 1};
	std::vector<brtr::mesh> meshes;
	const char* path_to_scene = "../../test_app/Assets/test_scene_monkeys.obj";
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
	//While application is running
	bool quit = false;
	SDL_Event e;
	void* texture_ptr = nullptr;
	int pitch;
	while (!quit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}

			if(e.type == SDL_KEYDOWN)
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
					camera.set_position(camera.position() + glm::vec3(0, 0.1f, 0));
				}
				if (e.key.keysym.sym == SDLK_s)
				{
					camera.set_position(camera.position() + glm::vec3(0, -0.1f, 0));
				}
				//camera.set_look_at(camera.position() + glm::vec3(0, 0, -1));
			}
		}
		
		tracer.run();
		/*SDL_LockTexture(output_texture, NULL, &texture_ptr, &pitch);
		std::memcpy(texture_ptr, tracer.result_buffer().data, tracer.result_buffer().size);
		SDL_UnlockTexture(output_texture);*/
		SDL_UpdateTexture(output_texture, NULL, tracer.result_buffer()->data, screen_width * tracer.result_buffer()->stride);
		//Clear screen
		SDL_RenderClear(renderer);

		//Render texture to screen
		SDL_RenderCopyEx(renderer, output_texture, NULL, NULL, 0, NULL, SDL_RendererFlip::SDL_FLIP_VERTICAL);

		//Update screen
		SDL_RenderPresent(renderer);
	}
	std::cin.get();
    return 0;
}
