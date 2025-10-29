# Menu System Quick Reference

> Need a map of how menu, HUD, and other frameworks plug into the runtime?
> Start with the [Engine Framework Overview](engine_overview.md).

## ✅ Current Status
**Build**: ✅ Compiles successfully  
**Infrastructure**: ✅ Complete  
**Integration**: 🔄 Pending MainLoop wiring

---

## 📁 Files

### Core Framework
- `src/MenuSystem.h` (112 lines) - Generic menu system
- `src/MenuSystem.cpp` (186 lines) - Navigation & animation logic

### Main Menu
- `src/MainMenu.h` (67 lines) - Nova Engine main menu
- `src/MainMenu.cpp` (98 lines) - Callbacks & input handling

### Rendering
- `src/Viewport3D.h` - Added `DrawMenu()` method
- `src/Viewport3D.cpp` - OpenGL menu rendering (+100 lines)

---

## 🎮 Main Menu Options

| Option | Action | Status |
|--------|--------|--------|
| **New Game** | Start new adventure | ✅ Callback defined |
| **Continue** | Resume saved game | ✅ Can be disabled |
| **Settings** | Open settings | ⚠️ Placeholder |
| **Quit** | Exit game | ✅ Sets Quit action |

---

## 🔧 API Cheat Sheet

### Create Menu
```cpp
MenuSystem menu("Title");
menu.AddItem("Option", []() { /* action */ });
menu.SetStyle(customStyle);
```

### Navigation
```cpp
menu.SelectNext();          // Arrow down
menu.SelectPrevious();      // Arrow up  
menu.ActivateSelectedItem(); // Enter key
```

### Mouse Input
```cpp
menu.HandleMouseMove(x, y);   // Hover
menu.HandleMouseClick(x, y);  // Click
```

### Rendering
```cpp
viewport3D.DrawMenu(
    menu.GetTitle(),
    menu.GetItems(), 
    menu.GetSelectedIndex(),
    menu.GetStyle()
);
```

---

## 🎨 Styling

```cpp
MenuSystem::MenuStyle style;
style.titleColor = {1.0f, 0.8f, 0.0f, 1.0f}; // Gold
style.selectedItemColor = {1.0f, 1.0f, 1.0f, 1.0f}; // White
style.normalItemColor = {0.7f, 0.7f, 0.7f, 1.0f}; // Gray
style.disabledItemColor = {0.3f, 0.3f, 0.3f, 1.0f}; // Dark gray
style.titleFontSize = FontSize::Large;
style.itemFontSize = FontSize::Medium;
style.itemSpacing = 50;
```

---

## ⚡ Next Steps

1. **Add to MainLoop** - Create GameState enum, add menu instance
2. **Wire Input** - Connect GLFW callbacks to menu handlers  
3. **State Transitions** - Handle New Game → Gameplay transition
4. **Test** - Unit tests for navigation, wrapping, callbacks

---

## 🐛 Known Issues

- None! Menu system compiles cleanly
- Only harmless unused parameter warnings

---

## 💡 Usage Example

```cpp
// In MainLoop.h:
enum class GameState { MAIN_MENU, PLAYING, PAUSED };
GameState currentState_ = GameState::MAIN_MENU;
MainMenu mainMenu_;

// In MainLoop::Update():
if (currentState_ == GameState::MAIN_MENU) {
    mainMenu_.Update(deltaTime);
    
    // Check for actions:
    if (mainMenu_.GetLastAction() == MainMenu::Action::NewGame) {
        currentState_ = GameState::PLAYING;
        StartNewGame();
    }
    else if (mainMenu_.GetLastAction() == MainMenu::Action::Quit) {
        glfwSetWindowShouldClose(window_, true);
    }
}

// In MainLoop::Draw():
if (currentState_ == GameState::MAIN_MENU) {
    viewport3D_->DrawMenu(
        mainMenu_.GetTitle(),
        mainMenu_.GetRenderData().items,
        mainMenu_.GetRenderData().selectedIndex,
        mainMenu_.GetStyle()
    );
}

// In GLFW key callback:
if (currentState_ == GameState::MAIN_MENU) {
    mainMenu_.HandleKeyPress(key, action, mods);
}
```

---

## 📊 Progress Tracker

- [x] Design architecture
- [x] Implement MenuSystem framework  
- [x] Create MainMenu class
- [x] Add Viewport3D rendering
- [ ] Integrate into MainLoop
- [ ] Wire input handling
- [ ] Add state management
- [ ] Create unit tests
- [ ] Visual polish

**Current Milestone**: Core Implementation Complete ✅  
**Next Milestone**: MainLoop Integration 🔄
