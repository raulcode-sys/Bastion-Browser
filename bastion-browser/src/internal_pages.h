#pragma once
#include <string>
#include "app_state.h"

class DownloadManager;

namespace internal_pages {

// Build each page's HTML given current app state
std::string home     (const AppState& s);
std::string settings (const AppState& s);
std::string bookmarks(const AppState& s);
std::string downloads(const AppState& s, const DownloadManager& dm);
std::string blocked  (const std::string& url);
std::string not_found(const std::string& path);

} // namespace internal_pages
