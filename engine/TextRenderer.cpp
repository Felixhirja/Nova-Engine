#ifdef USE_GLFW

#include "TextRenderer.h"
#include <cstdarg>
#include <cstring>
#include <vector>
#include <sstream>

#ifdef _WIN32
#include <GL/glut.h>
#else
#include <GL/glut.h>
#endif
#include <mutex>

namespace {
    void EnsureGlutInitialized() {
        static std::once_flag once;
        std::call_once(once, []() {
            int argc = 1;
            char argv0[] = "nova";
            char* argv[] = { argv0, nullptr };
            glutInit(&argc, argv);
            // No need to create a GLUT window; we rely on the existing GL context (GLFW/SDL)
        });
    }
}

void* TextRenderer::GetGLUTFont(FontSize size) {
    switch (size) {
        case FontSize::Small:
            return GLUT_BITMAP_8_BY_13;
        case FontSize::Medium:
            return GLUT_BITMAP_HELVETICA_12;
        case FontSize::Large:
            return GLUT_BITMAP_HELVETICA_18;
        case FontSize::Fixed:
            return GLUT_BITMAP_9_BY_15;
        default:
            return GLUT_BITMAP_HELVETICA_12;
    }
}

int TextRenderer::GetFontHeight(FontSize size) {
    switch (size) {
        case FontSize::Small:  return 13;
        case FontSize::Medium: return 12;
        case FontSize::Large:  return 18;
        case FontSize::Fixed:  return 15;
        default: return 12;
    }
}

void TextRenderer::RenderChar(char c, void* font) {
    glutBitmapCharacter(font, c);
}

void TextRenderer::RenderText(const std::string& text, 
                             int x, int y, 
                             const TextColor& color,
                             FontSize size) {
    if (text.empty()) return;
    EnsureGlutInitialized();
    
    void* font = GetGLUTFont(size);
    
    // Set color
    glColor4f(color.r, color.g, color.b, color.a);
    
    // Set raster position
    glRasterPos2i(x, y);
    
    // Render each character
    for (char c : text) {
        if (c == '\n') {
            // Handle newline
            y += GetFontHeight(size) + 4; // 4 pixels line spacing
            glRasterPos2i(x, y);
        } else {
            RenderChar(c, font);
        }
    }
}

void TextRenderer::RenderTextAligned(const std::string& text,
                                    int x, int y,
                                    TextAlign align,
                                    const TextColor& color,
                                    FontSize size) {
    if (text.empty()) return;
    EnsureGlutInitialized();
    
    int offsetX = 0;
    
    switch (align) {
        case TextAlign::Center:
            offsetX = -MeasureText(text, size) / 2;
            break;
        case TextAlign::Right:
            offsetX = -MeasureText(text, size);
            break;
        case TextAlign::Left:
        default:
            offsetX = 0;
            break;
    }
    
    RenderText(text, x + offsetX, y, color, size);
}

void TextRenderer::RenderText3D(const std::string& text,
                               double worldX, double worldY, double worldZ,
                               const TextColor& color,
                               FontSize size) {
    if (text.empty()) return;
    EnsureGlutInitialized();
    
    void* font = GetGLUTFont(size);
    
    // Set color
    glColor4f(color.r, color.g, color.b, color.a);
    
    // Set raster position in 3D world space
    glRasterPos3d(worldX, worldY, worldZ);
    
    // Render each character
    for (char c : text) {
        RenderChar(c, font);
    }
}

int TextRenderer::MeasureText(const std::string& text, FontSize size) {
    if (text.empty()) return 0;
    EnsureGlutInitialized();
    
    void* font = GetGLUTFont(size);
    int width = 0;
    
    for (char c : text) {
        if (c == '\n') break; // Stop at newline for single-line measure
        width += glutBitmapWidth(font, c);
    }
    
    return width;
}

void TextRenderer::RenderTextF(int x, int y, 
                              const TextColor& color, 
                              FontSize size,
                              const char* format, ...) {
    EnsureGlutInitialized();
    char buffer[512];
    
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    RenderText(std::string(buffer), x, y, color, size);
}

int TextRenderer::RenderTextBlock(const std::string& text,
                                 int x, int y,
                                 int maxWidth,
                                 const TextColor& color,
                                 FontSize size,
                                 int lineSpacing) {
    if (text.empty()) return 0;
    
    void* font = GetGLUTFont(size);
    int fontHeight = GetFontHeight(size);
    int currentY = y;
    
    std::istringstream stream(text);
    std::string line;
    
    while (std::getline(stream, line)) {
        if (maxWidth > 0) {
            // Word wrapping enabled
            std::string word;
            std::istringstream lineStream(line);
            std::string currentLine;
            int currentWidth = 0;
            
            while (lineStream >> word) {
                int wordWidth = MeasureText(word + " ", size);
                
                if (currentWidth + wordWidth > maxWidth && !currentLine.empty()) {
                    // Render current line and start new one
                    RenderText(currentLine, x, currentY, color, size);
                    currentY += fontHeight + lineSpacing;
                    currentLine = word + " ";
                    currentWidth = wordWidth;
                } else {
                    currentLine += word + " ";
                    currentWidth += wordWidth;
                }
            }
            
            // Render remaining line
            if (!currentLine.empty()) {
                RenderText(currentLine, x, currentY, color, size);
                currentY += fontHeight + lineSpacing;
            }
        } else {
            // No word wrapping
            RenderText(line, x, currentY, color, size);
            currentY += fontHeight + lineSpacing;
        }
    }
    
    return currentY - y; // Total height
}

void TextRenderer::RenderTextWithShadow(const std::string& text,
                                       int x, int y,
                                       const TextColor& color,
                                       const TextColor& shadowColor,
                                       FontSize size) {
    if (text.empty()) return;
    
    // Render shadow (offset by 1-2 pixels)
    RenderText(text, x + 1, y + 1, shadowColor, size);
    
    // Render main text
    RenderText(text, x, y, color, size);
}

#else // !USE_GLFW

#include "TextRenderer.h"
#include <cstdarg>

void* TextRenderer::GetGLUTFont(FontSize) {
    return nullptr;
}

int TextRenderer::GetFontHeight(FontSize) {
    return 0;
}

void TextRenderer::RenderChar(char, void*) {}

void TextRenderer::RenderText(const std::string&, int, int, const TextColor&, FontSize) {}

void TextRenderer::RenderTextAligned(const std::string&, int, int, TextAlign, const TextColor&, FontSize) {}

void TextRenderer::RenderText3D(const std::string&, double, double, double, const TextColor&, FontSize) {}

int TextRenderer::MeasureText(const std::string&, FontSize) {
    return 0;
}

void TextRenderer::RenderTextF(int, int, const TextColor&, FontSize, const char*, ...) {}

int TextRenderer::RenderTextBlock(const std::string&, int, int, int, const TextColor&, FontSize, int) {
    return 0;
}

void TextRenderer::RenderTextWithShadow(const std::string&, int, int, const TextColor&, const TextColor&, FontSize) {}

#endif // USE_GLFW
