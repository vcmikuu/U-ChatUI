#include "HMUI/ViewController.hpp"
#include "HMUI/Touchable.hpp"

#include "bsml/shared/BSML.hpp"

#include "UnityEngine/UI/ContentSizeFitter.hpp"
#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/UI/RectMask2D.hpp"

#include "HMUI/CurvedCanvasSettings.hpp"

#include "TMPro/TextMeshProUGUI.hpp"

#include "VRUIControls/VRGraphicRaycaster.hpp"

#include "bsml/shared/BSML/Components/Backgroundable.hpp"

#include "Chat/ChatBuilder.hpp"
#include "logging.hpp"

using namespace BSML;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace HMUI;
using namespace TMPro;

//BAD STUFF I KNOW
ChatUI::ChatHandler* chatHandler = nullptr;

void CreateChatGameObject() {
    if(chatHandler) 
        return;
    UnityEngine::GameObject* chatGameObject = BSML::Lite::CreateCanvas();
    Object::DestroyImmediate(chatGameObject->GetComponent<VRUIControls::VRGraphicRaycaster*>());
    Object::DontDestroyOnLoad(chatGameObject);
    
    chatHandler = chatGameObject->AddComponent<ChatUI::ChatHandler*>();
    chatGameObject->AddComponent<RectMask2D*>();
    auto backgroundable = chatGameObject->AddComponent<Backgroundable*>();
    backgroundable->ApplyBackground("panel");
    backgroundable->ApplyAlpha(0.50f);
    RectTransform* transform = chatGameObject->GetComponent<RectTransform*>();

    VerticalLayoutGroup* layout = BSML::Lite::CreateVerticalLayoutGroup(transform);
    ContentSizeFitter* contentSizeFitter = layout->GetComponent<ContentSizeFitter*>();
    contentSizeFitter->set_horizontalFit(ContentSizeFitter::FitMode::Unconstrained);
    contentSizeFitter->set_verticalFit(ContentSizeFitter::FitMode::PreferredSize);
    layout->set_childControlWidth(false);
    layout->set_childControlHeight(true);
    layout->set_childForceExpandWidth(true);
    layout->set_childForceExpandHeight(false);
    layout->set_childAlignment(TextAnchor::LowerLeft);
    GameObject* layoutGameObject = layout->get_gameObject();
    RectTransform* layoutTransform = layout->get_rectTransform();
    layoutTransform->set_pivot(UnityEngine::Vector2(0.0f, 0.0f));
    chatHandler->LayoutTransform = layoutTransform;
    chatHandler->Canvas = chatGameObject;
}