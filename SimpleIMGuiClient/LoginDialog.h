#pragma once

#include <lvgl.h>
#include <string>
#include <functional>

namespace im_gui
{

class LoginDialog
{
public:
    // Callback type for when login is completed
    using LoginCallback = std::function<void(const std::string& username)>;

public:
    LoginDialog(LoginCallback callback);
    ~LoginDialog();

    void show();   
    void hide();
    bool isShown() const;

private:
    // LVGL objects
    lv_obj_t* m_modal = nullptr;
    lv_obj_t* m_dialog = nullptr;
    lv_obj_t* m_username_input = nullptr;
    lv_obj_t* m_login_btn = nullptr;
    
    lv_indev_t* m_keyboard_device = nullptr;
    lv_group_t* m_input_group = nullptr;
    
    LoginCallback m_login_callback;
    
    static void login_btn_event_cb(lv_event_t* e);
    static void username_input_event_cb(lv_event_t* e);
    static void username_focus_cb(lv_event_t* e);
    static void username_value_changed_cb(lv_event_t* e);
    
    void setupKeyboardHandling();
    lv_indev_t* findKeyboardDevice();
    void performLogin();
};

} // end namespace




