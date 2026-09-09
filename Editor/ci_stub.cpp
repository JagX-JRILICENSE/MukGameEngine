#ifdef _WIN32
#include <Windows.h>
#include <iostream>

int main() {
    std::cout << "Muk Game Engine v0.14 — Windows build artifact\n";
    std::cout << "Full editor sources are in the repo; this CI package is a runnable stub\n";
    std::cout << "Build locally with VS2022 for the complete DX12 editor.\n";
    MessageBoxA(nullptr,
        "Muk Game Engine (Windows)\n\n"
        "CI package OK.\n"
        "Clone the repo and build with CMake + VS2022 for the full editor.\n\n"
        "https://github.com/JagX-JRILICENSE/MukGameEngine",
        "Muk Game Engine",
        MB_OK | MB_ICONINFORMATION);
    return 0;
}
#else
int main() { return 0; }
#endif
