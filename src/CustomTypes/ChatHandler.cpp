#include <sstream>
#include <string>
#include <map>
#include <chrono>

#include "beatsaber-hook/shared/utils/utils.h"

#include "HMUI/ViewController.hpp"
#include "HMUI/Touchable.hpp"
#include "bsml/shared/BSML.hpp"

#include "UnityEngine/Rect.hpp"
#include "UnityEngine/SceneManagement/SceneManager.hpp"
#include "UnityEngine/SceneManagement/Scene.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"

#include "CustomTypes/ChatHandler.hpp"

#include "logging.hpp"

#include "ModConfig.hpp"

DEFINE_TYPE(ChatUI, ChatHandler);

using namespace BSML;
using namespace UnityEngine;
using namespace UnityEngine::SceneManagement;
using namespace UnityEngine::UI;
using namespace TMPro;

void ChatUI::ChatHandler::Update() {
    if(!LayoutTransform || !Canvas) return;

    Scene activeScene = SceneManager::GetActiveScene();
    if(activeScene.IsValid()){
        std::string sceneName = activeScene.get_name();
        auto position = getModConfig().PositionMenu.GetValue();
        auto rotation = getModConfig().RotationMenu.GetValue();
        auto size = getModConfig().SizeMenu.GetValue();
        if(sceneName == "GameCore" || getModConfig().ForceGame.GetValue()) {
            position = getModConfig().PositionGame.GetValue();
            rotation = getModConfig().RotationGame.GetValue();
            size = getModConfig().SizeGame.GetValue();
        }
        static bool _posSaveInit = false;
        static UnityEngine::Vector3 lastMenuPos;
        static UnityEngine::Vector3 lastGamePos;
        static std::chrono::steady_clock::time_point lastMenuSaveTime;
        static std::chrono::steady_clock::time_point lastGameSaveTime;
        if(!_posSaveInit) {
            lastMenuPos = getModConfig().PositionMenu.GetValue();
            lastGamePos = getModConfig().PositionGame.GetValue();
            lastMenuSaveTime = std::chrono::steady_clock::now() - std::chrono::seconds(10);
            lastGameSaveTime = std::chrono::steady_clock::now() - std::chrono::seconds(10);
            _posSaveInit = true;
        }
        bool usingGame = (sceneName == "GameCore" || getModConfig().ForceGame.GetValue());
        auto now = std::chrono::steady_clock::now();
        const float posEpsilon = 0.001f;
        try {
            if(!usingGame) {
                if (std::abs(position.x - lastMenuPos.x) > posEpsilon || std::abs(position.y - lastMenuPos.y) > posEpsilon || std::abs(position.z - lastMenuPos.z) > posEpsilon) {
                    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastMenuSaveTime).count() > 1000) {
                        lastMenuPos = position;
                        lastMenuSaveTime = now;
                        getModConfig().PositionMenu.SetValue(position);
                        getModConfig().Save();
                        INFO("ChatUI: Saved PositionMenu to config");
                    }
                }
            } else {
                if (std::abs(position.x - lastGamePos.x) > posEpsilon || std::abs(position.y - lastGamePos.y) > posEpsilon || std::abs(position.z - lastGamePos.z) > posEpsilon) {
                    if(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastGameSaveTime).count() > 1000) {
                        lastGamePos = position;
                        lastGameSaveTime = now;
                        getModConfig().PositionGame.SetValue(position);
                        getModConfig().Save();
                        INFO("ChatUI: Saved PositionGame to config");
                    }
                }
            }
        } catch(...) {
            INFO("ChatUI: Failed to save position to config");
        }
        SetPosition(position);
        SetRotation(rotation);
        SetSize(size);
    }

    std::lock_guard<std::mutex> guard(chatObjectsMutex);
    for (auto it = chatObjects.begin(); it != chatObjects.end(); it++) {
        ChatObject& object = *it;
        if(object.GameObject) {
            RectTransform* transform = object.GameObject->GetComponent<RectTransform*>();
            if((transform->get_localPosition().y - transform->get_rect().get_yMax()) > Canvas->GetComponent<RectTransform*>()->get_sizeDelta().y){
                Object::Destroy(object.GameObject);
                it = chatObjects.erase(it--);
            }
        } else {
            TextMeshProUGUI* text = BSML::Lite::CreateText(LayoutTransform, object.Text);
            Canvas->GetComponent<RectTransform*>()->set_sizeDelta(_textRsize);
            text->set_enableWordWrapping(true);
            text->set_overflowMode(TextOverflowModes::Overflow);
            text->set_fontSize(3.2f);
            text->set_alignment(TextAlignmentOptions::MidlineLeft);
            text->set_margin(UnityEngine::Vector4(1.0f, 0.0f, 0.0f, 0.0f));
            object.GameObject = text->get_gameObject();
        }
    }
}

void ChatUI::ChatHandler::Finalize() {
    chatObjects.~vector();
}

void ChatUI::ChatHandler::SetPosition(UnityEngine::Vector3 position) {
    if(Canvas)
        Canvas->GetComponent<RectTransform*>()->set_position(position);
}

void ChatUI::ChatHandler::SetRotation(UnityEngine::Vector3 rotation) {
    if(Canvas)
        Canvas->GetComponent<RectTransform*>()->set_eulerAngles(rotation);
}

void ChatUI::ChatHandler::SetSize(UnityEngine::Vector2 size) {
    if(Canvas) {
        _textRsize = UnityEngine::Vector2(size);
        Canvas->GetComponent<RectTransform*>()->set_sizeDelta(_textRsize);
    }
}

void ChatUI::ChatHandler::AddChatObject(ChatObject object) {
    std::lock_guard<std::mutex> guard(chatObjectsMutex);
    chatObjects.push_back(object);
}
