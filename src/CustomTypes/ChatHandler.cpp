#include <sstream>
#include <string>
#include <map>

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
    if(!LayoutTransform && !Canvas) return;

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
            // Temporary word wrapping false
            text->set_enableWordWrapping(false);
            text->set_fontSize(3.0f);
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
    if(Canvas)
        _textRsize = UnityEngine::Vector2(size);
        Canvas->GetComponent<RectTransform*>()->set_sizeDelta(_textRsize);
}

void ChatUI::ChatHandler::AddChatObject(ChatObject object) {
    std::lock_guard<std::mutex> guard(chatObjectsMutex);
    chatObjects.push_back(object);
}