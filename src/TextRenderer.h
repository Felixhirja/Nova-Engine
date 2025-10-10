#pragma once

#include <string>
#include <cstdint>

// Color struct for text rendering
struct TextColor {
    float r, g, b, a;
    
    TextColor() : r(1.0f), g(1.0f), b(1.0f), a(1.0f) {}
    TextColor(float red, float green, float blue, float alpha = 1.0f)
        : r(red), g(green), b(blue), a(alpha) {}
    
    // Predefined colors
    static TextColor White()   { return {1.0f, 1.0f, 1.0f, 1.0f}; }
    static TextColor Black()   { return {0.0f, 0.0f, 0.0f, 1.0f}; }
    static TextColor Red()     { return {1.0f, 0.0f, 0.0f, 1.0f}; }
    static TextColor Green()   { return {0.0f, 1.0f, 0.0f, 1.0f}; }
    static TextColor Blue()    { return {0.0f, 0.0f, 1.0f, 1.0f}; }
    static TextColor Yellow()  { return {1.0f, 1.0f, 0.0f, 1.0f}; }
    static TextColor Cyan()    { return {0.0f, 1.0f, 1.0f, 1.0f}; }
    static TextColor Magenta() { return {1.0f, 0.0f, 1.0f, 1.0f}; }
    static TextColor Orange()  { return {1.0f, 0.6f, 0.0f, 1.0f}; }
    static TextColor Gray(float brightness = 0.5f) { return {brightness, brightness, brightness, 1.0f}; }
};

// Font sizes (GLUT bitmap fonts)
enum class FontSize {
    Small,      // GLUT_BITMAP_8_BY_13
    Medium,     // GLUT_BITMAP_HELVETICA_12
    Large,      // GLUT_BITMAP_HELVETICA_18
    Fixed       // GLUT_BITMAP_9_BY_15 (monospace)
};

// Text alignment options
enum class TextAlign {
    Left,
    Center,
    Right
};

/**
 * TextRenderer - Simple text rendering system using GLUT bitmap fonts
 * 
 * Provides easy-to-use text rendering for HUD elements, debug info, and UI.
 * Works with both 2D (orthographic) and 3D (world space) text.
 * 
 * Example usage:
 *   TextRenderer::RenderText("Hello World", 10, 30, TextColor::White(), FontSize::Large);
 *   
 *   int width = TextRenderer::MeasureText("Test", FontSize::Medium);
 *   
 *   TextRenderer::RenderTextAligned("Centered", 400, 300, 
 *                                   TextAlign::Center, TextColor::Yellow());
 */
class TextRenderer {
public:
    /**
     * Render text at screen coordinates (2D overlay mode)
     * @param text String to render
     * @param x Screen X coordinate (pixels)
     * @param y Screen Y coordinate (pixels from top)
     * @param color Text color (default white)
     * @param size Font size (default medium)
     */
    static void RenderText(const std::string& text, 
                          int x, int y, 
                          const TextColor& color = TextColor::White(),
                          FontSize size = FontSize::Medium);
    
    /**
     * Render text with alignment (2D overlay mode)
     * @param text String to render
     * @param x Screen X coordinate (alignment reference point)
     * @param y Screen Y coordinate (pixels from top)
     * @param align Text alignment (left/center/right)
     * @param color Text color (default white)
     * @param size Font size (default medium)
     */
    static void RenderTextAligned(const std::string& text,
                                 int x, int y,
                                 TextAlign align,
                                 const TextColor& color = TextColor::White(),
                                 FontSize size = FontSize::Medium);
    
    /**
     * Render text in 3D world space
     * @param text String to render
     * @param worldX World X coordinate
     * @param worldY World Y coordinate
     * @param worldZ World Z coordinate
     * @param color Text color (default white)
     * @param size Font size (default medium)
     */
    static void RenderText3D(const std::string& text,
                            double worldX, double worldY, double worldZ,
                            const TextColor& color = TextColor::White(),
                            FontSize size = FontSize::Medium);
    
    /**
     * Measure text width in pixels
     * @param text String to measure
     * @param size Font size
     * @return Width in pixels
     */
    static int MeasureText(const std::string& text, FontSize size = FontSize::Medium);
    
    /**
     * Get font height in pixels
     * @param size Font size
     * @return Height in pixels
     */
    static int GetFontHeight(FontSize size = FontSize::Medium);
    
    /**
     * Render formatted text (printf-style)
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param color Text color
     * @param size Font size
     * @param format Printf-style format string
     */
    static void RenderTextF(int x, int y, 
                           const TextColor& color, 
                           FontSize size,
                           const char* format, ...);
    
    /**
     * Render multi-line text with automatic line wrapping
     * @param text String to render (newlines create line breaks)
     * @param x Screen X coordinate
     * @param y Screen Y coordinate (top of text block)
     * @param maxWidth Maximum width before wrapping (0 = no wrap)
     * @param color Text color
     * @param size Font size
     * @param lineSpacing Extra pixels between lines (default 4)
     * @return Total height of rendered text block
     */
    static int RenderTextBlock(const std::string& text,
                              int x, int y,
                              int maxWidth = 0,
                              const TextColor& color = TextColor::White(),
                              FontSize size = FontSize::Medium,
                              int lineSpacing = 4);
    
    /**
     * Render text with shadow/outline for better readability
     * @param text String to render
     * @param x Screen X coordinate
     * @param y Screen Y coordinate
     * @param color Text color
     * @param shadowColor Shadow color (default black)
     * @param size Font size
     */
    static void RenderTextWithShadow(const std::string& text,
                                    int x, int y,
                                    const TextColor& color = TextColor::White(),
                                    const TextColor& shadowColor = TextColor::Black(),
                                    FontSize size = FontSize::Medium);

private:
    // Get GLUT font pointer for size
    static void* GetGLUTFont(FontSize size);
    
    // Render single character (internal helper)
    static void RenderChar(char c, void* font);
};
