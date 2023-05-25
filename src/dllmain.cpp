#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning(push, 0)
#include <cocos2d.h>
#pragma warning(pop)
#include <MinHook.h>
#include <gd.h>
#include <chrono>
#include <thread>
#include <random>

LONG_PTR oWindowProc;
bool isClosing = false;
bool newWindowProcSet = false;

LRESULT CALLBACK nWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
    case WM_DESTROY:
    case WM_QUIT:
    case WM_CLOSE:
        msg = 0;
        cocos2d::CCDirector::sharedDirector()->end();
        break;
    }

    return CallWindowProc((WNDPROC)oWindowProc, hwnd, msg, wparam, lparam);
}

void(__thiscall* CCDirector_end)(cocos2d::CCDirector* self);

void __fastcall CCDirector_end_H(cocos2d::CCDirector* self, void*) {

    HWND hwnd = GetActiveWindow();

    ShowWindow(hwnd, 0);

    gd::GameSoundManager::sharedState()->stopBackgroundMusic();
    std::string str = "http://www.robtopgames.net/database/accounts/backupGJAccountNew.php";
    gd::GJAccountManager::sharedState()->backupAccount(str);
    isClosing = true;
}

bool(__thiscall* MenuLayer_init)(gd::MenuLayer* self);

bool __fastcall MenuLayer_init_H(gd::MenuLayer* self, void*) {

    if (!MenuLayer_init(self)) return false;

    if(!newWindowProcSet) oWindowProc = SetWindowLongPtrA(GetForegroundWindow(), GWL_WNDPROC, (LONG_PTR)nWindowProc);
    newWindowProcSet = true;

    return true;
}

void(__thiscall* GJAccountManager_onBackupAccountCompleted)(gd::GJAccountManager* self, std::string, std::string);

void __fastcall GJAccountManager_onBackupAccountCompleted_H(gd::GJAccountManager* self, void*, std::string param1, std::string param2) {

    GJAccountManager_onBackupAccountCompleted(self, param1, param2);

    if(isClosing) CCDirector_end(cocos2d::CCDirector::sharedDirector());
}

DWORD WINAPI thread_func(void* hModule) {
    MH_Initialize();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distr(100, 2000);

    int random = distr(gen);

    std::this_thread::sleep_for(std::chrono::milliseconds(random));

    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    auto addr = GetProcAddress(GetModuleHandleA("libcocos2d.dll"), "?end@CCDirector@cocos2d@@QAEXXZ");
    MH_CreateHook(
        reinterpret_cast<void*>(addr),
        CCDirector_end_H,
        reinterpret_cast<void**>(&CCDirector_end)
    );
    MH_CreateHook(
        reinterpret_cast<void*>(base + 0x109620),
        GJAccountManager_onBackupAccountCompleted_H,
        reinterpret_cast<void**>(&GJAccountManager_onBackupAccountCompleted)
    );
    MH_CreateHook(
        reinterpret_cast<void*>(base + 0x1907B0),
        MenuLayer_init_H,
        reinterpret_cast<void**>(&MenuLayer_init)
    );

    MH_EnableHook(MH_ALL_HOOKS);
    
    return 0;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {

    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0x100, thread_func, handle, 0, 0);
    }
    return TRUE;
}