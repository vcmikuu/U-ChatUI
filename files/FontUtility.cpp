



#include "FontUtility.hpp"
#include "custom-types/shared/register.hpp"
#include "UnityEngine/GameObject.hpp"

#include "logging.hpp"

#include "TMPro/TMP_FontAsset.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/Font.hpp"

void SetInternalFont(UnityEngine::GameObject* textObject) {
    // Like why should I know what the name of the Font is
    TMPro::TMP_FontAsset* tekoFont = UnityEngine::Resources::FindObjectsOfTypeAll<TMPro::TMP_FontAsset*>()->FirstOrDefault( //Like damn
        [](TMPro::TMP_FontAsset* font) {
            return font->get_name() == "Teko-Light";

        }
    );

    if (tekoFont != nullptr) {
        TMPro::TextMeshProUGUI* textMesh = textObject->GetComponent<TMPro::TextMeshProUGUI*>();
        if (textMesh != nullptr) {
            textMesh->set_font(tekoFont);
        }
    } else {
        // i made it because i hate beat saber
        INFO("i hate beat saber");
    }

    
}