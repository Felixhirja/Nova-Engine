#include "ConfigEditor.h"
#include "ConfigEditorImGuiUI.h"
#include <iostream>

int main() {
    std::cout << "Testing Config Editor..." << std::endl;
    
    // Test ConfigEditor basic functionality
    nova::config::ConfigEditor editor;
    if (!editor.Initialize()) {
        std::cerr << "Failed to initialize ConfigEditor" << std::endl;
        return 1;
    }
    
    std::cout << "ConfigEditor initialized successfully" << std::endl;
    
    // Test ImGui UI creation
    nova::config::ConfigEditorImGuiUI ui(&editor);
    if (!ui.Initialize()) {
        std::cerr << "Failed to initialize ConfigEditorImGuiUI" << std::endl;
        return 1;
    }
    
    std::cout << "ConfigEditorImGuiUI initialized successfully" << std::endl;
    
    // Test basic functionality
    std::cout << "Testing CanUndo: " << (editor.CanUndo() ? "true" : "false") << std::endl;
    std::cout << "Testing CanRedo: " << (editor.CanRedo() ? "true" : "false") << std::endl;
    std::cout << "Testing GetCurrentFile: '" << editor.GetCurrentFile() << "'" << std::endl;
    
    // Test that we can access the current form (should be empty initially)
    const auto& form = editor.GetCurrentForm();
    std::cout << "Current form has " << form.fields.size() << " fields" << std::endl;
    
    ui.Shutdown();
    editor.Shutdown();
    
    std::cout << "Config Editor test completed successfully!" << std::endl;
    return 0;
}