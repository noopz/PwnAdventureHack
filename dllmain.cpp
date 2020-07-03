#include <Windows.h>
#include <iostream>
#include <TlHelp32.h>
#include <mutex>
#include "mem.h"
#include "hook.h"
#include "pa3classes.h"

// Need to make sure _ITERATOR_DEBUG_LEVEL=0 when doing dll injection as the application/dlls are generally in release mode
// and when the iterator level is > 0, MSVC adds additional things to std classes, which causes a mismatch.

// Some globals to be lazy
bool enableFlyHack = false;
bool enableSpeedHack = false;
Player* g_player;
uintptr_t g_ModuleBase;
ClientWorld* g_ClientWorld;
char buf[256] = { 0 };

// Get the global GameAPI Object
typedef void* (__cdecl* tGetGameAPIObject)();
tGetGameAPIObject GetGameAPIObject = nullptr;

// Template for ClientWorld::Chat
typedef void(__thiscall* tClientWorldChat)(ClientWorld* pThis, Player* param_1, std::string* param_2);
tClientWorldChat tClientWorldChatGateway = nullptr;

// Template for Player::Chat
typedef void (__thiscall* tPlayerChat)(Player* pThis, char const* message);
tPlayerChat tPlayerChatGateway = nullptr;

// When hooking __thiscall*, it must be done by actually using __fastcall. __thiscalls actually populate
// the ECX register with the address of "this". Leveraging a __fastcall will take care of this, however
// the important thing to note is that __fastcall also requires that the prototype have a second argument
// which is tied to EDX. For the hooks below EDX can just be ignored.

// Not really used for much more than reversing and introspection since it makes it easier just grabbing
// address this way. Saves a lot of time with CheatEngine and ReClass
void __fastcall hClientWorldChat(ClientWorld* pThis, void* _edx, Player* pPlayer, std::string* param_2)
{
    printf("ClientWorldChat : %s\n", param_2->c_str());
    printf("ClientWorldPtr: %p\n", pThis);
    printf("PlayerPtr for %s: (%p)\n", pPlayer->GetPlayerName(), pThis);
    printf("ILocalPlayerPtr %p\n", pPlayer->GetLocalPlayer());

    tClientWorldChatGateway(pThis, pPlayer, param_2);
}

// All of the magic happens here. Leverage hooking Player::Chat to have an easy and consistent means
// of getting the pointer to the player. From there the sky is the limit (literally).
// Every time a message is sent by a player it will come to the hooked function first
// and then do whatever is desired. Additionally, since this is an internal hack
// it is possible to call Player::Chat in order to have messages show up in game for the current player.
void __fastcall hPlayerChat(Player* pThis, void* _edx, char const* pMessage)
{
    // Quick and dirty way to get a std::string so compare/erase can be used for matching
    // commands
    std::string message(pMessage);

    printf("ILocalPlayerPtr %p\n", pThis->GetLocalPlayer());
    // Log the mesage the player said to the console, along with the name and address.
    printf("PlayerPtr %s (%p) said:\n    %s\n", pThis->GetPlayerName(), pThis, pMessage);

    g_player = pThis;

    if (message.compare("/loc") == 0)
    {
        // From the ILocalPlayer it's possible to get to most other things
        ILocalPlayer* pLocalPlayer = pThis->GetLocalPlayer();

        // There's a lot of different values that map the current position of the player, however
        // only one of them actually has any direct impact on the actual location of the player.
        // Other values may just be the sent/recieved updates.
        Vector3* location = &pLocalPlayer->ptrToIActor->ptrToUE4Actor->m_position;
        // The "look position" is stored in a quaternion, and seems to be stored in a different object from
        // the players actual position. It was hard to determine exactly what this was, so just generically named
        // this for the purposed needed.
        Vector4* quat = &pLocalPlayer->ptrToIActor->ptrToGameObjects->ptrToCameraUE4ActorCamera->m_quaternion;

        sprintf_s(buf, 255, "Current poistion:%f %f %f", location->x, location->y, location->z);
        printf("%s\n", buf);
        pThis->Chat(buf);
        sprintf_s(buf, 255, "Current quaternion:%f %f %f %f", quat->x, quat->y, quat->z, quat->w);
        printf("%s\n", buf);
        pThis->Chat(buf);
    }

    if (message.compare(0, 7, "/flyhax") == 0)
    {
        enableFlyHack = !enableFlyHack;
        sprintf_s(buf, 255, "Fly hack %s.", enableFlyHack ? "enabled" : "disabled");
        pThis->Chat(buf);
    }

    // Teleport based on an x, y, z
    if (message.compare(0, 3, "/tp") == 0)
    {
        ILocalPlayer* pLocalPlayer = pThis->GetLocalPlayer();
        Vector3* location = &pLocalPlayer->ptrToIActor->ptrToUE4Actor->m_position;
        Vector3* newPostion = new Vector3();

        message.erase(0, 4);
        sscanf_s(message.c_str(), "%f %f %f", &(newPostion->x), &(newPostion->y), &(newPostion->z));

        printf("Telporting to:\n   %f %f %f\n", location->x, location->y, location->z);

        location->x = newPostion->x;
        location->y = newPostion->y;
        location->z = newPostion->z;

        pThis->Chat("Teleporting...");
    }

    // Run super fast and jump really high
    if (message.find("/speedhax") == 0)
    {
        enableSpeedHack = !enableSpeedHack;

        pThis->m_walkSpeed = enableSpeedHack ? 800.0f : 200.0f;
        pThis->m_jumpSpeed = enableSpeedHack ? 2000.0f : 400.0f;

        sprintf_s(buf, 255, "Speedhacks %s.", enableSpeedHack ? "enabled" : "disabled");
        pThis->Chat(buf);
    }


    // Now that we are finished with our internal hacks, continue on
    // with what was orignally supposed to happen by calling the gateway function
    // which will just restore the stack and jmp to the original function.
    tPlayerChatGateway(pThis, pMessage);
}

// Main thread for the internal dll, created once the dll has been sucessfully injected
// and loaded by the application. This is responsible for setting up the pointers to the
// base modules and creating the function Hooks.
DWORD WINAPI MainThread(HMODULE hModule)
{
    FILE* fp;
    // Setup a console and redirect stdout to it.
    AllocConsole();
    freopen_s(&fp, "CONOUT$", "w", stdout);

    printf("DLL sucessfully injected...\n");

    // Passing null gets the module handle to the "exe". In this case, it's the same as
    // calling the function with PwnAdventure3-Win32-Shipping.exe. It isn't necessary
    // for g_ModuleBase for anything in this current hack, but may be useful in the future
    g_ModuleBase = (uintptr_t)GetModuleHandle(NULL);

    // All of the logic that is needed to hook for this hack can be found in GameLogic.dll
    uintptr_t gameLogicBase = (uintptr_t)GetModuleHandle(L"GameLogic.dll");

    // This isn't actually used for anything, but with more research and time I suspect
    // that all objects including the ClientWorld can be found off of the GameAPI. Additionally
    // there may be functionality in the GameAPI that would be desirable to exploit.
    GetGameAPIObject = (tGetGameAPIObject)(gameLogicBase + 0x20ca0);
    printf("GameAPI Addr: %p\n", GetGameAPIObject());

    // The ClientWorld is another global in the GameLogic.dll, this is much more useful as it provides
    // the basis for hooking the Player::Chat function.
    g_ClientWorld = *(ClientWorld**)(gameLogicBase + 0x97d7c);
    printf("ClientWorld Addr: %p\n", (void*)g_ClientWorld);

    // Actually create a hook based off the ClientWorld + offset. The important thing to note here is that
    // a lenght of 6 is used, as thats the number of bytes that must be stolen for this partiuclar function.
    // To determine the length value, there are three options: brute force (not recommended), manual inspection
    // of the assembly code, or using a Length Disassembler Engine (LDE). This value was determined manually.
    Hook PlayerChatHook((BYTE*)(gameLogicBase + 0x551a0), (BYTE*)hPlayerChat, (BYTE*)&tPlayerChatGateway, 6);
    PlayerChatHook.Enable();

    // These values are used with the fly hack
    float step = 0.1f;
    float speed = 0.05f;
    Vector3 directionFromQuat;
    Vector3 newPosition;

    // Loop forever until the hack is terminated
    while (true)
    {
        if (GetAsyncKeyState(VK_END) & 1)
        {
            // Before cleaning up, disable the Hook (which will restore the stolen bytes)
            PlayerChatHook.Disable();
            break;
        }
        Sleep(5);

        if (enableFlyHack == true)
        {
            ILocalPlayer* pLocalPlayer = g_player->GetLocalPlayer();
            Vector3* location = &pLocalPlayer->ptrToIActor->ptrToUE4Actor->m_position;
            Vector4* quat = &pLocalPlayer->ptrToIActor->ptrToGameObjects->ptrToCameraUE4ActorCamera->m_quaternion;

            if (speed < step)
            {
                speed = step;
            }
            if (GetAsyncKeyState(VK_RBUTTON))
            {
                // The important thing to note with the fly hack is that the orientation
                // is stored in a Quaternion. There's a formula
                directionFromQuat = ForwardVec3FromQuat(quat);
                directionFromQuat = directionFromQuat * speed;
                speed += step * 2;
                char buf[256];
                sprintf_s(buf, 255, "Current poistion:%f %f %f %f", directionFromQuat.x, directionFromQuat.y, directionFromQuat.z, speed);
                printf("%s\n", buf);

                // Update the actual position of the character
                location->x += directionFromQuat.x;
                location->y += directionFromQuat.y;
                location->z += directionFromQuat.z;
            }

            speed -= step;
        }

    }

    fclose(fp);
    FreeConsole();
    // This removes the injected DLL and terminates this thread (the one running MainThread).
    // This needs to be the last thing here otherwise odd crashes tend to happen.
    FreeLibraryAndExitThread(hModule, 0);

    return 0;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);
        HANDLE hHandle = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr);
        CloseHandle(hHandle);
        break;
	}
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

