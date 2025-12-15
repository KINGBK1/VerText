#include "version_manager.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <iostream>

using namespace std;

string VersionManager::versions_root;
string VersionManager::meta_root;

void VersionManager::init(const string& versions_dir, const string& meta_dir) {
    versions_root = versions_dir;
    meta_root = meta_dir;
    
    // Create directories if they don't exist
    mkdir(versions_root.c_str(), 0755);
    mkdir(meta_root.c_str(), 0755);
}

string VersionManager::get_version_dir(const string& backend_path) {
    string version_dir = versions_root;
    
    size_t last_slash = backend_path.find_last_of('/');
    string filename = (last_slash != string::npos) 
        ? backend_path.substr(last_slash + 1) 
        : backend_path;
    
    version_dir += "/" + filename + "_versions";
    
    return version_dir;
}

string VersionManager::get_meta_path(const string& backend_path) {
    size_t last_slash = backend_path.find_last_of('/');
    string filename = (last_slash != string::npos) 
        ? backend_path.substr(last_slash + 1) 
        : backend_path;
    
    return meta_root + "/" + filename + ".meta";
}

string VersionManager::get_version_filename(int version_num, time_t timestamp) {
    ostringstream oss;
    oss << "v" << version_num << "_" << timestamp;
    return oss.str();
}

bool VersionManager::parse_version_filename(const string& filename, int& version_num, time_t& timestamp) {
    if (filename.size() < 3 || filename[0] != 'v') return false;
    
    size_t underscore = filename.find('_');
    if (underscore == string::npos) return false;
    
    try {
        version_num = stoi(filename.substr(1, underscore - 1));
        timestamp = stol(filename.substr(underscore + 1));
        return true;
    } catch (...) {
        return false;
    }
}

bool VersionManager::create_version(const string& backend_path) {
    struct stat st;
    if (stat(backend_path.c_str(), &st) != 0) {
        return false;
    }
    
    string version_dir = get_version_dir(backend_path);
    mkdir(version_dir.c_str(), 0755);
    
    int version_count = get_version_count(backend_path);
    int new_version = version_count + 1;
    
    time_t now = time(nullptr);
    string version_filename = get_version_filename(new_version, now);
    string version_path = version_dir + "/" + version_filename;
    
    ifstream src(backend_path, ios::binary);
    ofstream dst(version_path, ios::binary);
    
    if (!src || !dst) {
        cerr << "[VFS] ✗ Failed to create version: " << version_path << endl;
        return false;
    }
    
    dst << src.rdbuf();
    src.close();
    dst.close();
    
    vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    
    FileVersion new_ver;
    new_ver.version_path = version_path;
    new_ver.timestamp = now;
    new_ver.size = st.st_size;
    new_ver.version_number = new_version;
    
    versions.push_back(new_ver);
    save_metadata(backend_path, versions);
    
    cout << "[VFS] ✓ Version " << new_version << " created for " << backend_path << endl;
    
    return true;
}

vector<FileVersion> VersionManager::get_versions(const string& backend_path) {
    vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    return versions;
}

int VersionManager::get_version_count(const string& backend_path) {
    vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    return versions.size();
}

bool VersionManager::restore_version(const string& backend_path, int version_number) {
    vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    
    for (const auto& ver : versions) {
        if (ver.version_number == version_number) {
            ifstream src(ver.version_path, ios::binary);
            ofstream dst(backend_path, ios::binary | ios::trunc);
            
            if (!src || !dst) return false;
            
            dst << src.rdbuf();
            cout << "[VFS] ✓ Restored version " << version_number << " to " << backend_path << endl;
            return true;
        }
    }
    
    return false;
}

void VersionManager::cleanup_old_versions(const string& backend_path, int keep_count) {
    vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    
    if ((int)versions.size() <= keep_count) return;
    
    sort(versions.begin(), versions.end(), 
        [](const FileVersion& a, const FileVersion& b) {
            return a.timestamp < b.timestamp;
        });
    
    int to_delete = versions.size() - keep_count;
    for (int i = 0; i < to_delete; i++) {
        unlink(versions[i].version_path.c_str());
    }
    
    versions.erase(versions.begin(), versions.begin() + to_delete);
    save_metadata(backend_path, versions);
}

void VersionManager::load_metadata(const string& backend_path, vector<FileVersion>& versions) {
    versions.clear();
    
    string meta_path = get_meta_path(backend_path);
    ifstream meta_file(meta_path);
    
    if (!meta_file) return;
    
    string line;
    while (getline(meta_file, line)) {
        istringstream iss(line);
        string part;
        vector<string> parts;
        
        while (getline(iss, part, '|')) {
            parts.push_back(part);
        }
        
        if (parts.size() == 4) {
            FileVersion ver;
            ver.version_number = stoi(parts[0]);
            ver.timestamp = stol(parts[1]);
            ver.size = stoull(parts[2]);
            ver.version_path = parts[3];
            versions.push_back(ver);
        }
    }
}

void VersionManager::save_metadata(const string& backend_path, const vector<FileVersion>& versions) {
    string meta_path = get_meta_path(backend_path);
    ofstream meta_file(meta_path, ios::trunc);
    
    if (!meta_file) {
        cerr << "[VFS] ✗ Failed to save metadata: " << meta_path << endl;
        return;
    }
    
    for (const auto& ver : versions) {
        meta_file << ver.version_number << "|" 
                  << ver.timestamp << "|" 
                  << ver.size << "|" 
                  << ver.version_path << "\n";
    }
}