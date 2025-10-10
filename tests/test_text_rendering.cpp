#include "TextRenderer.h"
#include <iostream>
#include <cassert>
#include <cmath>

#ifdef _WIN32
#include <GL/glut.h>
#else
#include <GL/glut.h>
#endif

/**
 * Test suite for TextRenderer
 * 
 * This demonstrates all text rendering capabilities:
 * - Basic text rendering
 * - Aligned text (left/center/right)
 * - Colored text
 * - Different font sizes
 * - Multi-line text blocks
 * - Text with shadows
 * - Formatted text (printf-style)
 * - Text measurement
 * - 3D world-space text
 */

void TestBasicRendering() {
    std::cout << "Testing basic text rendering..." << std::endl;
    
    // Test text measurement
    int width = TextRenderer::MeasureText("Hello World", FontSize::Medium);
    assert(width > 0);
    std::cout << "  'Hello World' width: " << width << " pixels" << std::endl;
    
    int height = TextRenderer::GetFontHeight(FontSize::Medium);
    assert(height > 0);
    std::cout << "  Font height: " << height << " pixels" << std::endl;
    
    // Test all font sizes
    std::cout << "  Font sizes:" << std::endl;
    std::cout << "    Small:  " << TextRenderer::GetFontHeight(FontSize::Small) << "px" << std::endl;
    std::cout << "    Medium: " << TextRenderer::GetFontHeight(FontSize::Medium) << "px" << std::endl;
    std::cout << "    Large:  " << TextRenderer::GetFontHeight(FontSize::Large) << "px" << std::endl;
    std::cout << "    Fixed:  " << TextRenderer::GetFontHeight(FontSize::Fixed) << "px" << std::endl;
}

void TestColors() {
    std::cout << "Testing text colors..." << std::endl;
    
    // Test predefined colors
    TextColor white = TextColor::White();
    assert(white.r == 1.0f && white.g == 1.0f && white.b == 1.0f);
    
    TextColor red = TextColor::Red();
    assert(red.r == 1.0f && red.g == 0.0f && red.b == 0.0f);
    
    TextColor custom(0.5f, 0.7f, 0.9f, 0.8f);
    assert(std::abs(custom.r - 0.5f) < 0.01f);
    
    std::cout << "  All color tests passed" << std::endl;
}

void TestAlignment() {
    std::cout << "Testing text alignment..." << std::endl;
    
    const char* testStr = "Aligned Text";
    int width = TextRenderer::MeasureText(testStr, FontSize::Medium);
    
    std::cout << "  Text width: " << width << " pixels" << std::endl;
    std::cout << "  Left alignment: offset = 0" << std::endl;
    std::cout << "  Center alignment: offset = " << (-width / 2) << std::endl;
    std::cout << "  Right alignment: offset = " << (-width) << std::endl;
}

/**
 * Visual test - requires OpenGL context
 * This would be called from MainLoop during rendering
 */
void VisualTestExample() {
    // Note: This should be called within an OpenGL rendering context
    // with proper 2D orthographic projection set up
    
    std::cout << "\n=== TextRenderer Visual Test Example ===" << std::endl;
    std::cout << "Call these functions from Viewport3D::DrawHUD():\n" << std::endl;
    
    std::cout << "// Basic text rendering" << std::endl;
    std::cout << "TextRenderer::RenderText(\"Hello World\", 10, 30, TextColor::White());" << std::endl;
    
    std::cout << "\n// Different colors and sizes" << std::endl;
    std::cout << "TextRenderer::RenderText(\"ERROR!\", 10, 60, TextColor::Red(), FontSize::Large);" << std::endl;
    std::cout << "TextRenderer::RenderText(\"Warning\", 10, 90, TextColor::Yellow(), FontSize::Medium);" << std::endl;
    std::cout << "TextRenderer::RenderText(\"OK\", 10, 120, TextColor::Green(), FontSize::Small);" << std::endl;
    
    std::cout << "\n// Aligned text (centered at x=400)" << std::endl;
    std::cout << "TextRenderer::RenderTextAligned(\"Centered\", 400, 150, TextAlign::Center, TextColor::Cyan());" << std::endl;
    std::cout << "TextRenderer::RenderTextAligned(\"Right Aligned\", 800, 180, TextAlign::Right, TextColor::Magenta());" << std::endl;
    
    std::cout << "\n// Formatted text (printf-style)" << std::endl;
    std::cout << "TextRenderer::RenderTextF(10, 210, TextColor::Orange(), FontSize::Medium," << std::endl;
    std::cout << "                         \"FPS: %.1f | Frame: %d\", 60.0, 1234);" << std::endl;
    
    std::cout << "\n// Text with shadow for better readability" << std::endl;
    std::cout << "TextRenderer::RenderTextWithShadow(\"Important Info\", 10, 240, " << std::endl;
    std::cout << "                                   TextColor::White(), TextColor::Black());" << std::endl;
    
    std::cout << "\n// Multi-line text block with word wrapping" << std::endl;
    std::cout << "std::string longText = \"This is a longer message that demonstrates \"" << std::endl;
    std::cout << "                       \"automatic word wrapping at specified width.\";" << std::endl;
    std::cout << "TextRenderer::RenderTextBlock(longText, 10, 270, 300, TextColor::Gray(0.8f));" << std::endl;
    
    std::cout << "\n// HUD info display example" << std::endl;
    std::cout << "int y = 30;" << std::endl;
    std::cout << "TextRenderer::RenderTextF(10, y, TextColor::Yellow(), FontSize::Medium, \"SHIELDS: %d%%\", 85);" << std::endl;
    std::cout << "y += 20;" << std::endl;
    std::cout << "TextRenderer::RenderTextF(10, y, TextColor::Red(), FontSize::Medium, \"HULL: %d%%\", 42);" << std::endl;
    std::cout << "y += 20;" << std::endl;
    std::cout << "TextRenderer::RenderTextF(10, y, TextColor::Cyan(), FontSize::Medium, \"POWER: %.1f MW\", 125.7);" << std::endl;
    
    std::cout << "\n=== End Visual Test Example ===" << std::endl;
}

/**
 * Example: Spaceship HUD with text rendering
 */
void SpaceshipHUDExample() {
    std::cout << "\n=== Spaceship HUD Example ===" << std::endl;
    std::cout << "Integration with ShipAssemblyResult:\n" << std::endl;
    
    std::cout << "void DrawSpaceshipHUD(const ShipAssemblyResult* ship, int screenWidth, int screenHeight) {" << std::endl;
    std::cout << "    if (!ship) return;" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    int x = 10;" << std::endl;
    std::cout << "    int y = 30;" << std::endl;
    std::cout << "    int lineHeight = 20;" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    // Ship name (centered at top)" << std::endl;
    std::cout << "    std::string shipName = ship->hull->displayName;" << std::endl;
    std::cout << "    TextRenderer::RenderTextAligned(shipName, screenWidth / 2, 10," << std::endl;
    std::cout << "                                   TextAlign::Center, TextColor::Cyan(), FontSize::Large);" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    // Power status (color-coded)" << std::endl;
    std::cout << "    double netPower = ship->NetPowerMW();" << std::endl;
    std::cout << "    TextColor powerColor = netPower >= 0 ? TextColor::Green() : TextColor::Red();" << std::endl;
    std::cout << "    TextRenderer::RenderTextF(x, y, powerColor, FontSize::Medium," << std::endl;
    std::cout << "                             \"NET POWER: %+.1f MW\", netPower);" << std::endl;
    std::cout << "    y += lineHeight;" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    // Power details" << std::endl;
    std::cout << "    TextRenderer::RenderTextF(x, y, TextColor::Gray(0.8f), FontSize::Small," << std::endl;
    std::cout << "                             \"  Output: %.1f MW | Draw: %.1f MW\"," << std::endl;
    std::cout << "                             ship->totalPowerOutputMW, ship->totalPowerDrawMW);" << std::endl;
    std::cout << "    y += lineHeight;" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    // Thrust to mass ratio (performance metric)" << std::endl;
    std::cout << "    double tmr = ship->ThrustToMassRatio();" << std::endl;
    std::cout << "    TextColor thrustColor = tmr > 2.0 ? TextColor::Green() :" << std::endl;
    std::cout << "                           tmr > 1.0 ? TextColor::Yellow() : TextColor::Orange();" << std::endl;
    std::cout << "    TextRenderer::RenderTextF(x, y, thrustColor, FontSize::Medium," << std::endl;
    std::cout << "                             \"T/M RATIO: %.2f\", tmr);" << std::endl;
    std::cout << "    y += lineHeight;" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    // Component counts" << std::endl;
    std::cout << "    TextRenderer::RenderTextF(x, y, TextColor::White(), FontSize::Medium," << std::endl;
    std::cout << "                             \"COMPONENTS: %zu\", ship->components.size());" << std::endl;
    std::cout << "    y += lineHeight;" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    // Warnings/Errors (if any)" << std::endl;
    std::cout << "    if (!ship->diagnostics.errors.empty()) {" << std::endl;
    std::cout << "        TextRenderer::RenderTextWithShadow(\"ASSEMBLY ERROR\"," << std::endl;
    std::cout << "                                          x, y," << std::endl;
    std::cout << "                                          TextColor::Red()," << std::endl;
    std::cout << "                                          TextColor::Black()," << std::endl;
    std::cout << "                                          FontSize::Large);" << std::endl;
    std::cout << "        y += lineHeight + 5;" << std::endl;
    std::cout << "        " << std::endl;
    std::cout << "        // Show first error with wrapping" << std::endl;
    std::cout << "        TextRenderer::RenderTextBlock(ship->diagnostics.errors[0]," << std::endl;
    std::cout << "                                     x, y, 400," << std::endl;
    std::cout << "                                     TextColor::Red(), FontSize::Small);" << std::endl;
    std::cout << "    } else if (!ship->diagnostics.warnings.empty()) {" << std::endl;
    std::cout << "        TextRenderer::RenderText(ship->diagnostics.warnings[0]," << std::endl;
    std::cout << "                                x, y," << std::endl;
    std::cout << "                                TextColor::Yellow()," << std::endl;
    std::cout << "                                FontSize::Small);" << std::endl;
    std::cout << "    }" << std::endl;
    std::cout << "}" << std::endl;
    
    std::cout << "\n=== End Spaceship HUD Example ===" << std::endl;
}

/**
 * Example: Energy Management HUD
 */
void EnergyManagementHUDExample() {
    std::cout << "\n=== Energy Management HUD Example ===" << std::endl;
    std::cout << "Real-time power distribution display:\n" << std::endl;
    
    std::cout << "void DrawEnergyHUD(const EnergyManagementSystem* energy, int x, int y) {" << std::endl;
    std::cout << "    if (!energy) return;" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    // Title" << std::endl;
    std::cout << "    TextRenderer::RenderText(\"ENERGY MANAGEMENT\", x, y," << std::endl;
    std::cout << "                            TextColor::Cyan(), FontSize::Large);" << std::endl;
    std::cout << "    y += 25;" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    // Power bars for each subsystem" << std::endl;
    std::cout << "    struct Subsystem { const char* name; double allocation; };" << std::endl;
    std::cout << "    Subsystem systems[] = {" << std::endl;
    std::cout << "        {\"SHIELDS\",  energy->GetShieldAllocation()}," << std::endl;
    std::cout << "        {\"WEAPONS\",  energy->GetWeaponAllocation()}," << std::endl;
    std::cout << "        {\"ENGINES\",  energy->GetEngineAllocation()}" << std::endl;
    std::cout << "    };" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    for (const auto& sys : systems) {" << std::endl;
    std::cout << "        // System name" << std::endl;
    std::cout << "        TextRenderer::RenderTextF(x, y, TextColor::White(), FontSize::Medium," << std::endl;
    std::cout << "                                 \"%-10s\", sys.name);" << std::endl;
    std::cout << "        " << std::endl;
    std::cout << "        // Percentage bar" << std::endl;
    std::cout << "        int barX = x + 100;" << std::endl;
    std::cout << "        int barWidth = (int)(sys.allocation * 200);" << std::endl;
    std::cout << "        " << std::endl;
    std::cout << "        // Color based on allocation" << std::endl;
    std::cout << "        TextColor barColor = sys.allocation > 0.75 ? TextColor::Green() :" << std::endl;
    std::cout << "                            sys.allocation > 0.5  ? TextColor::Yellow() :" << std::endl;
    std::cout << "                                                    TextColor::Red();" << std::endl;
    std::cout << "        " << std::endl;
    std::cout << "        // Percentage text" << std::endl;
    std::cout << "        TextRenderer::RenderTextF(barX + 210, y, barColor, FontSize::Medium," << std::endl;
    std::cout << "                                 \"%3.0f%%\", sys.allocation * 100);" << std::endl;
    std::cout << "        " << std::endl;
    std::cout << "        y += 20;" << std::endl;
    std::cout << "    }" << std::endl;
    std::cout << "    " << std::endl;
    std::cout << "    // Total power available" << std::endl;
    std::cout << "    y += 10;" << std::endl;
    std::cout << "    TextRenderer::RenderTextF(x, y, TextColor::Gray(0.7f), FontSize::Small," << std::endl;
    std::cout << "                             \"Total Available: %.1f MW\", energy->GetTotalPower());" << std::endl;
    std::cout << "}" << std::endl;
    
    std::cout << "\n=== End Energy Management HUD Example ===" << std::endl;
}

int main() {
    std::cout << "=== TextRenderer Test Suite ===" << std::endl;
    std::cout << std::endl;
    
    // Initialize GLUT (required for bitmap font functions)
    int argc = 0;
    char* argv[] = { nullptr };
    glutInit(&argc, argv);
    
    std::cout << "GLUT initialized for font system" << std::endl;
    std::cout << std::endl;
    
    // Run unit tests
    TestBasicRendering();
    TestColors();
    TestAlignment();
    
    // Show visual examples
    VisualTestExample();
    SpaceshipHUDExample();
    EnergyManagementHUDExample();
    
    std::cout << "\n=== All Tests Completed ===" << std::endl;
    std::cout << "\nNext steps:" << std::endl;
    std::cout << "1. Build the project: make" << std::endl;
    std::cout << "2. Integrate TextRenderer into Viewport3D::DrawHUD()" << std::endl;
    std::cout << "3. Replace glutBitmapCharacter calls with TextRenderer methods" << std::endl;
    std::cout << "4. Add text rendering to other UI elements" << std::endl;
    
    return 0;
}
