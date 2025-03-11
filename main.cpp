#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <string>
#include <vector>

// Structure to store exported function info
struct ExportedFunction {
    std::string name;
    DWORD startAddr;
    DWORD endAddr;
};

// Global variables
std::string selectedFile = "";
std::vector<ExportedFunction> exportedFunctions;

// Function to open file dialog and select a DLL
std::string OpenFileDialog() {
    char filename[MAX_PATH] = "";

    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = "DLL Files\0*.dll\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = "dll";

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
    return "";
}

// Function to extract exported functions from a DLL
std::vector<ExportedFunction> GetExportedFunctions(const std::string& dllPath) {
    std::vector<ExportedFunction> exports;
    HMODULE hModule = LoadLibraryA(dllPath.c_str());
    if (!hModule) {
        std::cerr << "Failed to load DLL: " << dllPath << std::endl;
        return exports;
    }

    auto dosHeader = (PIMAGE_DOS_HEADER)hModule;
    auto ntHeaders = (PIMAGE_NT_HEADERS)((BYTE*)hModule + dosHeader->e_lfanew);
    auto exportDirRVA = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;

    if (!exportDirRVA) {
        std::cerr << "No exports found in DLL." << std::endl;
        FreeLibrary(hModule);
        return exports;
    }

    auto exportDir = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hModule + exportDirRVA);
    auto names = (DWORD*)((BYTE*)hModule + exportDir->AddressOfNames);
    auto funcs = (DWORD*)((BYTE*)hModule + exportDir->AddressOfFunctions);
    auto ordinals = (WORD*)((BYTE*)hModule + exportDir->AddressOfNameOrdinals);

    for (DWORD i = 0; i < exportDir->NumberOfNames; i++) {
        std::string funcName = (char*)((BYTE*)hModule + names[i]);
        DWORD startAddr = (DWORD)hModule + funcs[ordinals[i]];
        DWORD endAddr = startAddr + 10;

        exports.push_back({ funcName, startAddr, endAddr });
    }

    FreeLibrary(hModule);
    return exports;
}

// Function to render the UI
void RenderUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin("DLL Export Viewer", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

    // Row with Browse and Clear buttons
    ImGui::Text("Select DLL:");
    ImGui::SameLine();
    if (ImGui::Button("Browse for DLL")) {
        std::string file = OpenFileDialog();
        if (!file.empty()) {
            selectedFile = file;
            exportedFunctions = GetExportedFunctions(selectedFile);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        selectedFile.clear();
        exportedFunctions.clear();
    }

    ImGui::Separator();
    ImGui::Text("Selected File:");
    ImGui::TextWrapped("%s", selectedFile.c_str());

    ImGui::Separator();
    ImGui::Text("Exported Functions:");

    if (ImGui::BeginTable("ExportTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable , ImVec2(0, 400))) {
        ImGui::TableSetupColumn("Function Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Start Address", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("End Address", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();

        for (const auto& func : exportedFunctions) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", func.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("0x%08X", func.startAddr);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("0x%08X", func.endAddr);
        }
        ImGui::EndTable();
    }

    ImGui::End();
}

int main() {
    if (!glfwInit()) return -1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "DLL Export Viewer", NULL, NULL);
    if (!window) return -1;
    glfwMakeContextCurrent(window);
    glewInit();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        RenderUI();

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
