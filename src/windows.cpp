#include "call.h"

namespace coordinates{
    long int triangleCount {};
    render::Point2D convertCoordinates(render::Point2D point, int screenW, int screenH){
        int screenOriginX {screenW/2};
        int screenOriginY {screenH/2};

        float newX {(screenOriginX + point.x*20)};
        float newY {(screenOriginY - point.y*20)};

        return {newX, newY, point.sZ, point.shown};
    }
}
