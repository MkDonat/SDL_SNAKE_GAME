#include "world.hpp"

world::world(
    world_config_struct cfg,
    SDL_Renderer* renderer
){
    world::world_params = &cfg;
    world::get_renderer_size(renderer);
    determine_cell_size(world::world_params);
}

world::~world(){
    // destructor
}

void world::determine_cell_size(
    world_config_struct* cfg
){
    world::CELLS_WIDTH = 
    SDL_floorf(
        world::renderer_w * 
        cfg->CELLS_PORTION/100.0f
    );
    world::CELLS_HEIGHT = 
    SDL_floorf(
        world::renderer_h * 
        cfg->CELLS_PORTION/100.0f
    );
}

void world::get_renderer_size(
    SDL_Renderer* renderer
){
    SDL_GetRenderOutputSize(
        renderer,
        &(world::renderer_w),
        &(world::renderer_h)
    );
}