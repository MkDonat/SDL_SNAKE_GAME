#pragma Once
#include "structures.hpp"
#include <SDL3/SDL.h>

class world{

    private:
        int CELLS_WIDTH = {0};
        int CELLS_HEIGHT = {0};

        int renderer_w = {0};
        int renderer_h = {0};

        void determine_cell_size(
            world_config_struct* cfg
        );
        void get_renderer_size(
            SDL_Renderer* renderer
        );

    public:
        world_config_struct* world_params;
        
        world(
            world_config_struct cfg,
            SDL_Renderer* renderer
        );
        ~world();  

    
};