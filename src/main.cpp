#include "main.hpp"

#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/LevelFilteringNavigationController.hpp"
#include "GlobalNamespace/AnnotatedBeatmapLevelCollectionsViewController.hpp"
#include "GlobalNamespace/LevelSearchViewController.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapLevelPack.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "custom-types/shared/register.hpp"

#include "TwitchIRC/TwitchIRCClient.hpp"
#include "TwitchIRC/IRCSocket.hpp"

#include "bsml/shared/BSML/MainThreadScheduler.hpp"

#include "HMUI/ViewController.hpp"
#include "HMUI/Touchable.hpp"
#include "bsml/shared/BSML.hpp"

#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"

#include "CustomTypes/ChatHandler.hpp"
#include "Chat/ChatBuilder.hpp"
#include "logging.hpp"
#include "ModConfig.hpp"
#include "ModSettingsViewController.hpp"

#include <map>
#include <thread>
#include <iomanip>
#include <sstream>
#include <chrono>



std::unordered_set<std::string> Blacklist;

std::map<std::string, std::string> usersColorCache;

bool threadRunning = false;

using namespace BSML;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace TMPro;

template <typename T>
inline std::string int_to_hex(T val, size_t width=sizeof(T)*2) {
    std::stringstream ss;
    ss << "#" << std::setfill('0') << std::setw(width) << std::hex << (val|0) << "ff";
    return ss.str();
}


void AddChatObject(std::string text) {
    ChatObject chatObject = {};
    chatObject.Text = text;
    chatObject.GameObject = nullptr;
    if(chatHandler)
        chatHandler->AddChatObject(chatObject);
}



void OnChatMessage(IRCMessage ircMessage, TwitchIRCClient* client) {
    std::string username = ircMessage.prefix.nick;
    std::string message = ircMessage.parameters.at(ircMessage.parameters.size() - 1);
    if (Blacklist.count(username)) {
        INFO("Twitch Chat: Blacklisted user {} sent the message: {}", username, message);
        return;
    } else {
        INFO("Twitch Chat: User {} sent the message: {}", username, message);
    }
    if (usersColorCache.find(username) == usersColorCache.end())
        usersColorCache.emplace(username, int_to_hex(rand() % 0x1000000, 6));
    std::string text = "<color=" + usersColorCache[username] + ">" + username +
                       "</color>: " + message;
    AddChatObject(text);
}

#define JOIN_RETRY_DELAY 3000
#define CONNECT_RETRY_DELAY 15000




void TwitchIRCThread() {
    if(threadRunning) 
        return;
    threadRunning = true;
    INFO("Thread Started!");
    TwitchIRCClient client = TwitchIRCClient();
    std::string currentChannel = "";
    using namespace std::chrono;
    milliseconds lastJoinTry = 0ms;
    milliseconds lastConnectTry = 0ms;
    bool wasConnected = false;
    while(threadRunning) {
        auto currentTime = duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch());
        if(client.Connected()) {
            std::string targetChannel = getModConfig().Channel.GetValue();
            if(currentChannel != targetChannel) {
                if ((currentTime - lastJoinTry).count() > JOIN_RETRY_DELAY) {
                    lastJoinTry = currentTime; 
                    if(client.JoinChannel(targetChannel)) {
                        currentChannel = targetChannel;
                        INFO("Twitch Chat: Joined Channel {}!", currentChannel);
                        AddChatObject("<color=#9D9DA8>Connected to the channel</color> <color=#0008FF>[TWITCH] " + currentChannel + "</color>");
                    }
                }
            }
            client.ReceiveData();
        } else {
            if(wasConnected) {
                wasConnected = false;
                INFO("Twitch Chat: Disconnected!");
                AddChatObject("<color=#FF0000FF>Disconnected!</color>");
            }
            if ((currentTime - lastConnectTry).count() > CONNECT_RETRY_DELAY) {
                INFO("Twitch Chat: Connecting...");
                lastConnectTry = currentTime;
                if (client.InitSocket()) {
                    if (client.Connect()) {
                        if (client.Login("justinfan" + std::to_string(1030307 + rand() % 1030307), "xxx")) {
                            wasConnected = true;
                            AddChatObject("<color=#9D9DA8>Established connection to <color=#0008FF>Twitch</color>");
                            INFO("Twitch Chat: Logged In!");
                            client.HookIRCCommand("PRIVMSG", OnChatMessage);
                            currentChannel = "";
                        }
                    }
                }
            }
        }
        std::this_thread::yield();
    }
    if(wasConnected) {
        wasConnected = false;
        INFO("Twitch Chat: Disconnected!");
        AddChatObject("<color=#9D9DA8>Disconnected from <color=#0008FF>Twitch!</color>");
    }
    threadRunning = false;
    client.Disconnect();
    INFO("Thread Stopped!");
}

//MAKE_HOOK_MATCH(LevelSelectionNavigationControllerDidActivateSongRequestButton, &GlobalNamespace::LevelSelectionNavigationController::DidActivate, void, GlobalNamespace::LevelSelectionNavigationController *self, bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling)
//{
//    LevelSelectionNavigationControllerDidActivateSongRequestButton(self, firstActivation, addedToHierarchy, screenSystemEnabling);
//
//    if (firstActivation)
//    {
//        Button* button = BSML::Lite::CreateUIButton(self->get_transform(), "", "Chat Requests", nullptr);
//        UnityEngine::Object::DestroyImmediate(button->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>());
//        UnityEngine::UI::LayoutElement* layoutElement = button->get_gameObject()->GetComponent<UnityEngine::UI::LayoutElement*>();
//        if(!layoutElement)
//            layoutElement = button->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
//        button->get_transform()->set_localScale({2.0f, 3.0f, 2.0f});
//        button->set_interactable(false);
//
//        return;
//    }
//
//}

MAKE_HOOK_MATCH(SceneManager_Internal_ActiveSceneChanged,
                &UnityEngine::SceneManagement::SceneManager::Internal_ActiveSceneChanged,
                void, UnityEngine::SceneManagement::Scene prevScene, UnityEngine::SceneManagement::Scene nextScene) {
    SceneManager_Internal_ActiveSceneChanged(prevScene, nextScene);
    if(nextScene.IsValid()) {
        std::string sceneName = nextScene.get_name();
        if(sceneName.find("Menu") != std::string::npos) {
            BSML::MainThreadScheduler::Schedule(
                [] {
                    if(!chatHandler)
                        CreateChatGameObject();
                    if (!threadRunning)
                        std::thread (TwitchIRCThread).detach();
                }
            );
        }
    }
}


#pragma region Mod setup
/// @brief Called at the early stages of game loading
/// @param info
/// @return
MOD_EXPORT_FUNC void setup(CModInfo& info) {
    info.id = MOD_ID;
    info.version = VERSION;
    info.version_long = 0;
    modInfo.assign(info);

    Blacklist.insert("streamelements");
    Blacklist.insert("nightbot");

    getModConfig().Init(modInfo);
    INFO("Completed setup!");
}

/// @brief Called later on in the game loading - a good time to install function hooks
/// @return
MOD_EXPORT_FUNC void late_load() {
    il2cpp_functions::Init();
    custom_types::Register::AutoRegister();
    BSML::Init();

    BSML::Register::RegisterMainMenu("ChatUI", "ChatUI", "TODO Hover hint", DidActivate);

    INFO("Connecting to the Twitch EventSub WS");
    RunTwitchClient();

    //INFO("Starting ChatUI Webserver..");
    //WebServer::start();


    INFO("Installing hooks...");
    INSTALL_HOOK(Logger, SceneManager_Internal_ActiveSceneChanged);
    INSTALL_HOOK(Logger, LevelSelectionNavigationControllerDidActivateSongRequestButton);
    INFO("Installed all hooks!");
}
#pragma endregion