#include "tui_manager.h"
#include "../fuse/version_manager.h"
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <fstream>
#include <algorithm>

TUIManager::TUIManager() 
    : main_win(nullptr), header_win(nullptr), files_win(nullptr),
      versions_win(nullptr), details_win(nullptr), status_win(nullptr),
      selected_file_idx(0), selected_version_idx(0),
      current_view(FILES_VIEW) {
    
    char* env_root = getenv("VFS_BACKEND_ROOT");
    backend_root = env_root ? std::string(env_root) : "./runtime/data";
}

TUIManager::~TUIManager() {
    cleanup();
}

void TUIManager::init_ncurses() {
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_CYAN, COLOR_BLACK);    // Header
        init_pair(2, COLOR_BLACK, COLOR_WHITE);   // Selected
        init_pair(3, COLOR_YELLOW, COLOR_BLACK);  // Highlight
    }
}

void TUIManager::create_windows() {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    header_win = newwin(1, max_x, 0, 0);
    files_win = newwin(max_y - 3, max_x / 2, 1, 0);
    versions_win = newwin(max_y - 3, max_x / 2, 1, max_x / 2);
    status_win = newwin(2, max_x, max_y - 2, 0);
}

void TUIManager::cleanup() {
    if (header_win) delwin(header_win);
    if (files_win) delwin(files_win);
    if (versions_win) delwin(versions_win);
    if (details_win) delwin(details_win);
    if (status_win) delwin(status_win);
    endwin();
}

void TUIManager::load_files() {
    files.clear();
    DIR* dir = opendir(backend_root.c_str());
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == '.') continue;
        
        std::string fullpath = backend_root + "/" + entry->d_name;
        struct stat st;
        if (stat(fullpath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            files.push_back(entry->d_name);
        }
    }
    closedir(dir);
    std::sort(files.begin(), files.end());
}

void TUIManager::load_versions_for_file(const std::string& filename) {
    std::string fullpath = get_full_path(filename);
    versions = VersionManager::get_versions(fullpath);
    
    std::sort(versions.begin(), versions.end(), 
        [](const FileVersion& a, const FileVersion& b) {
            return a.version_number > b.version_number;
        });
    
    selected_version_idx = 0;
}

std::string TUIManager::get_full_path(const std::string& filename) {
    return backend_root + "/" + filename;
}

void TUIManager::draw_header() {
    werase(header_win);
    wattron(header_win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(header_win, 0, 2, "VFS VERSION MANAGER");
    wattroff(header_win, COLOR_PAIR(1) | A_BOLD);
    wrefresh(header_win);
}

void TUIManager::draw_files() {
    werase(files_win);
    box(files_win, 0, 0);
    mvwprintw(files_win, 0, 2, " FILES ");
    
    int max_y, max_x;
    getmaxyx(files_win, max_y, max_x);
    
    for (size_t i = 0; i < files.size() && (int)i < max_y - 2; i++) {
        if ((int)i == selected_file_idx && current_view == FILES_VIEW) {
            wattron(files_win, COLOR_PAIR(2) | A_BOLD);
        }
        
        int ver_count = VersionManager::get_version_count(get_full_path(files[i]));
        mvwprintw(files_win, i + 1, 2, "%s (%d)", files[i].c_str(), ver_count);
        
        if ((int)i == selected_file_idx && current_view == FILES_VIEW) {
            wattroff(files_win, COLOR_PAIR(2) | A_BOLD);
        }
    }
    wrefresh(files_win);
}

void TUIManager::draw_versions() {
    werase(versions_win);
    box(versions_win, 0, 0);
    mvwprintw(versions_win, 0, 2, " VERSIONS ");
    
    if (versions.empty()) {
        mvwprintw(versions_win, 1, 2, "No versions");
        wrefresh(versions_win);
        return;
    }
    
    int max_y, max_x;
    getmaxyx(versions_win, max_y, max_x);
    
    for (size_t i = 0; i < versions.size() && (int)i < max_y - 2; i++) {
        if ((int)i == selected_version_idx && current_view == VERSIONS_VIEW) {
            wattron(versions_win, COLOR_PAIR(2) | A_BOLD);
        }
        
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M", 
                 localtime(&versions[i].timestamp));
        
        mvwprintw(versions_win, i + 1, 2, "v%d  %s  %lub", 
                  versions[i].version_number,
                  time_str,
                  versions[i].size);
        
        if ((int)i == selected_version_idx && current_view == VERSIONS_VIEW) {
            wattroff(versions_win, COLOR_PAIR(2) | A_BOLD);
        }
    }
    wrefresh(versions_win);
}

void TUIManager::draw_details() {
    // Removed - simplified design
}

void TUIManager::draw_status(const std::string& message) {
    werase(status_win);
    mvwprintw(status_win, 0, 2, "%s", message.c_str());
    mvwprintw(status_win, 1, 2, "q:Quit TAB:Switch ↑↓:Navigate ENTER:Select r:Restore");
    wrefresh(status_win);
}

void TUIManager::refresh_all() {
    draw_header();
    draw_files();
    draw_versions();
    draw_status("Ready");
}

std::string TUIManager::format_timestamp(time_t timestamp) {
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&timestamp));
    return std::string(buf);
}

std::string TUIManager::format_size(size_t bytes) {
    if (bytes < 1024) return std::to_string(bytes) + "B";
    if (bytes < 1024*1024) return std::to_string(bytes/1024) + "KB";
    return std::to_string(bytes/(1024*1024)) + "MB";
}

void TUIManager::select_file() {
    if (files.empty() || selected_file_idx >= (int)files.size()) return;
    current_file = files[selected_file_idx];
    load_versions_for_file(current_file);
    current_view = VERSIONS_VIEW;
    selected_version_idx = 0;
}

void TUIManager::restore_version() {
    if (versions.empty() || selected_version_idx >= (int)versions.size()) return;
    
    std::string fullpath = get_full_path(current_file);
    int version_num = versions[selected_version_idx].version_number;
    
    if (VersionManager::restore_version(fullpath, version_num)) {
        draw_status("✓ Restored!");
    } else {
        draw_status("✗ Failed!");
    }
    refresh();
}

void TUIManager::handle_files_input(int ch) {
    switch (ch) {
        case KEY_UP:
            if (selected_file_idx > 0) selected_file_idx--;
            break;
        case KEY_DOWN:
            if (selected_file_idx < (int)files.size() - 1) selected_file_idx++;
            break;
        case '\n':
        case KEY_ENTER:
            select_file();
            break;
        case '\t':
            if (!versions.empty()) current_view = VERSIONS_VIEW;
            break;
    }
}

void TUIManager::handle_versions_input(int ch) {
    switch (ch) {
        case KEY_UP:
            if (selected_version_idx > 0) selected_version_idx--;
            break;
        case KEY_DOWN:
            if (selected_version_idx < (int)versions.size() - 1) selected_version_idx++;
            break;
        case '\t':
        case KEY_LEFT:
            current_view = FILES_VIEW;
            break;
        case 'r':
        case 'R':
            restore_version();
            break;
    }
}

void TUIManager::handle_input() {
    int ch = getch();
    if (ch == 'q' || ch == 'Q') return;
    
    if (current_view == FILES_VIEW) {
        handle_files_input(ch);
    } else {
        handle_versions_input(ch);
    }
}

void TUIManager::run() {
    init_ncurses();
    create_windows();
    load_files();
    
    if (!files.empty()) {
        current_file = files[0];
        load_versions_for_file(current_file);
    }
    
    while (true) {
        refresh_all();
        int ch = getch();
        
        if (ch == 'q' || ch == 'Q') break;
        
        if (current_view == FILES_VIEW) {
            handle_files_input(ch);
        } else {
            handle_versions_input(ch);
        }
    }
    
    cleanup();
}