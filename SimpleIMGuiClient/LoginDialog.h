#pragma once

#include <lvgl.h>
#include <string>
#include "EventHandler.h"

namespace im_gui
{

class LoginDialog
{
public:
    LoginDialog(const std::shared_ptr<EventHandler> &event_handler);
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
    
    std::shared_ptr<EventHandler> m_event_handler;
    
    static void login_btn_event_cb(lv_event_t* e);
    static void username_input_event_cb(lv_event_t* e);
    static void username_focus_cb(lv_event_t* e);
    // static void username_value_changed_cb(lv_event_t* e);
    
    void setupKeyboardHandling();
    lv_indev_t* findKeyboardDevice();
    void performLogin();
};

} // end namespace




