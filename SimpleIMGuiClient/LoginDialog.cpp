#include "LoginDialog.h"
#include <iostream>

namespace im_gui
{

LoginDialog::LoginDialog(const std::shared_ptr<EventHandler> &event_handler)
    : m_modal(nullptr)
    , m_dialog(nullptr)
    , m_username_input(nullptr)
    , m_login_btn(nullptr)
    , m_keyboard_device(nullptr)
    , m_input_group(nullptr)
    , m_event_handler(event_handler)
{   
    // Create modal background on the top layer to ensure it's above everything
    lv_obj_t* top_layer = lv_layer_top();
    m_modal = lv_obj_create(top_layer);
    lv_obj_set_size(m_modal, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(m_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(m_modal, 180, 0);  // Semi-transparent
    lv_obj_clear_flag(m_modal, LV_OBJ_FLAG_SCROLLABLE);
    
    // Hide the modal initially - only show when explicitly called
    lv_obj_add_flag(m_modal, LV_OBJ_FLAG_HIDDEN);
    
    // Set user data to this instance for callbacks
    lv_obj_set_user_data(m_modal, this);

    // Create dialog container
    m_dialog = lv_obj_create(m_modal);
    lv_obj_set_size(m_dialog, 400, 250);
    lv_obj_center(m_dialog);
    lv_obj_set_style_bg_color(m_dialog, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_border_color(m_dialog, lv_color_hex(0x007ACC), 0);
    lv_obj_set_style_border_width(m_dialog, 2, 0);
    lv_obj_set_style_radius(m_dialog, 10, 0);
    lv_obj_clear_flag(m_dialog, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(m_dialog);
    lv_label_set_text(title, "Welcome to SimpleIM");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Username label
    lv_obj_t* username_label = lv_label_create(m_dialog);
    lv_label_set_text(username_label, "Enter your username:");
    lv_obj_set_style_text_color(username_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(username_label, LV_ALIGN_TOP_LEFT, 40, 70);

    // Username input
    m_username_input = lv_textarea_create(m_dialog);
    lv_textarea_set_one_line(m_username_input, true);
    lv_obj_set_size(m_username_input, 320, 40);
    lv_obj_align(m_username_input, LV_ALIGN_TOP_LEFT, 40, 100);
    lv_textarea_set_placeholder_text(m_username_input, "Username");
    lv_obj_set_style_bg_color(m_username_input, lv_color_hex(0x404040), 0);
    
    // Set text color for all parts
    lv_obj_set_style_text_color(m_username_input, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(m_username_input, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_color(m_username_input, lv_color_hex(0xFFFFFF), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_color(m_username_input, lv_color_hex(0x000000), LV_PART_CURSOR);
    lv_obj_set_style_bg_color(m_username_input, lv_color_hex(0xFFFFFF), LV_PART_CURSOR);
    
    // Ensure proper padding for text display
    lv_obj_set_style_pad_left(m_username_input, 8, 0);
    lv_obj_set_style_pad_right(m_username_input, 8, 0);
    lv_obj_set_style_pad_top(m_username_input, 8, 0);
    lv_obj_set_style_pad_bottom(m_username_input, 8, 0);
       
    // Set user data for callbacks
    lv_obj_set_user_data(m_username_input, this);
    
    // Add event callbacks
    lv_obj_add_event_cb(m_username_input, username_input_event_cb, LV_EVENT_READY, nullptr);
    lv_obj_add_event_cb(m_username_input, username_focus_cb, LV_EVENT_CLICKED, nullptr);
    
    // Add callbacks to refresh display on any text changes
    lv_obj_add_event_cb(m_username_input, username_value_changed_cb, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_obj_add_event_cb(m_username_input, username_value_changed_cb, LV_EVENT_INSERT, nullptr);
    
    lv_obj_add_flag(m_username_input, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_state(m_username_input, LV_STATE_FOCUSED);

    // Login button
    m_login_btn = lv_button_create(m_dialog);
    lv_obj_set_size(m_login_btn, 120, 40);
    lv_obj_align(m_login_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(m_login_btn, lv_color_hex(0x007ACC), 0);
    
    // Set user data for callbacks
    lv_obj_set_user_data(m_login_btn, this);
    lv_obj_add_event_cb(m_login_btn, login_btn_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(m_login_btn, login_btn_event_cb, LV_EVENT_KEY, nullptr);
    
    // Make sure button can be focused and clicked
    lv_obj_add_flag(m_login_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    
    std::cout << "Login button created and event callbacks added" << std::endl;

    lv_obj_t* login_label = lv_label_create(m_login_btn);
    lv_label_set_text(login_label, "Connect");
    lv_obj_set_style_text_color(login_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(login_label);

    m_keyboard_device = findKeyboardDevice();
    setupKeyboardHandling();
}

LoginDialog::~LoginDialog()
{
    // Ensure non parented objects are cleaned up.
    if (m_modal) {
        lv_obj_del(m_modal);
        m_modal = nullptr;
    }
}

void LoginDialog::show()
{
    if (m_modal) {
        lv_obj_clear_flag(m_modal, LV_OBJ_FLAG_HIDDEN);
               
        // Force invalidation and refresh
        lv_obj_invalidate(m_modal);
        
        if (m_input_group && m_username_input) {
            lv_group_focus_obj(m_username_input);
            lv_obj_add_state(m_username_input, LV_STATE_FOCUSED);
        }
    }
}

void LoginDialog::hide()
{
    if (m_modal) {
        lv_obj_add_flag(m_modal, LV_OBJ_FLAG_HIDDEN);
    }
}

bool LoginDialog::isShown() const
{
    return m_modal != nullptr && !lv_obj_has_flag(m_modal, LV_OBJ_FLAG_HIDDEN);
}

void LoginDialog::setupKeyboardHandling()
{
    if (m_keyboard_device && m_username_input && m_login_btn) {
        std::cout << "Setting up keyboard handling with input group" << std::endl;
        
        // Create input group
        m_input_group = lv_group_create();
        lv_group_add_obj(m_input_group, m_username_input);
        lv_group_add_obj(m_input_group, m_login_btn);
        
        // Set the group to the keyboard input device
        lv_indev_set_group(m_keyboard_device, m_input_group);
        
        // Focus the username input
        lv_group_focus_obj(m_username_input);
        lv_obj_add_state(m_username_input, LV_STATE_FOCUSED);
        
        std::cout << "Keyboard handling setup complete" << std::endl;
    } else {
        std::cout << "Warning: Cannot setup keyboard handling - missing components:" << std::endl;
        std::cout << "  keyboard_device: " << (m_keyboard_device ? "OK" : "NULL") << std::endl;
        std::cout << "  username_input: " << (m_username_input ? "OK" : "NULL") << std::endl;
        std::cout << "  login_btn: " << (m_login_btn ? "OK" : "NULL") << std::endl;
    }
}

lv_indev_t* LoginDialog::findKeyboardDevice()
{
    lv_indev_t* indev = nullptr;
    while((indev = lv_indev_get_next(indev)) != nullptr) {
        if(lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD) {
            std::cout << "Found keyboard input device" << std::endl;
            return indev;
        }
    }
    std::cout << "Warning: No keyboard input device found" << std::endl;
    return nullptr;
}

void LoginDialog::performLogin()
{
    if (m_username_input && m_event_handler) {
        const char* username_text = lv_textarea_get_text(m_username_input);
        std::string username(username_text ? username_text : "");
        
        std::cout << "Performing login for user: " << username << std::endl;
        
        hide();
        
        m_event_handler->NotifySubscribers(Event::LOGIN_REQUESTED, username);
    }
}

// Static event handlers
void LoginDialog::login_btn_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    std::cout << "login_btn_event_cb called with event: " << code << std::endl;
    
    if(code == LV_EVENT_CLICKED) {
        std::cout << "Button clicked - processing login" << std::endl;
        lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
        LoginDialog* dialog = static_cast<LoginDialog*>(lv_obj_get_user_data(target));
        if (dialog) {
            dialog->performLogin();
        }
    } else if(code == LV_EVENT_KEY) {
        std::cout << "Button key event received" << std::endl;
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        std::cout << "Key: " << key << std::endl;
        if(key == LV_KEY_ENTER) {
            std::cout << "Enter key on button - processing login" << std::endl;
            lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
            LoginDialog* dialog = static_cast<LoginDialog*>(lv_obj_get_user_data(target));
            if (dialog) {
                dialog->performLogin();
            }
        }
    }
}

void LoginDialog::username_input_event_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_READY) {
        std::cout << "Username input ready - logging in" << std::endl;
        lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
        LoginDialog* dialog = static_cast<LoginDialog*>(lv_obj_get_user_data(target));
        if (dialog) {
            dialog->performLogin();
        }
    }
}

void LoginDialog::username_focus_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        std::cout << "Username input clicked - focusing" << std::endl;
        lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
        LoginDialog* dialog = static_cast<LoginDialog*>(lv_obj_get_user_data(target));
        if (dialog && dialog->m_username_input) {
            lv_obj_add_state(dialog->m_username_input, LV_STATE_FOCUSED);
            if(dialog->m_keyboard_device && dialog->m_input_group) {
                lv_group_focus_obj(dialog->m_username_input);
            }
        }
    }
}

void LoginDialog::username_value_changed_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_INSERT) {
        std::cout << "Username text changed (event: " << code << ")" << std::endl;
        lv_obj_t* target = static_cast<lv_obj_t*>(lv_event_get_target(e));
        
        // Get and print current text for debugging
        const char* current_text = lv_textarea_get_text(target);
        std::cout << "Current text in textarea: '" << (current_text ? current_text : "NULL") << "'" << std::endl;
    }
}

} // end namespace
