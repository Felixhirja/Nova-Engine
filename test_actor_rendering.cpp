#include <iostream>
#include "engine/ecs/Components.h"

// Simple compilation test for DrawComponent
int main(int argc, char* argv[]) {
    std::cout << "Testing DrawComponent compilation..." << std::endl;

    // Test that DrawComponent can be instantiated
    DrawComponent drawComp;
    drawComp.mode = DrawComponent::RenderMode::Sprite2D;
    drawComp.visible = true;
    drawComp.SetTint(1.0f, 0.5f, 0.0f);
    drawComp.spriteScale = 2.0f;

    // Test Position component
    Position pos;
    pos.x = 1.0;
    pos.y = 2.0;
    pos.z = 3.0;

    std::cout << "DrawComponent created successfully!" << std::endl;
    std::cout << "Mode: " << static_cast<int>(drawComp.mode) << std::endl;
    std::cout << "Visible: " << (drawComp.visible ? "true" : "false") << std::endl;
    std::cout << "Tint: (" << drawComp.tintR << ", " << drawComp.tintG << ", " << drawComp.tintB << ")" << std::endl;
    std::cout << "Scale: " << drawComp.spriteScale << std::endl;
    std::cout << "Position: (" << pos.x << ", " << pos.y << ", " << pos.z << ")" << std::endl;

    std::cout << "Basic component test passed!" << std::endl;
    return 0;
}