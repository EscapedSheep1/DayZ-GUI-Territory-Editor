#include "Application.h"
#include <iostream>

int main(int argc, char* argv[]) {
    Application app;
    
    if (!app.Initialize()) {
        std::cerr << "Failed to initialize application\n";
        return 1;
    }
    
    // Load file from command line if provided
    if (argc > 1) {
        std::string filePath = argv[1];
        // Note: In a real implementation, you'd call a method on app to load the file
        // For now, the user can use File > Open in the UI
        std::cout << "To load a file, use File > Open in the application menu" << std::endl;
        std::cout << "Example: " << filePath << std::endl;
    }
    
    app.Run();
    app.Shutdown();
    
    return 0;
}

