#pragma once

#include <string>
#include <vector>
#include <ctime>

struct FileVersion {
    std::string version_path;  // Path to the versioned file
    time_t timestamp;          // When this version was created
    size_t size;               // File size
    int version_number;        // Version number (1, 2, 3, ...)
};

class VersionManager {
public:
    // Initialize versioning system
    static void init(const std::string& versions_dir, const std::string& meta_dir);
    
    // Create a new version of a file before it's modified
    // Returns true if version was created successfully
    static bool create_version(const std::string& backend_path);
    
    // Get all versions of a file
    static std::vector<FileVersion> get_versions(const std::string& backend_path);
    
    // Restore a specific version
    static bool restore_version(const std::string& backend_path, int version_number);
    
    // Get version count for a file
    static int get_version_count(const std::string& backend_path);
    
    // Delete old versions (keep only last N versions)
    static void cleanup_old_versions(const std::string& backend_path, int keep_count);

private:
    static std::string versions_root;
    static std::string meta_root;
    
    // Helper: Get version directory for a file
    static std::string get_version_dir(const std::string& backend_path);
    
    // Helper: Get metadata file path
    static std::string get_meta_path(const std::string& backend_path);
    
    // Helper: Generate version filename
    static std::string get_version_filename(int version_num, time_t timestamp);
    
    // Helper: Parse version filename to get number and timestamp
    static bool parse_version_filename(const std::string& filename, int& version_num, time_t& timestamp);
    
    // Helper: Load metadata for a file
    static void load_metadata(const std::string& backend_path, std::vector<FileVersion>& versions);
    
    // Helper: Save metadata for a file
    static void save_metadata(const std::string& backend_path, const std::vector<FileVersion>& versions);
};