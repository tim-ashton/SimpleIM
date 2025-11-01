#include "lvgl.h"
#include <SimpleIMClient.h>
#include <unistd.h>
#include <iostream>
#include <signal.h>
#include <string>
#include <vector>
#include <cstring>
#include <sstream>
#include <thread>
#include <chrono>

static bool running = true;

// Networking
static SimpleIMClient* client = nullptr;
static std::string current_username;

// UI Elements
static lv_obj_t* chat_list = nullptr;
static lv_obj_t* user_list = nullptr;
static lv_obj_t* input_textarea = nullptr;
static lv_obj_t* send_btn = nullptr;

// Login dialog elements
static lv_obj_t* login_modal = nullptr;
static lv_obj_t* username_input = nullptr;
static bool login_completed = false;

void signal_handler(int signal) {
    running = false;
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

// Event handler for exit
static void exit_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        std::cout << "Exit button clicked!" << std::endl;
        
        if (client) {
            client->disconnectFromServer();
        }
        
        running = false;
    }
}

void create_chat_ui() {
    // Create main container
    lv_obj_t* main_cont = lv_obj_create(lv_scr_act());
    lv_obj_set_size(main_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(main_cont, 5, 0);
    lv_obj_set_style_bg_color(main_cont, lv_color_hex(0x1E1E1E), 0);

    // Create left panel for user list
    lv_obj_t* left_panel = lv_obj_create(main_cont);
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
    lv_obj_t* right_panel = lv_obj_create(main_cont);
    lv_obj_set_size(right_panel, 730, lv_pct(100));
    lv_obj_align(right_panel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(right_panel, lv_color_hex(0x1E1E1E), 0);

    // Chat title
    lv_obj_t* chat_title = lv_label_create(right_panel);
    lv_label_set_text(chat_title, "SimpleIM Chat");
    lv_obj_set_style_text_color(chat_title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(chat_title, LV_ALIGN_TOP_MID, 0, 10);

    // Create exit button (top right)
    lv_obj_t* exit_btn = lv_button_create(right_panel);
    lv_obj_set_size(exit_btn, 60, 30);
    lv_obj_align(exit_btn, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_add_event_cb(exit_btn, exit_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_style_bg_color(exit_btn, lv_color_hex(0xFF4444), 0);

    lv_obj_t* exit_label = lv_label_create(exit_btn);
    lv_label_set_text(exit_label, "Exit");
    lv_obj_set_style_text_color(exit_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(exit_label);

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
    lv_obj_set_style_text_color(input_textarea, lv_color_hex(0xFFFFFF), 0);
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
        } else {
            add_message_to_chat("System", "Failed to connect to server. Running in offline mode.", false);
        }
    }
}

// Event handler for login button
static void login_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        const char* username_text = lv_textarea_get_text(username_input);
        std::string username(username_text ? username_text : "");
        
        // Close the modal
        lv_obj_del(login_modal);
        login_modal = nullptr;
        login_completed = true;
        
        // Attempt login
        attempt_login(username);
    }
}

// Event handler for login dialog key events (Enter to login)
static void login_input_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_indev_get_key(lv_indev_get_act());
        if(key == LV_KEY_ENTER) {
            // Trigger login
            const char* username_text = lv_textarea_get_text(username_input);
            std::string username(username_text ? username_text : "");
            
            // Close the modal
            lv_obj_del(login_modal);
            login_modal = nullptr;
            login_completed = true;
            
            // Attempt login
            attempt_login(username);
        }
    }
}

// Function to create login modal dialog
void create_login_dialog() {
    // Create modal background
    login_modal = lv_obj_create(lv_scr_act());
    lv_obj_set_size(login_modal, lv_pct(100), lv_pct(100));
    lv_obj_set_style_bg_color(login_modal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(login_modal, 180, 0);  // Semi-transparent
    lv_obj_clear_flag(login_modal, LV_OBJ_FLAG_SCROLLABLE);

    // Create dialog container
    lv_obj_t* dialog = lv_obj_create(login_modal);
    lv_obj_set_size(dialog, 400, 250);
    lv_obj_center(dialog);
    lv_obj_set_style_bg_color(dialog, lv_color_hex(0x2D2D2D), 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex(0x007ACC), 0);
    lv_obj_set_style_border_width(dialog, 2, 0);
    lv_obj_set_style_radius(dialog, 10, 0);
    lv_obj_clear_flag(dialog, LV_OBJ_FLAG_SCROLLABLE);

    // Title
    lv_obj_t* title = lv_label_create(dialog);
    lv_label_set_text(title, "Welcome to SimpleIM");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // Username label
    lv_obj_t* username_label = lv_label_create(dialog);
    lv_label_set_text(username_label, "Enter your username:");
    lv_obj_set_style_text_color(username_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(username_label, LV_ALIGN_TOP_LEFT, 20, 70);

    // Username input
    username_input = lv_textarea_create(dialog);
    lv_obj_set_size(username_input, 360, 40);
    lv_obj_align(username_input, LV_ALIGN_TOP_LEFT, 20, 100);
    lv_textarea_set_placeholder_text(username_input, "Username");
    lv_obj_set_style_bg_color(username_input, lv_color_hex(0x404040), 0);
    lv_obj_set_style_text_color(username_input, lv_color_hex(0xFFFFFF), 0);
    lv_textarea_set_one_line(username_input, true);
    lv_obj_add_event_cb(username_input, login_input_event_cb, LV_EVENT_KEY, nullptr);

    // Login button
    lv_obj_t* login_btn = lv_button_create(dialog);
    lv_obj_set_size(login_btn, 120, 40);
    lv_obj_align(login_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(login_btn, login_btn_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_set_style_bg_color(login_btn, lv_color_hex(0x007ACC), 0);

    lv_obj_t* login_label = lv_label_create(login_btn);
    lv_label_set_text(login_label, "Connect");
    lv_obj_set_style_text_color(login_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(login_label);

    // Focus on the input field
    lv_group_t* group = lv_group_create();
    lv_group_add_obj(group, username_input);
    lv_indev_set_group(lv_indev_get_next(nullptr), group);
}

int main(void)
{
    std::cout << "Starting SimpleIM GUI Client..." << std::endl;
    
    // Set up signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    // Initialize LVGL
    lv_init();

    // Create an SDL window
    lv_display_t* disp = lv_sdl_window_create(1000, 700);
    
    // Add mouse and keyboard support
    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
    lv_indev_t* keyboard = lv_sdl_keyboard_create();

    // Create the chat UI
    create_chat_ui();

    // Initialize networking
    client = new SimpleIMClient();
    
    // Show login dialog
    create_login_dialog();

    std::cout << "LVGL Chat UI initialized successfully!" << std::endl;
    std::cout << "Window created: 1000x700" << std::endl;
    std::cout << "Mouse and keyboard input enabled" << std::endl;
    std::cout << "Waiting for login..." << std::endl;

    // Main event loop
    while(running) {
        // Let LVGL handle tasks
        uint32_t time_till_next = lv_timer_handler();
        
        // Sleep for the time till next task
        usleep(time_till_next * 1000);
    }

    // Clean up
    if (client) {
        client->disconnectFromServer();
        delete client;
        client = nullptr;
    }

    std::cout << "Exiting SimpleIM GUI Client..." << std::endl;
    return 0;
}
