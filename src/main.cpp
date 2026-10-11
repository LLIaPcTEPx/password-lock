#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>

using namespace geode::prelude;

// ============================================================
//  Forward declarations
// ============================================================
class PasswordPopup;

// ============================================================
//  PasswordLock singleton
// ============================================================
class PasswordLock {
public:
    static PasswordLock* get() {
        static PasswordLock instance;
        return &instance;
    }

    bool isLocked() const { return m_locked; }

    void lock();
    void unlock();

private:
    bool m_locked = true;
    bool m_popupShown = false;
    PasswordLock() = default;
};

// ============================================================
//  Password Popup
// ============================================================
class PasswordPopup : public geode::Popup {
protected:
    std::string m_input = "";
    CCLabelBMFont* m_inputLabel = nullptr;
    CCLabelBMFont* m_errorLabel = nullptr;
    geode::comm::ListenerHandle m_keyListener;
    bool m_unlocked = false;

    bool init() override {
        // Компактный попап: 240 x 300
        if (!Popup::init(240.f, 300.f)) return false;

        this->setTitle("Enter Password");

        // Индикатор ввода (____)
        m_inputLabel = CCLabelBMFont::create("____", "bigFont.fnt");
        m_inputLabel->setScale(0.75f);
        m_inputLabel->setPosition({ m_size.width / 2.f, 260.f });
        m_mainLayer->addChild(m_inputLabel);

        // Метка ошибки
        m_errorLabel = CCLabelBMFont::create("", "bigFont.fnt");
        m_errorLabel->setScale(0.42f);
        m_errorLabel->setColor({ 255, 80, 80 });
        m_errorLabel->setPosition({ m_size.width / 2.f, 235.f });
        m_mainLayer->addChild(m_errorLabel);

        // Параметры сетки кнопок
        constexpr float btnSize = 42.f;
        constexpr float gap     = 10.f;
        constexpr float step    = btnSize + gap;

        // Центр первой кнопки по X (сетка центрируется в попапе)
        constexpr float startX  = (240.f - (3.f * btnSize + 2.f * gap)) / 2.f + btnSize / 2.f;
        // Центр первой строки по Y
        constexpr float startY  = 195.f;

        const char* labels[4][3] = {
            { "1", "2", "3" },
            { "4", "5", "6" },
            { "7", "8", "9" },
            { "x", "0", ">" }
        };

        auto menu = CCMenu::create();
        menu->setPosition({ 0.f, 0.f });
        m_mainLayer->addChild(menu);

        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 3; ++col) {
                const char* lbl = labels[row][col];

                ccColor3B color = { 200, 200, 200 };
                if (std::string(lbl) == "x") color = { 220, 60, 60 };
                if (std::string(lbl) == ">") color = { 60, 200, 60 };

                auto btnSprite = CCScale9Sprite::create("GJ_button_01.png");
                btnSprite->setColor(color);
                btnSprite->setContentSize({ btnSize, btnSize });

                auto label = CCLabelBMFont::create(lbl, "bigFont.fnt");
                label->setScale(0.6f);
                label->setPosition({ btnSize / 2.f, btnSize / 2.f });
                btnSprite->addChild(label);

                auto btn = CCMenuItemSpriteExtra::create(
                    btnSprite,
                    this,
                    menu_selector(PasswordPopup::onButton)
                );
                btn->setTag(row * 3 + col);

                btn->setPosition({
                    startX + col * step,
                    startY - row * step
                });
                menu->addChild(btn);
            }
        }

        // Ввод с клавиатуры по ASCII-кодам
        m_keyListener = KeyboardInputEvent().listen([this](KeyboardInputData& data) {
            if (data.action == KeyboardInputData::Action::Release)
                return geode::ListenerResult::Propagate;

            int k = static_cast<int>(data.key);

            // Цифры 0-9 (ASCII 48..57)
            if (k >= 48 && k <= 57) {
                if (m_input.size() < 4) {
                    m_input += std::to_string(k - 48);
                    updateInputLabel();
                    m_errorLabel->setString("");
                }
                return geode::ListenerResult::Stop;
            }

            // Backspace (ASCII 8)
            if (k == 8) {
                if (!m_input.empty()) {
                    m_input.pop_back();
                    updateInputLabel();
                    m_errorLabel->setString("");
                }
                return geode::ListenerResult::Stop;
            }

            // Enter (ASCII 13)
            if (k == 13) {
                checkPassword();
                return geode::ListenerResult::Stop;
            }

            return geode::ListenerResult::Propagate;
        });

        return true;
    }

    void updateInputLabel() {
        std::string display = m_input;
        while (display.size() < 4) display += "_";
        m_inputLabel->setString(display.c_str());
    }

    void checkPassword() {
        auto correct = Mod::get()->getSettingValue<std::string>("password");
        if (m_input == correct) {
            m_unlocked = true;
            PasswordLock::get()->unlock();
            onClose(nullptr);
        } else {
            m_errorLabel->setString("Invalid password!!!");
            m_input.clear();
            updateInputLabel();
        }
    }

    void onButton(CCObject* sender) {
        auto btn = static_cast<CCMenuItemSpriteExtra*>(sender);
        int tag = btn->getTag();

        const char* labels[4][3] = {
            { "1", "2", "3" },
            { "4", "5", "6" },
            { "7", "8", "9" },
            { "x", "0", ">" }
        };
        std::string value = labels[tag / 3][tag % 3];

        if (value == "x") {
            if (!m_input.empty()) {
                m_input.pop_back();
                updateInputLabel();
                m_errorLabel->setString("");
            }
        }
        else if (value == ">") {
            checkPassword();
        }
        else {
            if (m_input.size() < 4) {
                m_input += value;
                updateInputLabel();
                m_errorLabel->setString("");
            }
        }
    }

    void onClose(CCObject* sender) override {
        // Запрещаем закрывать попап, пока пароль не введён верно
        if (!m_unlocked) return;
        Popup::onClose(sender);
    }

public:
    static PasswordPopup* create() {
        auto ret = new PasswordPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

// ============================================================
//  PasswordLock method implementations
// ============================================================
void PasswordLock::lock() {
    if (m_popupShown) return;
    m_popupShown = true;
    m_locked = true;
    if (auto popup = PasswordPopup::create()) {
        popup->show();
    }
}

void PasswordLock::unlock() {
    m_locked = false;
    m_popupShown = false;
}

// ============================================================
//  MenuLayer hook — показываем попап при загрузке главного меню
// ============================================================
class $modify(MenuLayerHook, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        if (PasswordLock::get()->isLocked()) {
            Loader::get()->queueInMainThread([]() {
                PasswordLock::get()->lock();
            });
        }
        return true;
    }
};

// ============================================================
//  Keyboard hook — блокируем весь ввод, пока попап открыт
// ============================================================
class $modify(KeyboardHook, CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool isKeyDown, bool isKeyRepeat, double timestamp) {
        if (PasswordLock::get()->isLocked()) {
            if (isKeyDown) {
                int k = static_cast<int>(key);
                bool allowed = (k >= 48 && k <= 57) || k == 8 || k == 13;
                if (!allowed) return false;
            }
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isKeyRepeat, timestamp);
    }
};

// ============================================================
//  Mod init
// ============================================================
$on_mod(Loaded) {
    log::info("PasswordLock mod loaded!");
}