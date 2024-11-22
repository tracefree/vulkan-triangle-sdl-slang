#include <SDL3/SDL.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>

#include <string>
#include <fstream>



#include "util.h"
#include "renderer.h"

SDL_Window* gWindow{ nullptr };

Uint64 previous_time {0};
double delta {0.0};

Renderer gRenderer;



static std::vector<char> read_file(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        print("Could not open file '%s'!", file_path);
    }

    size_t file_size = (size_t) file.tellg();
    std::vector<char> buffer(file_size);

    file.seekg(0);
    file.read(buffer.data(), file_size);
    file.close();

    return buffer;
}



SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

    // Initialize app
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        SDL_Log("SDL could not initialize! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Load Vulkan driver
    if(!SDL_Vulkan_LoadLibrary(nullptr)) {
        SDL_Log("Could not load Vulkan library! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    // Create window
    SDL_PropertiesID window_props {SDL_CreateProperties()};
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, 800);
    SDL_SetNumberProperty(window_props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, 800);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_BORDERLESS_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN, false);
    SDL_SetBooleanProperty(window_props, SDL_PROP_WINDOW_CREATE_VULKAN_BOOLEAN, true);
    SDL_SetStringProperty(window_props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, "Prosper");
    gWindow = SDL_CreateWindowWithProperties(window_props);
    if (gWindow == nullptr) {
        SDL_Log("Window could not be created! SDL error: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    uint32_t sdl_extension_count;
    const char* const* sdl_extension_names = SDL_Vulkan_GetInstanceExtensions(&sdl_extension_count);
    

    // Initialize Vulkan
    gRenderer = Renderer();
    gRenderer.initialize(sdl_extension_count, sdl_extension_names, gWindow);

    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppIterate(void *appstate) {
    // Delta
    Uint64 current_time = SDL_GetTicksNS();
    delta = double(current_time - previous_time) * 0.000000001;
    previous_time = current_time;

    gRenderer.draw();
    
    return SDL_APP_CONTINUE;
}


SDL_AppResult SDL_AppEvent(void *appsatte, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}


void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    gRenderer.cleanup();
}