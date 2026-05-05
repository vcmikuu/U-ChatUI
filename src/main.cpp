#include "main.hpp"

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
#include "ChatBuilder.hpp"
#include "logging.hpp"
#include "ModConfig.hpp"
#include "ModSettingsViewController.hpp"

#include <map>
#include <thread>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <cctype>

std::unordered_set<std::string> Blacklist;

std::map<std::string, std::string> usersColorCache;

bool threadRunning = false;

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

static bool ShouldShowEventMessages() {
    return getModConfig().ShowEventMessages.GetValue();
}

static int ParseTierNumber(std::string const& plan) {
    if (plan == "Prime") return 1;
    if (plan == "1000") return 1;
    if (plan == "2000") return 2;
    if (plan == "3000") return 3;
    return 1;
}

void OnUserNotice(IRCMessage ircMessage, TwitchIRCClient* client) {
    (void)client;
    if (!ShouldShowEventMessages()) return;
    auto msgIdIt = ircMessage.tags.find("msg-id");
    if (msgIdIt == ircMessage.tags.end()) return;

    auto planIt = ircMessage.tags.find("msg-param-sub-plan");
    int tier = ParseTierNumber(planIt != ircMessage.tags.end() ? planIt->second : "1000");

    if (msgIdIt->second == "sub" || msgIdIt->second == "resub") {
        auto displayNameIt = ircMessage.tags.find("display-name");
        std::string username = displayNameIt != ircMessage.tags.end() && !displayNameIt->second.empty()
            ? displayNameIt->second
            : ircMessage.prefix.nick;
        AddChatObject(username + " subscribed with Tier " + std::to_string(tier));
        return;
    }

    if (msgIdIt->second == "subgift" || msgIdIt->second == "anonsubgift") {
        auto gifterIt = ircMessage.tags.find("display-name");
        auto recipientIt = ircMessage.tags.find("msg-param-recipient-display-name");
        std::string gifter = (gifterIt != ircMessage.tags.end() && !gifterIt->second.empty()) ? gifterIt->second : "Someone";
        std::string recipient = (recipientIt != ircMessage.tags.end() && !recipientIt->second.empty()) ? recipientIt->second : "someone";
        AddChatObject(gifter + " gifted " + recipient + " a Tier " + std::to_string(tier) + " Subscription");
        return;
    }
}

void OnBitsMessage(IRCMessage ircMessage, TwitchIRCClient* client) {
    (void)client;
    if (!ShouldShowEventMessages()) return;
    auto bitsIt = ircMessage.tags.find("bits");
    if (bitsIt == ircMessage.tags.end() || bitsIt->second.empty()) return;

    int bits = 0;
    try {
        bits = std::stoi(bitsIt->second);
    } catch (...) {
        return;
    }
    if (bits <= 0) return;

    auto displayNameIt = ircMessage.tags.find("display-name");
    std::string username = displayNameIt != ircMessage.tags.end() && !displayNameIt->second.empty()
        ? displayNameIt->second
        : ircMessage.prefix.nick;
    AddChatObject(username + " gave you " + std::to_string(bits) + " Bits!");
}

void OnFollowFallback(IRCMessage ircMessage, TwitchIRCClient* client) {
    (void)client;
    if (!ShouldShowEventMessages()) return;
    if (ircMessage.parameters.empty()) return;

    std::string message = ircMessage.parameters.back();
    auto markerPos = message.find(" is now following");
    if (markerPos == std::string::npos) return;

    std::string username = message.substr(0, markerPos);
    while (!username.empty() && std::isspace(static_cast<unsigned char>(username.back()))) username.pop_back();
    if (username.empty()) return;
    AddChatObject(username + " is now following you");
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
                        AddChatObject("<color=#FFFFFFFF>Joined Channel:</color> <color=#FFB300FF>" + currentChannel + "</color>");
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
                            AddChatObject("<color=#FFFFFFFF>Logged In!</color>");
                            INFO("Twitch Chat: Logged In!");
                            client.HookIRCCommand("PRIVMSG", OnChatMessage);
                            client.HookIRCCommand("PRIVMSG", OnBitsMessage);
                            client.HookIRCCommand("PRIVMSG", OnFollowFallback);
                            client.HookIRCCommand("USERNOTICE", OnUserNotice);
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
        AddChatObject("<color=#FF0000FF>Disconnected!</color>");
    }
    threadRunning = false;
    client.Disconnect();
    INFO("Thread Stopped!");
}

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

    BSML::Register::RegisterSettingsMenu("ChatUI", DidActivate, false);
    INFO("Installing hooks...");
    INSTALL_HOOK(Logger, SceneManager_Internal_ActiveSceneChanged);
    INFO("Installed all hooks!");
}
#pragma endregion
