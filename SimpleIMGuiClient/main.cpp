#include "lvgl.h"
#include <SimpleIMClient.h>
#include "LoginDialog.h"
#include "EventHandler.h"

#include <unistd.h>
#include <signal.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

static bool m_terminate = false;

// Networking
static SimpleIMClient* client = nullptr;
static std::string current_username;

// UI Elements
static lv_obj_t* chat_list = nullptr;
static lv_obj_t* user_list = nullptr;
static lv_obj_t* input_textarea = nullptr;
static lv_obj_t* send_btn = nullptr;

// Login dialog
static im_gui::LoginDialog* login_dialog = nullptr;

// Login UI elements
static lv_obj_t* login_btn = nullptr;
static lv_obj_t* username_label = nullptr;

// Event coordinator
static std::shared_ptr<im_gui::EventHandler> m_event_handler = nullptr;
static std::vector<im_gui::EventSubscription> event_subscriptions;

// Input devices (for main chat interface)
static lv_indev_t* keyboard_device = nullptr;
static lv_group_t* main_input_group = nullptr;

// Forward declarations
void attempt_login(const std::string& username);

// void signal_handler(int signal) {
//     m_terminate = true;
// }

uint32_t tick_handler() {
    const auto now{std::chrono::steady_clock::now()};
    return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

// Function to add a message to the chat
void add_message_to_chat(const std::string& username, const std::string& message, bool is_own_message = false) {
    if (!chat_list) return;

    // Create a container for the message
    lv_obj_t* msg_container = lv_obj_create(chat_list);
    lv_obj_set_width(msg_container, lv_pct(95));
    lv_obj_set_height(msg_container, LV_SIZE_CONTENT);
    lv_obj_clear_flag(msg_container, LV_OBJ_FLAG_SCROLLABLE);
    
    // Style the message container
    if (is_own_message) {
        lv_obj_set_style_bg_color(msg_container, lv_color_hex(0x007ACC), 0);  // Blue for own messages
    } else {
        lv_obj_set_style_bg_color(msg_container, lv_color_hex(0x404040), 0);  // Gray for others
    }
    lv_obj_set_style_border_width(msg_container, 1, 0);
    lv_obj_set_style_border_color(msg_container, lv_color_hex(0x666666), 0);
    lv_obj_set_style_radius(msg_container, 8, 0);
    lv_obj_set_style_pad_all(msg_container, 8, 0);

    // Create message text as a single label
    lv_obj_t* msg_label = lv_label_create(msg_container);
    
    // Format the message with username
    std::string formatted_msg;
    if (is_own_message) {
        formatted_msg = message;
    } else {
        formatted_msg = username + ": " + message;
    }
    
    lv_label_set_text(msg_label, formatted_msg.c_str());
    lv_obj_set_style_text_color(msg_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_width(msg_label, lv_pct(100));
    lv_label_set_long_mode(msg_label, LV_LABEL_LONG_WRAP);
    lv_obj_align(msg_label, LV_ALIGN_TOP_LEFT, 0, 0);

    // Scroll to bottom to show new message
    lv_obj_scroll_to_y(chat_list, LV_COORD_MAX, LV_ANIM_ON);
}

// Function to add a user to the user list
void add_user_to_list(const std::string& username, bool is_online = true) {
    if (!user_list) return;

    lv_obj_t* user_item = lv_obj_create(user_list);
    lv_obj_set_width(user_item, lv_pct(100));
    lv_obj_set_height(user_item, LV_SIZE_CONTENT);
    lv_obj_clear_flag(user_item, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(user_item, lv_color_hex(0x333333), 0);
    lv_obj_set_style_pad_all(user_item, 5, 0);

    lv_obj_t* user_label = lv_label_create(user_item);
    lv_label_set_text(user_label, username.c_str());
    
    // Style based on online status
    if (is_online) {
        lv_obj_set_style_text_color(user_label, lv_color_hex(0x00FF00), 0);  // Green for online
    } else {
        lv_obj_set_style_text_color(user_label, lv_color_hex(0x808080), 0);  // Gray for offline
    }
    lv_obj_align(user_label, LV_ALIGN_LEFT_MID, 0, 0);
}

// Function to clear the user list
void clear_user_list() {
    if (!user_list) return;
    lv_obj_clean(user_list);
}

// Function to update user list from comma-separated string
void update_user_list(const std::string& userListStr) {
    clear_user_list();
    
    if (userListStr.empty()) {
        return;
    }
    
    std::istringstream iss(userListStr);
    std::string username;
    
    while (std::getline(iss, username, ',')) {
        if (!username.empty()) {
            add_user_to_list(username, true);
        }
    }
}

// Event handler for send button
static void send_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        const char* text = lv_textarea_get_text(input_textarea);
        if (strlen(text) > 0 && client && client->connected()) {
            std::cout << "Sending message: " << text << std::endl;
            
            // Send message via network
            client->sendChatMessage(text);
            
            // Add message to chat as own message
            add_message_to_chat(current_username, text, true);
            
            // Clear input field
            lv_textarea_set_text(input_textarea, "");
        } else if (!client || !client->connected()) {
            add_message_to_chat("System", "Not connected to server", false);
        }
    }
}

// Event handler for input textarea (Enter key)
static void input_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER) {
            // Simulate send button click
            const char* text = lv_textarea_get_text(input_textarea);
            if (strlen(text) > 0 && client && client->connected()) {
                std::cout << "Sending message: " << text << std::endl;
                
                // Send message via network
                client->sendChatMessage(text);
                
                // Add message to chat as own message
                add_message_to_chat(current_username, text, true);
                
                // Clear input field
                lv_textarea_set_text(input_textarea, "");
            } else if (!client || !client->connected()) {
                add_message_to_chat("System", "Not connected to server", false);
            }
        }
    }
}

// Forward declarations
void attempt_login(const std::string& username);

// Event handler functions
void handle_login_requested(im_gui::Event event, const std::any& data) {
    try {
        std::string username = std::any_cast<std::string>(data);
        attempt_login(username);
    } catch (const std::bad_any_cast& e) {
        std::cerr << "Error handling login request: " << e.what() << std::endl;
    }
}

void handle_login_success(im_gui::Event event, const std::any& data) {
    try {
        std::string username = std::any_cast<std::string>(data);
        std::cout << "Login successful for: " << username << std::endl;
        
        // Hide login button and show username
        if (login_btn) {
            lv_obj_add_flag(login_btn, LV_OBJ_FLAG_HIDDEN);
        }
        if (username_label) {
            std::string display_text = "User: " + username;
            lv_label_set_text(username_label, display_text.c_str());
            lv_obj_clear_flag(username_label, LV_OBJ_FLAG_HIDDEN);
        }
    } catch (const std::bad_any_cast& e) {
        std::cerr << "Error handling login success: " << e.what() << std::endl;
    }
}

// Event handler for login button
static void login_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        std::cout << "Login button clicked!" << std::endl;
        if (login_dialog) {
            login_dialog->show();
            lv_timer_handler(); // Process the dialog show() operation
        }
    }
}

lv_obj_t* create_chat_ui() {

    lv_obj_t* main_window = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_window, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(main_window, 5, 0);
    lv_obj_set_style_bg_color(main_window, lv_color_hex(0x1E1E1E), 0);

    // Create left panel for user list
    lv_obj_t* left_panel = lv_obj_create(main_window);
    lv_obj_set_size(left_panel, 250, lv_pct(100));
    lv_obj_align(left_panel, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(left_panel, lv_color_hex(0x2D2D2D), 0);

    // User list title
    lv_obj_t* user_title = lv_label_create(left_panel);
    lv_label_set_text(user_title, "Connected Users");
    lv_obj_set_style_text_color(user_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(user_title, LV_ALIGN_TOP_MID, 0, 10);

    // Create user list (simple scrollable container)
    user_list = lv_obj_create(left_panel);
    lv_obj_set_size(user_list, lv_pct(90), lv_pct(85));
    lv_obj_align(user_list, LV_ALIGN_CENTER, 0, 15);
    lv_obj_set_style_bg_color(user_list, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_flex_flow(user_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(user_list, 5, 0);
    lv_obj_set_style_pad_gap(user_list, 2, 0);

    // Create right panel for chat
    lv_obj_t* right_panel = lv_obj_create(main_window);
    lv_obj_set_size(right_panel, 730, lv_pct(100));
    lv_obj_align(right_panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(right_panel, lv_color_hex(0x1E1E1E), 0);

    // Chat title
    lv_obj_t* chat_title = lv_label_create(right_panel);
    lv_label_set_text(chat_title, "SimpleIM Chat");
    lv_obj_set_style_text_color(chat_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(chat_title, LV_ALIGN_TOP_MID, 0, 10);

    // Create login button (top right)
    login_btn = lv_button_create(right_panel);
    lv_obj_set_size(login_btn, 80, 30);
    lv_obj_align(login_btn, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_add_event_cb(login_btn, login_btn_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_style_bg_color(login_btn, lv_color_hex(0x007ACC), 0);

    lv_obj_t* login_label = lv_label_create(login_btn);
    lv_label_set_text(login_label, "Login");
    lv_obj_set_style_text_color(login_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(login_label);

    // Create username label (hidden initially)
    username_label = lv_label_create(right_panel);
    lv_obj_align(username_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_text_color(username_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_add_flag(username_label, LV_OBJ_FLAG_HIDDEN);

    // Create chat history area
    chat_list = lv_obj_create(right_panel);
    lv_obj_set_size(chat_list, lv_pct(95), 450);
    lv_obj_align(chat_list, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_style_bg_color(chat_list, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_border_color(chat_list, lv_color_hex(0x404040), 0);
    lv_obj_set_style_border_width(chat_list, 1, 0);
    lv_obj_set_flex_flow(chat_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(chat_list, 5, 0);
    lv_obj_set_style_pad_gap(chat_list, 5, 0);

    // Create input area container
    lv_obj_t* input_container = lv_obj_create(right_panel);
    lv_obj_set_size(input_container, lv_pct(95), 120);
    lv_obj_align(input_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_color(input_container, lv_color_hex(0x2D2D2D), 0);
    lv_obj_clear_flag(input_container, LV_OBJ_FLAG_SCROLLABLE);

    // Create input textarea
    input_textarea = lv_textarea_create(input_container);
    lv_obj_set_size(input_textarea, 500, 80);
    lv_obj_align(input_textarea, LV_ALIGN_LEFT_MID, 10, 0);
    lv_textarea_set_placeholder_text(input_textarea, "Type your message...");
    lv_obj_set_style_bg_color(input_textarea, lv_color_hex(0x404040), 0);
    
    // Set text color for all parts (like in LoginDialog)
    lv_obj_set_style_text_color(input_textarea, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_color(input_textarea, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_color(input_textarea, lv_color_hex(0xFFFFFF), LV_PART_TEXTAREA_PLACEHOLDER);
    lv_obj_set_style_text_color(input_textarea, lv_color_hex(0x000000), LV_PART_CURSOR);
    lv_obj_set_style_bg_color(input_textarea, lv_color_hex(0xFFFFFF), LV_PART_CURSOR);
    
    // Ensure proper padding for text display
    lv_obj_set_style_pad_left(input_textarea, 8, 0);
    lv_obj_set_style_pad_right(input_textarea, 8, 0);
    lv_obj_set_style_pad_top(input_textarea, 8, 0);
    lv_obj_set_style_pad_bottom(input_textarea, 8, 0);
    
    // Make it focusable and add focus state
    lv_obj_add_flag(input_textarea, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_state(input_textarea, LV_STATE_FOCUSED);
    
    lv_obj_add_event_cb(input_textarea, input_event_cb, LV_EVENT_KEY, nullptr);

    // Create send button
    send_btn = lv_button_create(input_container);
    lv_obj_set_size(send_btn, 150, 80);
    lv_obj_align(send_btn, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_obj_add_event_cb(send_btn, send_btn_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_style_bg_color(send_btn, lv_color_hex(0x007ACC), 0);

    lv_obj_t* send_label = lv_label_create(send_btn);
    lv_label_set_text(send_label, "Send");
    lv_obj_set_style_text_color(send_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(send_label);

    return main_window;
}

// Function to setup keyboard handling for main chat interface
void setup_main_keyboard_handling() {
    if(keyboard_device && input_textarea) {
        // Create the main input group if it doesn't exist
        if (!main_input_group) {
            main_input_group = lv_group_create();
        }
        
        lv_group_add_obj(main_input_group, input_textarea);
        lv_group_add_obj(main_input_group, send_btn);
        lv_indev_set_group(keyboard_device, main_input_group);
        lv_group_focus_obj(input_textarea);
        
        lv_obj_add_state(input_textarea, LV_STATE_FOCUSED);
    }
}

// Function to handle login attempt
void attempt_login(const std::string& username) {
    if (username.empty()) {
        current_username = "Guest";
        add_message_to_chat("System", "No username provided. Running in offline mode.", false);
    } else {
        current_username = username;
        
        // Add initial system message
        add_message_to_chat("System", "Connecting to server...", false);
        
        // Connect to server
        client->logon(username);
        
        // Give some time for connection
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        if (client->connected()) {
            add_message_to_chat("System", "Connected! You can now chat.", false);
            m_event_handler->NotifySubscribers(im_gui::Event::LOGIN_SUCCESS, current_username);
        } else {
            add_message_to_chat("System", "Failed to connect to server. Running in offline mode.", false);
            m_event_handler->NotifySubscribers(im_gui::Event::LOGIN_SUCCESS, current_username); // Still show as logged in
        }
    }
    setup_main_keyboard_handling();
}

int main(void)
{
    std::cout << "Starting SimpleIM GUI Client..." << std::endl;
    
    // Set up signal handler for Ctrl+C
    // signal(SIGINT, signal_handler);

    std::filesystem::current_path(std::filesystem::canonical("/proc/self/exe").parent_path());
    lv_init();
    lv_tick_set_cb(tick_handler);

    lv_display_t* disp = lv_sdl_window_create(1000, 700);
    lv_sdl_window_set_title(disp, "Simple IM");
    
    // Add mouse and keyboard support
    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
    keyboard_device = lv_sdl_keyboard_create();

    // Create the chat UI and get reference to main window
    lv_obj_t* main_window = create_chat_ui();

    // Initialize event coordinator
    m_event_handler = std::make_shared<im_gui::EventHandler>();
    
    // Subscribe to events
    event_subscriptions.push_back(
        m_event_handler->Subscribe(im_gui::Event::LOGIN_REQUESTED, handle_login_requested)
    );
    event_subscriptions.push_back(
        m_event_handler->Subscribe(im_gui::Event::LOGIN_SUCCESS, handle_login_success)
    );
    
    // Initialize networking
    client = new SimpleIMClient();
    
    std::cout << "LVGL Chat UI initialized successfully!" << std::endl;
    std::cout << "Window created: 1000x700" << std::endl;
    std::cout << "Mouse and keyboard input enabled" << std::endl;

    // Load the main screen first
    lv_scr_load(main_window);
       
    // Create login dialog but don't show it immediately
    login_dialog = new im_gui::LoginDialog(m_event_handler); 
    
    std::cout << "Chat interface ready. Click the Login button to connect." << std::endl;

    constexpr std::chrono::milliseconds lv_delay{1};
    while(!m_terminate) {
        std::this_thread::sleep_for(lv_delay);
        lv_timer_handler();

        m_event_handler->ProcessEvents();
    }

    // Clean up
    event_subscriptions.clear();
    
    // Clean up input group
    if (main_input_group) {
        lv_group_del(main_input_group);
        main_input_group = nullptr;
    }
      
    if (login_dialog) {
        delete login_dialog;
        login_dialog = nullptr;
    }
    
    if (client) {
        client->disconnectFromServer();
        delete client;
        client = nullptr;
    }

    std::cout << "Exiting SimpleIM GUI Client..." << std::endl;
    lv_deinit();
    return 0;
}
