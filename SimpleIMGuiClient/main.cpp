#include "lvgl.h"
#include <unistd.h>
#include <iostream>
#include <signal.h>

static bool running = true;

void signal_handler(int signal) {
    running = false;
}

// Event handler for button click
static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        std::cout << "Button clicked!" << std::endl;
        running = false;  // Exit the application
    }
}

int main(void)
{
    std::cout << "Starting SimpleIM GUI Client..." << std::endl;
    
    // Set up signal handler for Ctrl+C
    signal(SIGINT, signal_handler);

    // Initialize LVGL
    lv_init();

    // Create an SDL window
    lv_display_t* disp = lv_sdl_window_create(800, 600);
    
    // Add mouse and keyboard support
    lv_indev_t* mouse = lv_sdl_mouse_create();
    lv_indev_t* mousewheel = lv_sdl_mousewheel_create();
    lv_indev_t* keyboard = lv_sdl_keyboard_create();

    // Create a simple label widget
    lv_obj_t* label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "SimpleIM GUI Client");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, -50);

    // Create a button
    lv_obj_t* btn = lv_button_create(lv_scr_act());
    lv_obj_set_size(btn, 120, 50);
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 20);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

    // Add button label
    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "Exit");
    lv_obj_center(btn_label);

    std::cout << "LVGL initialized successfully!" << std::endl;
    std::cout << "Window created: 800x600" << std::endl;
    std::cout << "Mouse and keyboard input enabled" << std::endl;
    std::cout << "Click the Exit button or press Ctrl+C to close" << std::endl;

    // Main event loop
    while(running) {
        // Let LVGL handle tasks
        uint32_t time_till_next = lv_timer_handler();
        
        // Sleep for the time till next task
        usleep(time_till_next * 1000);
    }

    std::cout << "Exiting SimpleIM GUI Client..." << std::endl;
    return 0;
}
