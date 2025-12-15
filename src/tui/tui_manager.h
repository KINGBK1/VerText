#pragma once

#include <string>
#include <vector>
#include <ncurses.h>

using namespace std;

// Forward declarations
struct FileVersion;

class TUIManager {
public:
    TUIManager();
    ~TUIManager();
    
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
    vector<string> files;
    vector<FileVersion> versions;
    int selected_file_idx;
    int selected_version_idx;
    string current_file;
    string backend_root;
    string last_status_message;
    
    // UI State
    enum View { FILES_VIEW, VERSIONS_VIEW };
    View current_view;
    
    // Initialization
    void init_ncurses();
    void create_windows();
    void cleanup();
    
    // Data loading
    void load_files();
    void load_versions_for_file(const string& filename);
    
    // Drawing
    void draw_header();
    void draw_files();
    void draw_versions();
    void draw_details();
    void draw_status(const string& message);
    void draw_help_popup();
    void refresh_all();
    
    // Input handling
    void handle_input();
    void handle_files_input(int ch);
    void handle_versions_input(int ch);
    
    // Actions
    void select_file();
    void restore_version();
    void view_version_content();
    
    // Helpers
    string format_timestamp(time_t timestamp);
    string format_size(size_t bytes);
    string get_full_path(const string& filename);
};