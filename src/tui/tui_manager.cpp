#include "tui_manager.h"
#include "../fuse/version_manager.h"
#include <dirent.h>
#include <sys/stat.h>
#include <ctime>
#include <fstream>
#include <algorithm>

using namespace std;

TUIManager::TUIManager() : main_win(nullptr), header_win(nullptr), files_win(nullptr),
    versions_win(nullptr), details_win(nullptr), status_win(nullptr),
    selected_file_idx(0), selected_version_idx(0), current_view(FILES_VIEW) {
    char* env_root = getenv("VFS_BACKEND_ROOT");
    backend_root = env_root ? string(env_root) : "./runtime/data";
}

TUIManager::~TUIManager() { cleanup(); }

void TUIManager::init_ncurses() {
    initscr(); cbreak(); noecho(); keypad(stdscr, TRUE); curs_set(0);
    if (has_colors()) {
        start_color(); use_default_colors();
        init_pair(1, COLOR_CYAN, -1); init_pair(2, COLOR_BLACK, COLOR_WHITE);
        init_pair(3, COLOR_YELLOW, -1); init_pair(4, COLOR_GREEN, -1);
        init_pair(5, COLOR_RED, -1); init_pair(6, COLOR_BLUE, -1);
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
        string fullpath = backend_root + "/" + entry->d_name;
        struct stat st;
        if (stat(fullpath.c_str(), &st) == 0 && S_ISREG(st.st_mode))
            files.push_back(entry->d_name);
    }
    closedir(dir);
    sort(files.begin(), files.end());
}

void TUIManager::load_versions_for_file(const string& filename) {
    versions = VersionManager::get_versions(backend_root + "/" + filename);
    sort(versions.begin(), versions.end(), [](const FileVersion& a, const FileVersion& b) {
        return a.version_number > b.version_number;
    });
    selected_version_idx = 0;
}

string TUIManager::get_full_path(const string& filename) {
    return backend_root + "/" + filename;
}

void TUIManager::draw_header() {
    werase(header_win);
    wattron(header_win, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(header_win, 0, 2, "VFS Manager");
    wattroff(header_win, COLOR_PAIR(1) | A_BOLD);
    wattron(header_win, COLOR_PAIR(6));
    mvwprintw(header_win, 0, getmaxx(header_win) - 35, "Q:Quit H:Help TAB:Switch ↑↓:Nav");
    wattroff(header_win, COLOR_PAIR(6));
    wnoutrefresh(header_win);
}

void TUIManager::draw_files() {
    werase(files_win);
    int max_y = getmaxy(files_win), max_x = getmaxx(files_win);
    
    if (current_view == FILES_VIEW) {
        wattron(files_win, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(files_win, 0, 1, "FILES (%zu)", files.size());
        wattroff(files_win, COLOR_PAIR(4) | A_BOLD);
    } else {
        wattron(files_win, COLOR_PAIR(6));
        mvwprintw(files_win, 0, 1, "FILES (%zu)", files.size());
        wattroff(files_win, COLOR_PAIR(6));
    }
    mvwhline(files_win, 1, 0, ACS_HLINE, max_x);
    
    if (files.empty()) {
        mvwprintw(files_win, max_y/2, (max_x-12)/2, "No files");
        wnoutrefresh(files_win);
        return;
    }
    
    for (size_t i = 0; i < files.size() && (int)i < max_y - 2; i++) {
        bool sel = ((int)i == selected_file_idx && current_view == FILES_VIEW);
        if (sel) wattron(files_win, COLOR_PAIR(2) | A_BOLD);
        
        string name = files[i];
        if (name.length() > (size_t)(max_x - 10)) name = name.substr(0, max_x - 13) + "...";
        int ver = VersionManager::get_version_count(backend_root + "/" + files[i]);
        mvwprintw(files_win, i + 2, 1, "%c %-*s v%d", sel ? '>' : ' ', max_x - 8, name.c_str(), ver);
        
        if (sel) wattroff(files_win, COLOR_PAIR(2) | A_BOLD);
    }
    wnoutrefresh(files_win);
}

void TUIManager::draw_versions() {
    werase(versions_win);
    int max_y = getmaxy(versions_win), max_x = getmaxx(versions_win);
    
    if (current_view == VERSIONS_VIEW) {
        wattron(versions_win, COLOR_PAIR(4) | A_BOLD);
        mvwprintw(versions_win, 0, 1, "VERSIONS");
        wattroff(versions_win, COLOR_PAIR(4) | A_BOLD);
    } else {
        wattron(versions_win, COLOR_PAIR(6));
        mvwprintw(versions_win, 0, 1, "VERSIONS");
        wattroff(versions_win, COLOR_PAIR(6));
    }
    
    if (!current_file.empty()) {
        wattron(versions_win, COLOR_PAIR(6));
        string fn = current_file.length() > (size_t)(max_x - 3) ? 
            current_file.substr(0, max_x - 6) + "..." : current_file;
        mvwprintw(versions_win, 0, max_x - fn.length() - 2, "%s", fn.c_str());
        wattroff(versions_win, COLOR_PAIR(6));
    }
    mvwhline(versions_win, 1, 0, ACS_HLINE, max_x);
    
    if (versions.empty()) {
        mvwprintw(versions_win, max_y/2, (max_x-14)/2, "No versions");
        wnoutrefresh(versions_win);
        return;
    }
    
    for (size_t i = 0; i < versions.size() && (int)i < max_y - 2; i++) {
        bool sel = ((int)i == selected_version_idx && current_view == VERSIONS_VIEW);
        if (sel) wattron(versions_win, COLOR_PAIR(2) | A_BOLD);
        
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%m/%d %H:%M", localtime(&versions[i].timestamp));
        mvwprintw(versions_win, i + 2, 1, "%c v%-2d %s %6s", 
            sel ? '>' : ' ', versions[i].version_number, time_str, 
            format_size(versions[i].size).c_str());
        
        if (sel) wattroff(versions_win, COLOR_PAIR(2) | A_BOLD);
    }
    wnoutrefresh(versions_win);
}

void TUIManager::draw_status(const string& msg) {
    werase(status_win);
    mvwhline(status_win, 0, 0, ACS_HLINE, getmaxx(status_win));
    if (!msg.empty()) {
        wattron(status_win, COLOR_PAIR(4));
        mvwprintw(status_win, 1, 2, "%s", msg.c_str());
        wattroff(status_win, COLOR_PAIR(4));
    }
    wnoutrefresh(status_win);
}

void TUIManager::refresh_all() {
    draw_header(); draw_files(); draw_versions(); draw_status(last_status_message);
    doupdate();
}

string TUIManager::format_size(size_t b) {
    if (b < 1024) return to_string(b) + "B";
    if (b < 1048576) return to_string(b/1024) + "K";
    return to_string(b/1048576) + "M";
}

string TUIManager::format_timestamp(time_t t) {
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", localtime(&t));
    return buf;
}

void TUIManager::select_file() {
    if (files.empty() || selected_file_idx >= (int)files.size()) return;
    current_file = files[selected_file_idx];
    load_versions_for_file(current_file);
    current_view = VERSIONS_VIEW;
    last_status_message = to_string(versions.size()) + " versions loaded";
}

void TUIManager::restore_version() {
    if (versions.empty() || selected_version_idx >= (int)versions.size()) return;
    int ver = versions[selected_version_idx].version_number;
    if (VersionManager::restore_version(backend_root + "/" + current_file, ver)) {
        last_status_message = "✓ Restored v" + to_string(ver);
        load_versions_for_file(current_file);
    } else {
        last_status_message = "✗ Restore failed";
    }
}

void TUIManager::draw_help_popup() {
    int h = 11, w = 40;
    WINDOW* help = newwin(h, w, (LINES - h) / 2, (COLS - w) / 2);
    box(help, 0, 0);
    wattron(help, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(help, 0, (w - 6) / 2, " HELP ");
    wattroff(help, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(help, 2, 2, "↑/↓      Navigate");
    mvwprintw(help, 3, 2, "TAB      Switch panel");
    mvwprintw(help, 4, 2, "ENTER    Select file");
    mvwprintw(help, 5, 2, "R        Restore version");
    mvwprintw(help, 6, 2, "V        View content");
    mvwprintw(help, 7, 2, "Q        Quit");
    wattron(help, COLOR_PAIR(6));
    mvwprintw(help, 9, (w - 16) / 2, "Press any key");
    wattroff(help, COLOR_PAIR(6));
    wrefresh(help);
    nodelay(stdscr, FALSE); getch(); nodelay(stdscr, TRUE);
    delwin(help);
}

void TUIManager::view_version_content() {
    if (versions.empty() || selected_version_idx >= (int)versions.size()) return;
    int h = LINES - 4, w = COLS - 4;
    WINDOW* view = newwin(h, w, 2, 2);
    box(view, 0, 0);
    const FileVersion& ver = versions[selected_version_idx];
    wattron(view, COLOR_PAIR(1) | A_BOLD);
    mvwprintw(view, 0, 2, " v%d - %s ", ver.version_number, current_file.c_str());
    wattroff(view, COLOR_PAIR(1) | A_BOLD);
    ifstream file(ver.version_path);
    int line = 2;
    if (file) {
        string content;
        while (getline(file, content) && line < h - 2) {
            if (content.length() > (size_t)(w - 4)) content = content.substr(0, w - 7) + "...";
            mvwprintw(view, line++, 2, "%s", content.c_str());
        }
    } else {
        mvwprintw(view, h/2, (w-15)/2, "Cannot read file");
    }
    mvwprintw(view, h - 1, (w - 16) / 2, "Press any key");
    wrefresh(view);
    nodelay(stdscr, FALSE); getch(); nodelay(stdscr, TRUE);
    delwin(view);
}

void TUIManager::handle_files_input(int ch) {
    if (ch == KEY_UP && selected_file_idx > 0) selected_file_idx--;
    else if (ch == KEY_DOWN && selected_file_idx < (int)files.size() - 1) selected_file_idx++;
    else if (ch == '\n' || ch == KEY_ENTER) select_file();
    else if (ch == '\t' && !versions.empty()) current_view = VERSIONS_VIEW;
}

void TUIManager::handle_versions_input(int ch) {
    if (ch == KEY_UP && selected_version_idx > 0) selected_version_idx--;
    else if (ch == KEY_DOWN && selected_version_idx < (int)versions.size() - 1) selected_version_idx++;
    else if (ch == '\t') current_view = FILES_VIEW;
    else if (ch == 'r' || ch == 'R') restore_version();
    else if (ch == 'v' || ch == 'V') view_version_content();
}

void TUIManager::handle_input() {
    int ch = getch();
    if (ch != ERR) {
        if (current_view == FILES_VIEW) handle_files_input(ch);
        else handle_versions_input(ch);
    }
}

void TUIManager::run() {
    init_ncurses(); create_windows(); load_files();
    if (!files.empty()) { current_file = files[0]; load_versions_for_file(current_file); }
    last_status_message = "Press H for help";
    nodelay(stdscr, TRUE);
    bool redraw = true;
    while (true) {
        if (redraw) { refresh_all(); redraw = false; }
        int ch = getch();
        if (ch == ERR) { napms(50); continue; }
        if (ch == 'q' || ch == 'Q') break;
        redraw = true;
        if (ch == 'h' || ch == 'H') { draw_help_popup(); continue; }
        if (current_view == FILES_VIEW) handle_files_input(ch);
        else handle_versions_input(ch);
    }
    cleanup();
}

void TUIManager::draw_details() {}