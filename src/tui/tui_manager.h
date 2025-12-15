#pragma once

#include <string>
#include <vector>
#include <ncurses.h>

// Forward declarations
struct FileVersion;

class TUIManager {
public:
    TUIManager();
    ~TUIManager();
    
    // Main entry point
    void run();
    
private:
    // Windows
    WINDOW* main_win;
    WINDOW* header_win;
    WINDOW* files_win;
    WINDOW* versions_win;
    WINDOW* details_win;
    WINDOW* status_win;
    
    // Data
    std::vector<std::string> files;
    std::vector<FileVersion> versions;
    int selected_file_idx;
    int selected_version_idx;
    std::string current_file;
    std::string backend_root;
    
    // UI State
    enum View { FILES_VIEW, VERSIONS_VIEW };
    View current_view;
    
    // Initialization
    void init_ncurses();
    void create_windows();
    void cleanup();
    
    // Data loading
    void load_files();
    void load_versions_for_file(const std::string& filename);
    
    // Drawing
    void draw_header();
    void draw_files();
    void draw_versions();
    void draw_details();
    void draw_status(const std::string& message);
    void refresh_all();
    
    // Input handling
    void handle_input();
    void handle_files_input(int ch);
    void handle_versions_input(int ch);
    
    // Actions
    void select_file();
    void restore_version();
    void delete_version();
    void view_version_content();
    void compare_versions();
    
    // Helpers
    std::string format_timestamp(time_t timestamp);
    std::string format_size(size_t bytes);
    std::string get_full_path(const std::string& filename);
};