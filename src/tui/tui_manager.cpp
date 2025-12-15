#include "tui_manager.h"
#include "../fuse/version_manager.h"
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>

TUIManager::TUIManager() 
    : main_win(nullptr), header_win(nullptr), files_win(nullptr),
      versions_win(nullptr), details_win(nullptr), status_win(nullptr),
      selected_file_idx(0), selected_version_idx(0),
      current_view(FILES_VIEW) {
    
    // Get backend root from environment or default
    char* env_root = getenv("VFS_BACKEND_ROOT");
    backend_root = env_root ? std::string(env_root) : "./runtime/data";
}

TUIManager::~TUIManager() {
    cleanup();
}

void TUIManager::init_ncurses() {
    initscr();              // Initialize ncurses
    cbreak();               // Disable line buffering
    noecho();               // Don't echo input
    keypad(stdscr, TRUE);   // Enable function keys
    curs_set(0);            // Hide cursor
    
    // Initialize colors
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_BLACK, COLOR_CYAN);    // Header
        init_pair(2, COLOR_WHITE, COLOR_BLUE);    // Selected item
        init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Normal text
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);  // Highlight
        init_pair(5, COLOR_RED, COLOR_BLACK);     // Error/Warning
    }
}

void TUIManager::create_windows() {
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    
    // Header (top 3 lines)
    header_win = newwin(3, max_x, 0, 0);
    wbkgd(header_win, COLOR_PAIR(1));
    
    // Files list (left side, below header)
    files_win = newwin(max_y - 5, max_x / 2, 3, 0);
    box(files_win, 0, 0);
    
    // Versions list (right side, below header, top half)
    versions_win = newwin((max_y - 5) / 2, max_x / 2, 3, max_x / 2);
    box(versions_win, 0, 0);
    
    // Details (right side, below versions)
    details_win = newwin((max_y - 5) / 2, max_x / 2, 3 + (max_y - 5) / 2, max_x / 2);
    box(details_win, 0, 0);
    
    // Status bar (bottom 2 lines)
    status_win = newwin(2, max_x, max_y - 2, 0);
    wbkgd(status_win, COLOR_PAIR(1));
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
        if (entry->d_name[0] == '.') continue;  // Skip hidden files
        
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
    
    // Sort by version number (newest first)
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
    
    mvwprintw(header_win, 0, 2, "╔═══════════════════════════════════════════════════════════════╗");
    mvwprintw(header_win, 1, 2, "║  VERSIONED VFS - File Version Manager  ║");
    mvwprintw(header_win, 2, 2, "╚═══════════════════════════════════════════════════════════════╝");
    
    wattroff(header_win, COLOR_PAIR(1) | A_BOLD);
    wrefresh(header_win);
}

void TUIManager::draw_files() {
    werase(files_win);
    box(files_win, 0, 0);
    
    wattron(files_win, A_BOLD);
    mvwprintw(files_win, 0, 2, "[ FILES ]");
    wattroff(files_win, A_BOLD);
    
    int max_y, max_x;
    getmaxyx(files_win, max_y, max_x);
    
    int start = std::max(0, selected_file_idx - (max_y - 4));
    
    for (size_t i = start; i < files.size() && (int)(i - start) < max_y - 3; i++) {
        int y = 2 + (i - start);
        
        if ((int)i == selected_file_idx && current_view == FILES_VIEW) {
            wattron(files_win, COLOR_PAIR(2) | A_BOLD);
            mvwprintw(files_win, y, 1, " > ");
        } else {
            mvwprintw(files_win, y, 1, "   ");
        }
        
        // Get version count
        std::string fullpath = get_full_path(files[i]);
        int ver_count = VersionManager::get_version_count(fullpath);
        
        std::string display = files[i];
        if (display.length() > (size_t)(max_x - 15)) {
            display = display.substr(0, max_x - 18) + "...";
        }
        
        mvwprintw(files_win, y, 5, "%s", display.c_str());
        
        wattron(files_win, COLOR_PAIR(4));
        mvwprintw(files_win, y, max_x - 10, "(%d ver)", ver_count);
        wattroff(files_win, COLOR_PAIR(4));
        
        if ((int)i == selected_file_idx && current_view == FILES_VIEW) {
            wattroff(files_win, COLOR_PAIR(2) | A_BOLD);
        }
    }
    
    wrefresh(files_win);
}

void TUIManager::draw_versions() {
    werase(versions_win);
    box(versions_win, 0, 0);
    
    wattron(versions_win, A_BOLD);
    if (!current_file.empty()) {
        mvwprintw(versions_win, 0, 2, "[ VERSIONS: %s ]", current_file.c_str());
    } else {
        mvwprintw(versions_win, 0, 2, "[ VERSIONS ]");
    }
    wattroff(versions_win, A_BOLD);
    
    if (versions.empty()) {
        wattron(versions_win, COLOR_PAIR(5));
        mvwprintw(versions_win, 2, 2, "No versions available");
        wattroff(versions_win, COLOR_PAIR(5));
        wrefresh(versions_win);
        return;
    }
    
    int max_y, max_x;
    getmaxyx(versions_win, max_y, max_x);
    
    for (size_t i = 0; i < versions.size() && (int)i < max_y - 3; i++) {
        int y = 2 + i;
        
        if ((int)i == selected_version_idx && current_view == VERSIONS_VIEW) {
            wattron(versions_win, COLOR_PAIR(2) | A_BOLD);
            mvwprintw(versions_win, y, 1, " > ");
        } else {
            mvwprintw(versions_win, y, 1, "   ");
        }
        
        std::string timestamp = format_timestamp(versions[i].timestamp);
        std::string size = format_size(versions[i].size);
        
        mvwprintw(versions_win, y, 5, "v%-3d %s  %s", 
                  versions[i].version_number,
                  timestamp.c_str(),
                  size.c_str());
        
        if ((int)i == selected_version_idx && current_view == VERSIONS_VIEW) {
            wattroff(versions_win, COLOR_PAIR(2) | A_BOLD);
        }
    }
    
    wrefresh(versions_win);
}

void TUIManager::draw_details() {
    werase(details_win);
    box(details_win, 0, 0);
    
    wattron(details_win, A_BOLD);
    mvwprintw(details_win, 0, 2, "[ DETAILS ]");
    wattroff(details_win, A_BOLD);
    
    if (versions.empty() || selected_version_idx >= (int)versions.size()) {
        wrefresh(details_win);
        return;
    }
    
    const FileVersion& ver = versions[selected_version_idx];
    
    mvwprintw(details_win, 2, 2, "Version:   %d", ver.version_number);
    mvwprintw(details_win, 3, 2, "Created:   %s", format_timestamp(ver.timestamp).c_str());
    mvwprintw(details_win, 4, 2, "Size:      %s", format_size(ver.size).c_str());
    mvwprintw(details_win, 5, 2, "Path:      %s", ver.version_path.c_str());
    
    // Try to show first few lines of content
    std::ifstream file(ver.version_path);
    if (file.is_open()) {
        mvwprintw(details_win, 7, 2, "Preview:");
        std::string line;
        int line_num = 0;
        int max_y, max_x;
        getmaxyx(details_win, max_y, max_x);
        
        while (std::getline(file, line) && line_num < max_y - 10) {
            if (line.length() > (size_t)(max_x - 6)) {
                line = line.substr(0, max_x - 9) + "...";
            }
            mvwprintw(details_win, 8 + line_num, 3, "%s", line.c_str());
            line_num++;
        }
    }
    
    wrefresh(details_win);
}

void TUIManager::draw_status(const std::string& message) {
    werase(status_win);
    wbkgd(status_win, COLOR_PAIR(1));
    
    mvwprintw(status_win, 0, 2, "%s", message.c_str());
    
    std::string help = "q:Quit | TAB:Switch | ↑↓:Navigate | ENTER:Select | r:Restore | d:Delete";
    int max_y, max_x;
    getmaxyx(status_win, max_y, max_x);
    mvwprintw(status_win, 1, 2, "%s", help.c_str());
    
    wrefresh(status_win);
}

void TUIManager::refresh_all() {
    draw_header();
    draw_files();
    draw_versions();
    draw_details();
    draw_status("Ready");
}

std::string TUIManager::format_timestamp(time_t timestamp) {
    char buf[64];
    struct tm* tm_info = localtime(&timestamp);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    return std::string(buf);
}

std::string TUIManager::format_size(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unit = 0;
    double size = bytes;
    
    while (size >= 1024 && unit < 3) {
        size /= 1024;
        unit++;
    }
    
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
    return oss.str();
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
        draw_status("✓ Version restored successfully!");
    } else {
        draw_status("✗ Failed to restore version!");
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
            current_view = FILES_VIEW;
            break;
        case 'r':
        case 'R':
            restore_version();
            break;
        case KEY_BACKSPACE:
        case KEY_LEFT:
            current_view = FILES_VIEW;
            break;
    }
}

void TUIManager::handle_input() {
    int ch = getch();
    
    if (ch == 'q' || ch == 'Q') {
        return;  // Exit
    }
    
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
    
    refresh_all();
    
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