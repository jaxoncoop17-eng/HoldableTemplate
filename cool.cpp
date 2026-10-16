#pragma once
#include <XRInput.hpp>
#include <BNMResolve.hpp>
//#include "../NotiLib/NotiLib.hpp"
#include "Settings.hpp"
#include "../PhotonResolve.hpp"
#include "ButtonInfo.hpp"
#include "Buttons.hpp"

using namespace BNM;
using namespace BNM::IL2CPP;
using namespace BNM::Structures;

class MenuManager {
private:
public:

    // --- DOCKING STATE VARIABLES ---
    static inline bool wasDocked = false;
    static inline Vector3 dockPosOffset = Vector3::zero;
    static inline Quaternion dockRotation = Quaternion::identity;

    // Tracking the last undocked position so RecreateMenu() doesn't mess up the offset calculation
    static inline Vector3 lastMenuPos = Vector3::zero;
    static inline Quaternion lastMenuRot = Quaternion::identity;

    struct ButtonCollider : BNM::UnityEngine::MonoBehaviour //im lazy tbh so ima just make this stay here lmao
    {
        BNM_CustomClass(ButtonCollider,
                        BNM::CompileTimeClassBuilder(BNM_OBFUSCATE_TMP(O("")), BNM_OBFUSCATE_TMP(O("ButtonCollider"))).Build(),
                        BNM::CompileTimeClassBuilder(BNM_OBFUSCATE_TMP(O("UnityEngine")), BNM_OBFUSCATE_TMP(O("MonoBehaviour")), BNM_OBFUSCATE_TMP(O("UnityEngine.CoreModule"))).Build(),
                        {},
        );

        void Constructor() {
            BNM::UnityEngine::MonoBehaviour tmp = *this;
            *this = ButtonCollider();
            *((BNM::UnityEngine::MonoBehaviour *)this) = tmp;
        }

        Mono::String* relatedText;
        void OnTriggerStay(Collider* collider) {
            bool rt;
            if (Settings::onehand) {
                rt = (!Settings::rightHanded && XRInput::GetBoolFeature(BoolFeature::TriggerButton, Controller::Left)) || (Settings::rightHanded && XRInput::GetBoolFeature(BoolFeature::TriggerButton, Controller::Right));
            } else if (!Settings::onehand) {
                rt = (!Settings::rightHanded && XRInput::GetBoolFeature(BoolFeature::TriggerButton, Controller::Right)) || (Settings::rightHanded && XRInput::GetBoolFeature(BoolFeature::TriggerButton, Controller::Left));            }

            if (rt) {
                if (collider->GetGameObject() != Settings::reference) return;
                if (Time::GetFrameCount() >= Settings::framePressCooldown + 20) {
                    MenuManager::Toggle(relatedText);
                    Settings::framePressCooldown = Time::GetFrameCount();
                }
            }
            if (!rt) {
                if (collider->GetGameObject() != Settings::reference) {

                }
            }
        }
        BNM_CustomField(relatedText, BNM::Defaults::Get<Mono::String*>(), "relatedText");
        BNM_CustomMethod(OnTriggerStay, false, BNM::Defaults::Get<void>(), "OnTriggerStay");
        BNM_CustomMethod(Constructor, false, BNM::Defaults::Get<void>(), ".ctor");
    };
    static void UpdateCursorPosition() {
        if (Settings::menu == nullptr || Settings::reference == nullptr) return;

        const char* handName = nullptr;

        if (Settings::onehand) {
            handName = Settings::rightHanded ? "RightHand Controller" : "LeftHand Controller";
        } else {
            handName = Settings::rightHanded ? "LeftHand Controller" : "RightHand Controller";
        }

        GameObject* handObj = GameObject::Find(handName);
        if (!handObj) return;

        Transform* handTrans = handObj->GetTransform();
        Transform* menuTrans = Settings::menu->GetTransform();

        Vector3 handPos = handTrans->GetPosition();
        Vector3 handForward = handTrans->GetForward();

        Vector3 menuCenter = menuTrans->GetPosition();
        Vector3 menuNormal = menuTrans->GetRight();

        float dot = Vector3::Dot(menuNormal, handForward);

        if (std::abs(dot) < 0.001f) return;

        float distance = Vector3::Dot(menuNormal, menuCenter - handPos) / dot;

        if (distance < 0.0f) {
            handForward = handForward * -1.0f;
            dot = Vector3::Dot(menuNormal, handForward);
            distance = Vector3::Dot(menuNormal, menuCenter - handPos) / dot;
        }

        if (distance >= 0.0f && distance < 2.0f) {
            Vector3 hitPoint = handPos + (handForward * distance);

            Vector3 surfaceOffset = menuNormal * 0.058f;

            Settings::reference->GetTransform()->SetPosition(hitPoint + surfaceOffset);
            Settings::reference->GetTransform()->SetRotation(menuTrans->GetRotation());

            Vector3 textoffset = menuNormal * 0.01f;

            Settings::cursorRect->SetPosition(Settings::reference->GetTransform()->GetPosition() + textoffset);
        }
    }
    static void Prefix() {
        try {
            bool open = (!Settings::rightHanded && XRInput::GetBoolFeature(BoolFeature::SecondaryButton, Controller::Left)) || (Settings::rightHanded && XRInput::GetBoolFeature(BoolFeature::SecondaryButton, Controller::Right));

            if (Settings::menu == nullptr) {
                if (open) {
                    CreateMenu();
                    RecenterMenu(Settings::rightHanded);
                    if (Settings::reference == nullptr) CreateReference(Settings::rightHanded);
                }
            } else {
                if (open) {
                    RecenterMenu(Settings::rightHanded);
                    UpdateCursorPosition();
                } else {
                    if (Settings::menu) {
                        GameObject::Destroy(Settings::menu);
                        Settings::menu = nullptr;
                    }
                    if (Settings::reference) {
                        GameObject::Destroy(Settings::reference);
                        Settings::reference = nullptr;
                    }
                }
            }
            for (auto& buttonList : Buttons::buttons) {
                for (auto& button : buttonList) {
                    if (button.enabled && button.method != nullptr) {
                        try { button.method(); } catch (...) {}
                    }
                }
            }
        } catch (Exception exc) {}
    }

    static void CreateMenu() {
        // menu holder
        Settings::menu = GameObject::CreatePrimitive(PrimitiveType::Cube);
        GameObject::Destroy(Settings::menu->GetComponent(Rigidbody::GetType()));
        GameObject::Destroy(Settings::menu->GetComponent(BoxCollider::GetType()));
        GameObject::Destroy(Settings::menu->GetComponent(Renderer::GetType()));
        Settings::menu->GetTransform()->SetLocalScale(Vector3(0.1f, 0.3f, 0.3825f));

        // Background
        Settings::menuBackground = GameObject::CreatePrimitive(PrimitiveType::Cube);
        GameObject::Destroy(Settings::menuBackground->GetComponent(Rigidbody::GetType()));
        GameObject::Destroy(Settings::menuBackground->GetComponent(BoxCollider::GetType()));
        Settings::menuBackground->GetTransform()->SetParent(Settings::menu->GetTransform());
        Settings::menuBackground->GetTransform()->SetRotation(Quaternion::identity);
        Settings::menuBackground->GetTransform()->SetLocalScale(Settings::menuSize);
        Settings::menuBackground->GetTransform()->SetPosition(Vector3(0.05f, 0.f, 0.f));

        Renderer* bgRenderer = (Renderer*)Settings::menuBackground->GetComponent(Renderer::GetType());
        fixShader(bgRenderer);
        bgRenderer->GetMaterial()->SetColor(Settings::backgroundColor);

        CreateRoundedObject(Settings::menuBackground);

        // Upper Tab
        Settings::menuUpperTab = GameObject::CreatePrimitive(PrimitiveType::Cube);
        GameObject::Destroy(Settings::menuUpperTab->GetComponent(Rigidbody::GetType()));
        GameObject::Destroy(Settings::menuUpperTab->GetComponent(BoxCollider::GetType()));
        Settings::menuUpperTab->GetTransform()->SetParent(Settings::menu->GetTransform());
        Settings::menuUpperTab->GetTransform()->SetRotation(Quaternion::identity);
        Settings::menuUpperTab->GetTransform()->SetLocalScale(Vector3(0.1f, 1.5f, 0.07f));
        Settings::menuUpperTab->GetTransform()->SetPosition(Vector3(0.052f, 0.f, 0.157f));

        Renderer* tr = (Renderer*)Settings::menuUpperTab->GetComponent(Renderer::GetType());
        fixShader(tr);
        tr->GetMaterial()->SetColor(Settings::uppertabcolor);

        CreateRoundedObject(Settings::menuUpperTab);


        // Canvas
        Settings::canvasObject = (GameObject*)GameObject::GetClass().CreateNewObjectParameters();
        Settings::canvasObject->GetTransform()->SetParent(Settings::menu->GetTransform());
        Canvas* canvas = (Canvas*)Settings::canvasObject->AddComponent(Canvas::GetType());
        CanvasScaler* scaler = (CanvasScaler*)Settings::canvasObject->AddComponent(CanvasScaler::GetType());
        Settings::canvasObject->AddComponent(GraphicRaycaster::GetType());
        canvas->SetRenderMode(RenderMode::WorldSpace);
        scaler->SetDynamicPixelsPerUnit(4000); // was 1000

        // Title
        GameObject* titleObj = (GameObject*)GameObject::GetClass().CreateNewObjectParameters();
        titleObj->GetTransform()->SetParent(Settings::canvasObject->GetTransform());
        Settings::titleText = (Text*)titleObj->AddComponent(Text::GetType());

        Settings::titleText->SetFont(Settings::currentFont);
        Settings::titleText->SetText(" ▲ PIGEON.WTF ▲         ");
        Settings::titleText->SetFontSize(1);
        Settings::titleText->SetColor(Settings::textColors[0]);
        Settings::titleText->SetColor(Settings::textColors[0]);
        Settings::titleText->SetSupportRichText(true);
        Settings::titleText->SetFontStyle(FontStyle::Bold);
        Settings::titleText->SetAlignment(TextAnchor::MiddleLeft);
        Settings::titleText->SetTextResizeForBestFit(true);
        Settings::titleText->SetResizeTextMinSize(0);


        RectTransform* titleRect = (RectTransform*)Settings::titleText->GetComponent(RectTransform::GetType());
        titleRect->SetLocalPosition(Vector3::zero);
        titleRect->SetSizeDelta(Vector2(0.2f, 0.03f));
        titleRect->SetPosition(Vector3(0.062f, 0.123f, 0.157f));
        titleRect->SetRotation(Quaternion::FromEuler(Vector3(180.f, 90.f, 90.f)));

        // cursor

        GameObject* cursor = (GameObject*)GameObject::GetClass().CreateNewObjectParameters();
        cursor->GetTransform()->SetParent(Settings::canvasObject->GetTransform());
        Settings::Cursor = (Text*)cursor->AddComponent(Text::GetType());

        Settings::Cursor->SetFont(Settings::currentFont);
        Settings::Cursor->SetText("⌖");
        Settings::Cursor->SetFontSize(1);
        Settings::Cursor->SetColor(Settings::textColors[0]);
        Settings::Cursor->SetSupportRichText(true);
        Settings::Cursor->SetFontStyle(FontStyle::Bold);
        Settings::Cursor->SetAlignment(TextAnchor::MiddleCenter);
        Settings::Cursor->SetTextResizeForBestFit(true);
        Settings::Cursor->SetResizeTextMinSize(0);


        Settings::cursorRect = (RectTransform*)Settings::Cursor->GetComponent(RectTransform::GetType());
        Settings::cursorRect->SetLocalPosition(Vector3::zero);
        Settings::cursorRect->SetSizeDelta(Vector2(0.2f, 0.03f));
        Settings::cursorRect->SetRotation(Quaternion::FromEuler(Vector3(180.f, 90.f, 90.f)));


        // button stuff

        std::vector<ButtonInfo> activeButtons(
                Buttons::buttons[Settings::currentCategory()].begin() + std::min((int)Buttons::buttons[Settings::currentCategory()].size(), Settings::pageNumber * Settings::buttonsPerPage),
                Buttons::buttons[Settings::currentCategory()].begin() + std::min((int)Buttons::buttons[Settings::currentCategory()].size(), Settings::pageNumber * Settings::buttonsPerPage + Settings::buttonsPerPage)
        );
        for (int i = 0; i < activeButtons.size(); i++)
        {
            CreateButton(i * 0.07f, activeButtons[i]);
        }
        for (int i = 0; i < Buttons::buttons[0].size(); i++)
        {
            CreateTab(i * 0.17f, Buttons::buttons[0][i]);
        }
    }

    static void fixShader(Renderer* rend) {
        // auto shader = Shader::Find("GUI/Text Shader");
        auto shader = Shader::Find("Unlit/Color");
        if (shader != nullptr)
        {
            rend->GetMaterial()->SetShader(shader);
        }
    }

    static void CreateRoundedObject(GameObject* targetObj)
    {
        float radius = Settings::rounding;

        auto* targetRenderer = reinterpret_cast<Renderer*>(
                targetObj->GetComponent(Renderer::GetType())
        );

        Transform* targetTransform = targetObj->GetTransform();
        Vector3 basePos = targetTransform->GetLocalPosition();
        Vector3 baseScale = targetTransform->GetLocalScale();

        auto setupPrimitive = [&](GameObject* obj)
        {
            auto* rend = reinterpret_cast<Renderer*>(obj->GetComponent(Renderer::GetType()));
            rend->SetEnabled(targetRenderer->GetEnabled());

            GameObject::Destroy(obj->GetComponent(Collider::GetType()));

            obj->GetTransform()->SetParent(Settings::menu->GetTransform());
            obj->GetTransform()->SetLocalRotation(Quaternion::identity);
        };


        GameObject* coreY = GameObject::CreatePrimitive(PrimitiveType::Cube);
        setupPrimitive(coreY);
        coreY->GetTransform()->SetLocalPosition(basePos);
        coreY->GetTransform()->SetLocalScale(
                baseScale + Vector3(0.0f, radius * -2.55f, 0.0f)
        );


        GameObject* coreZ = GameObject::CreatePrimitive(PrimitiveType::Cube);
        setupPrimitive(coreZ);
        coreZ->GetTransform()->SetLocalPosition(basePos);
        coreZ->GetTransform()->SetLocalScale(
                baseScale + Vector3(0.0f, 0.0f, -radius * 2.0f)
        );

        auto createCorner = [&](Vector3 offset, bool top, bool front)
        {
            GameObject* corner = GameObject::CreatePrimitive(PrimitiveType::Cylinder);
            setupPrimitive(corner);

            float yDir = top ? 1.0f : -1.0f;
            float zDir = front ? 1.0f : -1.0f;

            corner->GetTransform()->SetLocalRotation(
                    Quaternion::identity * Quaternion::FromEuler(0.0f, 0.0f, 90.0f)
            );

            corner->GetTransform()->SetLocalPosition(
                    basePos + Vector3(
                            0.0f,
                            yDir * (baseScale.y / 2.0f - radius * 1.275f),
                            zDir * (baseScale.z / 2.0f - radius)
                    )
            );

            corner->GetTransform()->SetLocalScale(
                    Vector3(
                            radius * 2.55f,
                            baseScale.x / 2.0f,
                            radius * 2.0f
                    )
            );

            return corner;
        };

        GameObject* cornerTF = createCorner(basePos, true, true);
        GameObject* cornerBF = createCorner(basePos, false, true);
        GameObject* cornerTB = createCorner(basePos, true, false);
        GameObject* cornerBB = createCorner(basePos, false, false);

        std::vector<GameObject*> generatedParts = {
                coreY, coreZ, cornerTF, cornerBF, cornerTB, cornerBB
        };

        for (auto* part : generatedParts)
        {
            Renderer* rend = reinterpret_cast<Renderer*>(part->GetComponent(Renderer::GetType()));
            fixShader(rend);

            rend->GetMaterial()->SetColor(targetRenderer->GetMaterial()->GetColor());
        }

        targetRenderer->SetEnabled(false);
    }

    static void CreateButton(float offset, ButtonInfo method) {
        GameObject* buttonObj = GameObject::CreatePrimitive(PrimitiveType::Cube);
        GameObject::Destroy(buttonObj->GetComponent(Rigidbody::GetType()));
        BoxCollider* col = (BoxCollider *) buttonObj->GetComponent(BoxCollider::GetType());
        col->SetIsTrigger(true);
        buttonObj->GetTransform()->SetParent(Settings::menu->GetTransform());
        buttonObj->GetTransform()->SetRotation(Quaternion::identity);
        buttonObj->GetTransform()->SetLocalScale(Vector3(0.09f, 0.25f, 0.06f));
        buttonObj->GetTransform()->SetLocalPosition(Vector3(0.56f, 0.f, 0.255f - offset));
        Renderer* rend = (Renderer *) buttonObj->GetComponent(Renderer::GetType());
        fixShader(rend);
        rend->GetMaterial()->SetColor(method.enabled ? Settings::buttonColors[1] : Settings::buttonColors[0]);

        ButtonCollider* nbc = (ButtonCollider*)buttonObj->AddComponent(ButtonCollider::BNMCustomClass.type);
        nbc->relatedText = CreateMonoString(method.buttonText);

        GameObject* textObj = (GameObject*)GameObject::GetClass().CreateNewObjectParameters();
        textObj->GetTransform()->SetParent(Settings::canvasObject->GetTransform());
        Text* text = (Text*)textObj->AddComponent(Text::GetType());
        text->SetFont(Settings::currentFont);
        if (!method.overlapText.empty()) text->SetText(method.overlapText);
        else text->SetText(method.buttonText);
        text->SetSupportRichText(true);
        text->SetFontSize(1);
        text->SetColor(method.enabled ? Settings::textColors[1] : Settings::textColors[0]);
        text->SetAlignment(TextAnchor::MiddleCenter);
        text->SetFontStyle(FontStyle::Normal);
        text->SetTextResizeForBestFit(true);
        text->SetResizeTextMinSize(0);
        RectTransform* rect = (RectTransform*)text->GetComponent(RectTransform::GetType());
        rect->SetLocalPosition(Vector3::zero);
        rect->SetSizeDelta(Vector2(0.1f, 0.03f));
        rect->SetLocalPosition(Vector3(0.064f, 0.f, 0.098f - (offset / 2.6f)));
        rect->SetRotation(Quaternion::FromEuler(Vector3(180.f, 90.f, 90.f)));

        CreateRoundedObject(buttonObj);
    }

    static void CreateTab(float offset, ButtonInfo method) {
        GameObject* tabButtonObj = GameObject::CreatePrimitive(PrimitiveType::Cube);
        GameObject::Destroy(tabButtonObj->GetComponent(Rigidbody::GetType()));
        BoxCollider* col = (BoxCollider *) tabButtonObj->GetComponent(BoxCollider::GetType());
        col->SetIsTrigger(true);
        tabButtonObj->GetTransform()->SetParent(Settings::menu->GetTransform());
        tabButtonObj->GetTransform()->SetRotation(Quaternion::identity);
        tabButtonObj->GetTransform()->SetLocalScale(Vector3(0.09f, 0.15f, 0.06f));
        tabButtonObj->GetTransform()->SetLocalPosition(Vector3(0.56f, 0.62 - offset, 0.335f));
        Renderer* rend = (Renderer *) tabButtonObj->GetComponent(Renderer::GetType());
        fixShader(rend);
        rend->GetMaterial()->SetColor(method.enabled ? Settings::buttonColors[1] : Settings::buttonColors[0]);

        ButtonCollider* nbc = (ButtonCollider*)tabButtonObj->AddComponent(ButtonCollider::BNMCustomClass.type);
        nbc->relatedText = CreateMonoString(method.buttonText);

        GameObject* textObj = (GameObject*)GameObject::GetClass().CreateNewObjectParameters();
        textObj->GetTransform()->SetParent(Settings::canvasObject->GetTransform());
        Text* text = (Text*)textObj->AddComponent(Text::GetType());
        text->SetFont(Settings::currentFont);
        if (!method.overlapText.empty()) text->SetText(method.overlapText);
        else text->SetText(method.buttonText);
        text->SetSupportRichText(true);
        text->SetFontSize(1);
        text->SetColor(method.enabled ? Settings::textColors[1] : Settings::textColors[0]);
        text->SetAlignment(TextAnchor::MiddleCenter);
        text->SetFontStyle(FontStyle::Normal);
        text->SetTextResizeForBestFit(true);
        text->SetResizeTextMinSize(0);
        RectTransform* rect = (RectTransform*)text->GetComponent(RectTransform::GetType());
        rect->SetLocalPosition(Vector3::zero);
        rect->SetSizeDelta(Vector2(0.1f, 0.015f));
        rect->SetLocalPosition(Vector3(0.064f, 0.186f - (offset * 0.3f), 0.128f));
        rect->SetRotation(Quaternion::FromEuler(Vector3(180.f, 90.f, 90.f)));

        CreateRoundedObject(tabButtonObj);
    }

    static void RecreateMenu() {
        if (Settings::menu != nullptr) {
            GameObject::Destroy(Settings::menu);
            Settings::menu = nullptr;
        }
        CreateMenu();
        RecenterMenu(Settings::rightHanded);
    }
    static void RecenterMenu(bool isrightHanded) {
        if (Settings::dock) {
            GameObject* player = GameObject::Find("GorillaPlayer");
            if (player != nullptr) {
                if (!MenuManager::wasDocked) {
                    MenuManager::dockPosOffset = MenuManager::lastMenuPos - player->GetTransform()->GetPosition();
                    MenuManager::dockRotation = MenuManager::lastMenuRot;
                    MenuManager::wasDocked = true;
                }

                Settings::menu->GetTransform()->SetPosition(player->GetTransform()->GetPosition() + MenuManager::dockPosOffset);
                Settings::menu->GetTransform()->SetRotation(MenuManager::dockRotation);
            }
        } else {
            MenuManager::wasDocked = false;

            if (Settings::facecam) {
                if (Settings::difof) {
                    if (GameObject::Find("Main Camera") != nullptr) {
                        auto cam = GameObject::Find("Main Camera")->GetTransform();
                        auto menu = Settings::menu->GetTransform();
                        Vector3 camf = cam->GetForward();
                        float d = 0.3f;
                        Quaternion fix = Quaternion::FromEuler(-90.f, 90.f, 0.f);
                        auto camrot = cam->GetRotation();
                        menu->SetRotation(camrot * fix);
                        Vector3 calcpos = cam->GetPosition() + (camf * d);
                        calcpos.y -= 0.05f;
                        menu->SetPosition(calcpos);
                    }
                } else {
                    if (!isrightHanded) {
                        if (GameObject::Find("LeftHand Controller") != nullptr) {
                            Transform *hand = GameObject::Find("LeftHand Controller")->GetTransform();
                            Vector3 pos = hand->GetPosition();
                            pos.y += 0.15f;
                            Settings::menu->GetTransform()->SetPosition(pos);
                            auto cam = GameObject::Find("Main Camera")->GetTransform();
                            auto menu = Settings::menu->GetTransform();
                            Vector3 dir = cam->GetPosition() - menu->GetPosition();
                            Quaternion fix = Quaternion::FromEuler(-90, 90, 0);
                            auto camrot = cam->GetTransform()->GetRotation();
                            menu->SetRotation(camrot * fix);
                        }
                    }
                    if (isrightHanded) {
                        if (GameObject::Find("RightHand Controller") != nullptr) {
                            Transform *hand = GameObject::Find("RightHand Controller")->GetTransform();
                            Vector3 pos = hand->GetPosition();
                            pos.y += 0.15f;
                            Settings::menu->GetTransform()->SetPosition(pos);
                            auto cam = GameObject::Find("Main Camera")->GetTransform();
                            auto menu = Settings::menu->GetTransform();
                            Vector3 dir = cam->GetPosition() - menu->GetPosition();
                            Quaternion fix = Quaternion::FromEuler(-90, 90, 0);
                            auto camrot = cam->GetTransform()->GetRotation();
                            menu->SetRotation(camrot * fix);
                        }
                    }
                }
            } else {
                if (Settings::difof) {
                    if (GameObject::Find("Main Camera") != nullptr) {
                        auto cam = GameObject::Find("Main Camera")->GetTransform();
                        auto menu = Settings::menu->GetTransform();
                        Vector3 camf = cam->GetForward();
                        float d = 0.3f;
                        Quaternion fix = Quaternion::FromEuler(-90.f, 90.f, 0.f);
                        auto camrot = cam->GetRotation();
                        menu->SetRotation(camrot * fix);
                        Vector3 calcpos = cam->GetPosition() + (camf * d);
                        calcpos.y -= 0.05f;
                        menu->SetPosition(calcpos);
                    }
                } else {
                    if (!isrightHanded) {
                        if (GameObject::Find("LeftHand Controller") != nullptr) {
                            Transform *hand = GameObject::Find("LeftHand Controller")->GetTransform();
                            Vector3 up = hand->GetUp();
                            auto menu = Settings::menu->GetTransform();
                            auto cam = GameObject::Find("Main Camera")->GetTransform();
                            auto handPos = hand->GetPosition();
                            auto handUp = hand->GetUp();
                            menu->SetPosition(handPos + (handUp * 0.15f));
                            Quaternion fix = Quaternion::FromEuler(-90, 90, 0);
                            auto camrot = GameObject::Find("LeftHand Controller")->GetTransform()->GetRotation();
                            menu->SetRotation(camrot * fix);
                        }
                    }
                    if (isrightHanded) {
                        if (GameObject::Find("RightHand Controller") != nullptr) {
                            Transform *hand = GameObject::Find("RightHand Controller")->GetTransform();
                            Vector3 pos = hand->GetPosition();
                            auto cam = GameObject::Find("Main Camera")->GetTransform();
                            auto menu = Settings::menu->GetTransform();
                            auto handPos = hand->GetPosition();
                            auto handUp = hand->GetUp();
                            menu->SetPosition(handPos + (handUp * 0.15f));
                            Quaternion fix = Quaternion::FromEuler(-90, 90, 0);
                            auto camrot = hand->GetRotation();
                            menu->SetRotation(camrot * fix);
                        }
                    }
                }
            }

            if (Settings::menu != nullptr) {
                MenuManager::lastMenuPos = Settings::menu->GetTransform()->GetPosition();
                MenuManager::lastMenuRot = Settings::menu->GetTransform()->GetRotation();
            }
        }
    }

    static void CreateReference(bool isrightHanded) {
        Settings::reference = GameObject::CreatePrimitive(PrimitiveType::Sphere);
        Settings::reference->GetTransform()->SetParent(nullptr);

        Renderer* RefRend = (Renderer*)Settings::reference->GetComponent(Renderer::GetType());
        fixShader(RefRend);

        RefRend->GetMaterial()->SetColor(Color(1.f, 1.f, 1.f, 0.f));
        Settings::reference->GetTransform()->SetLocalScale(Vector3(0.001f, 0.001f, 0.001f));

        auto rb = (Rigidbody*)Settings::reference->AddComponent(Rigidbody::GetType());
        if (rb) { rb->SetUseGravity(false); rb->SetIsKinematic(true); }

        Settings::buttonCollider = (SphereCollider*)Settings::reference->GetComponent(SphereCollider::GetType());
    };

    static void Toggle(Mono::String* text) {
        if (!text) return;
        string buttonText = text->str();
        int lastPage = ((Buttons::buttons[Settings::currentCategory()].size() + Settings::buttonsPerPage - 1) / Settings::buttonsPerPage) - 1;
        if (buttonText == "PreviousPage") {
            Settings::pageNumber--;
            if (Settings::pageNumber < 0)
                Settings::pageNumber = lastPage;
        } else if (buttonText == "NextPage") {
            Settings::pageNumber++;
            if (Settings::pageNumber > lastPage)
                Settings::pageNumber = 0;
        } else if (buttonText == "DisconnectBTN") {
            PhotonNetwork::Disconnect();
            // NotiLib::SendNotification("<color=grey>[</color><color=yellow>DISCONNECT</color><color=grey>]</color> Disconnected from the server");
        } else {
            for (auto& buttonList : Buttons::buttons) {
                for (auto& Button : buttonList) {
                    if (Button.buttonText == buttonText) {
                        if (Button.isTogglable) {
                            Button.enabled = !Button.enabled;

                            if (Button.enabled) {
                                if (Button.enableMethod != nullptr) Button.enableMethod();
                            }
                            else
                            {
                                if (Button.disableMethod != nullptr) Button.disableMethod();
                            }
                        }
                        else
                        {
                            if (Button.method != nullptr) Button.method();
                        }

                        break;
                    }
                }
            }
        }

        RecreateMenu();
    }
};


