#include "version_manager.h"
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <iostream>

std::string VersionManager::versions_root;
std::string VersionManager::meta_root;

void VersionManager::init(const std::string& versions_dir, const std::string& meta_dir) {
    versions_root = versions_dir;
    meta_root = meta_dir;
    
    // Create directories if they don't exist
    mkdir(versions_root.c_str(), 0755);
    mkdir(meta_root.c_str(), 0755);
}

std::string VersionManager::get_version_dir(const std::string& backend_path) {
    // Convert /path/to/file.txt -> versions/path/to/file.txt/
    std::string relative = backend_path;
    
    // Create a sanitized path
    std::string version_dir = versions_root;
    
    // Extract just the filename for simpler structure
    size_t last_slash = backend_path.find_last_of('/');
    std::string filename = (last_slash != std::string::npos) 
        ? backend_path.substr(last_slash + 1) 
        : backend_path;
    
    // Hash or use full path - for now, use filename
    version_dir += "/" + filename + "_versions";
    
    return version_dir;
}

std::string VersionManager::get_meta_path(const std::string& backend_path) {
    size_t last_slash = backend_path.find_last_of('/');
    std::string filename = (last_slash != std::string::npos) 
        ? backend_path.substr(last_slash + 1) 
        : backend_path;
    
    return meta_root + "/" + filename + ".meta";
}

std::string VersionManager::get_version_filename(int version_num, time_t timestamp) {
    std::ostringstream oss;
    oss << "v" << version_num << "_" << timestamp;
    return oss.str();
}

bool VersionManager::parse_version_filename(const std::string& filename, int& version_num, time_t& timestamp) {
    // Parse: v1_1234567890
    if (filename.size() < 3 || filename[0] != 'v') return false;
    
    size_t underscore = filename.find('_');
    if (underscore == std::string::npos) return false;
    
    try {
        version_num = std::stoi(filename.substr(1, underscore - 1));
        timestamp = std::stol(filename.substr(underscore + 1));
        return true;
    } catch (...) {
        return false;
    }
}

bool VersionManager::create_version(const std::string& backend_path) {
    // Check if file exists
    struct stat st;
    if (stat(backend_path.c_str(), &st) != 0) {
        return false; // File doesn't exist, no need to version
    }
    
    // Get version directory
    std::string version_dir = get_version_dir(backend_path);
    
    // Create version directory if it doesn't exist
    mkdir(version_dir.c_str(), 0755);
    
    // Get current version count
    int version_count = get_version_count(backend_path);
    int new_version = version_count + 1;
    
    // Create version filename with timestamp
    time_t now = time(nullptr);
    std::string version_filename = get_version_filename(new_version, now);
    std::string version_path = version_dir + "/" + version_filename;
    
    // Copy the current file to version storage
    std::ifstream src(backend_path, std::ios::binary);
    std::ofstream dst(version_path, std::ios::binary);
    
    if (!src || !dst) {
        std::cerr << "[VFS] Failed to create version: " << version_path << std::endl;
        return false;
    }
    
    dst << src.rdbuf();
    src.close();
    dst.close();
    
    // Update metadata
    std::vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    
    FileVersion new_ver;
    new_ver.version_path = version_path;
    new_ver.timestamp = now;
    new_ver.size = st.st_size;
    new_ver.version_number = new_version;
    
    versions.push_back(new_ver);
    save_metadata(backend_path, versions);
    
    std::cout << "[VFS] Created version " << new_version << " for " << backend_path << std::endl;
    
    return true;
}

std::vector<FileVersion> VersionManager::get_versions(const std::string& backend_path) {
    std::vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    return versions;
}

int VersionManager::get_version_count(const std::string& backend_path) {
    std::vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    return versions.size();
}

bool VersionManager::restore_version(const std::string& backend_path, int version_number) {
    std::vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    
    // Find the version
    for (const auto& ver : versions) {
        if (ver.version_number == version_number) {
            // Copy version back to main file
            std::ifstream src(ver.version_path, std::ios::binary);
            std::ofstream dst(backend_path, std::ios::binary | std::ios::trunc);
            
            if (!src || !dst) return false;
            
            dst << src.rdbuf();
            std::cout << "[VFS] Restored version " << version_number << " to " << backend_path << std::endl;
            return true;
        }
    }
    
    return false;
}

void VersionManager::cleanup_old_versions(const std::string& backend_path, int keep_count) {
    std::vector<FileVersion> versions;
    load_metadata(backend_path, versions);
    
    if ((int)versions.size() <= keep_count) return;
    
    // Sort by timestamp (oldest first)
    std::sort(versions.begin(), versions.end(), 
        [](const FileVersion& a, const FileVersion& b) {
            return a.timestamp < b.timestamp;
        });
    
    // Delete old versions
    int to_delete = versions.size() - keep_count;
    for (int i = 0; i < to_delete; i++) {
        unlink(versions[i].version_path.c_str());
    }
    
    // Keep only recent versions in metadata
    versions.erase(versions.begin(), versions.begin() + to_delete);
    save_metadata(backend_path, versions);
}

void VersionManager::load_metadata(const std::string& backend_path, std::vector<FileVersion>& versions) {
    versions.clear();
    
    std::string meta_path = get_meta_path(backend_path);
    std::ifstream meta_file(meta_path);
    
    if (!meta_file) return; // No metadata yet
    
    std::string line;
    while (std::getline(meta_file, line)) {
        // Format: version_number|timestamp|size|path
        std::istringstream iss(line);
        std::string part;
        std::vector<std::string> parts;
        
        while (std::getline(iss, part, '|')) {
            parts.push_back(part);
        }
        
        if (parts.size() == 4) {
            FileVersion ver;
            ver.version_number = std::stoi(parts[0]);
            ver.timestamp = std::stol(parts[1]);
            ver.size = std::stoull(parts[2]);
            ver.version_path = parts[3];
            versions.push_back(ver);
        }
    }
}

void VersionManager::save_metadata(const std::string& backend_path, const std::vector<FileVersion>& versions) {
    std::string meta_path = get_meta_path(backend_path);
    std::ofstream meta_file(meta_path, std::ios::trunc);
    
    if (!meta_file) {
        std::cerr << "[VFS] Failed to save metadata: " << meta_path << std::endl;
        return;
    }
    
    for (const auto& ver : versions) {
        meta_file << ver.version_number << "|" 
                  << ver.timestamp << "|" 
                  << ver.size << "|" 
                  << ver.version_path << "\n";
    }
}