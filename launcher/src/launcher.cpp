#include <string>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include "include/portable-file-dialogs.h"
#include "include/ArduinoJson.h"
#include "include/picosha2.h"

#ifdef _WIN32
#include <Windows.h>
#endif

using namespace std;
namespace fs = std::filesystem;
namespace sha = picosha2;

constexpr char DIRSEP = fs::path::preferred_separator;

#ifdef _WIN32
  constexpr char ENVSEP = ';';
  const string PLATFORM = "windows";
#elif defined(__APPLE__)
  constexpr char ENVSEP = ':';
  const string PLATFORM = "macos";
#elif defined(__linux__)
  constexpr char ENVSEP = ':';
  const string PLATFORM = "linux";
#endif

#ifdef _WIN32
const vector<string> browsers = {
  "chrome.exe",
  "vivaldi.exe",
  "msedge.exe",
};
#else
const vector<string> browsers = {
  "chromium-browser",
  "google-chrome",
  "vivaldi",
  "microsoft-edge-stable",
};
#endif

vector<string> split(const string& s, const char delim) {
  stringstream ss(s);
  string item;
  vector<string> elems;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

string find_browser(const string& app_title) {
  cout << "searching for supported browser" << endl;
  auto paths = split(getenv("PATH"), ENVSEP);
  string result;
  for (auto path : paths) {
    for (auto browser : browsers) {
      auto bin = path + DIRSEP + browser;
      if (fs::exists(bin)) {
        result = bin;
        break;
      }
    }
  }
  if (result.empty()) {
    pfd::message(app_title, "Could not find a compatible Chromium-based browser.",
      pfd::choice::ok, pfd::icon::error);
    exit(1);
  }
  return result;
}

fs::path get_data_dir(const string& app_name, const bool development) {
#ifdef _WIN32
  auto data_dir = fs::path(getenv("APPDATA")) / app_name;
#else
  auto data_dir = fs::path(getenv("HOME")) / ".config" / app_name;
#endif
  if (development) {
    data_dir = data_dir / "development";
  }
  return data_dir;
}

DynamicJsonDocument get_app_config(const fs::path& app_dir) {
  const auto path = app_dir / "calory.json";
  if (!fs::exists(path)) {
    pfd::message("Error", "Could not find application configuration.",
      pfd::choice::ok, pfd::icon::error);
    exit(1);
  }

  ifstream stream(path);
  stringstream buffer;
  buffer << stream.rdbuf();
  const auto json = buffer.str();
  DynamicJsonDocument doc(4000);
  const auto error = deserializeJson(doc, json);
  return doc;
}

string get_extension_hash(const string& path) {
  vector<unsigned char> hash(sha::k_digest_size);
  sha::hash256(path.begin(), path.end(), hash.begin(), hash.end());
  string hex = sha::bytes_to_hex_string(hash.begin(), hash.end());
  hex.resize(32);
  string result;
  for (char c : hex) {
    if (c >= 'a') {
      result += (char)(c + 10);
    }
    else {
      result += (char)('a' + c - '0');
    }
  }
  return result;
}

#ifdef _WIN32
void write_host_registry_key(const string& app_name, const fs::path& host_path) {
  HKEY hKey;
  LPCTSTR lpSubKey = "SOFTWARE\\Google\\Chrome\\NativeMessagingHosts\\" + app_name + "_host";
  RegOpenKeyEx(HKEY_CURRENT_USER, lpSubKey, 0, KEY_WRITE, &hKey);
  LPCTSTR lpData = host_path.c_str();
  RegSetValueEx(hKey, NULL, 0, REG_SZ, (const BYTE *)lpData, strlen(lpData) + 1);
  RegCloseKey(hKey);
}
#endif

void write_bridge_manifest(
  const string& app_name,
  const string& app_version,
  const fs::path& bridge_path
) {
  cout << "creating bridge manifest" << endl;
  DynamicJsonDocument json(2000);
  json["manifest_version"] = 3;
  json["name"] = app_name + "_bridge";
  json["version"] = app_version;
  auto background = json.createNestedObject("background");
  background["service_worker"] = "lib/worker.js";
  background["type"] = "module";
  auto content_scripts = json.createNestedArray("content_scripts");
  auto content_script = content_scripts.createNestedObject();
  auto matches = content_script.createNestedArray("matches");
  matches.add("http://*/*");
  matches.add("file:///*");
  auto js = content_script.createNestedArray("js");
  js.add("lib/content.js");
  auto permissions = json.createNestedArray("permissions");
  permissions.add("nativeMessaging");
  permissions.add("storage");
  permissions.add("unlimitedStorage");
  permissions.add("clipboardRead");
  permissions.add("clipboardWrite");
  permissions.add("geolocation"); 
  auto host_permissions = json.createNestedArray("host_permissions");
  host_permissions.add("http://*/");

  cout << "writing bridge manifest" << endl;
  ofstream outfile(bridge_path / "manifest.json");
  outfile << json.as<string>();
  outfile.close();
}

void write_host_manifest(
  const string& app_name,
  const fs::path& host_path,
  const fs::path& manifest_path,
  const fs::path& bridge_path
) {
  cout << "creating host manifest" << endl;
  DynamicJsonDocument json(2000);
  json["path"] = (string)host_path;
  json["name"] = app_name + "_host";
  json["description"] = "Calory Native Messaging Host";
  json["type"] = "stdio";
  auto allowed_origins = json.createNestedArray("allowed_origins");
  auto actual_bridge_path = fs::canonical(bridge_path);
  auto extension_id = get_extension_hash(actual_bridge_path);
  auto origin = "chrome-extension://" + extension_id + "/";
  allowed_origins.add(origin);

  cout << "writing host manifest" << endl;
  fs::create_directories(manifest_path.parent_path());
  ofstream outfile(manifest_path);
  outfile << json.as<string>();
  outfile.close();

#ifdef _WIN32
  write_host_registry_key(app_name, host_path);
#endif
}

int main() {
  const auto development = getenv("CALORY_ENV") == "development";
  const auto app_dir = fs::current_path();
  const auto app_config = get_app_config(app_dir);
  const auto app_name = (string)app_config["name"];
  const auto app_title = (string)app_config["title"];
  const auto app_version = (string)app_config["version"];
  const auto host_path = app_dir / (string)app_config["host"][PLATFORM];
  const auto browser_path = find_browser(app_title);
  const auto browser = fs::path(browser_path).filename();
  const auto data_dir = get_data_dir(app_name, development);

  const auto bridge_path = development
    ? app_dir / "host" / "node_modules" / "@calory" / "bridge"
    : app_dir / "bridge";  
  const auto host_manifest_path = data_dir / browser
    / "NativeMessagingHosts" / (app_name + "_host.json");

  cout << "development: " << (development ? "true" : "false") << endl;
  cout << "app_name: " << app_name << endl;
  cout << "host_path: " << host_path << endl;
  cout << "browser: " << browser << endl;
  cout << "browser_path: " << browser_path << endl;
  cout << "data_dir: " << data_dir << endl;
  cout << "bridge_path: " << bridge_path << endl;
  cout << "host_manifest_path: " << host_manifest_path << endl;

  write_bridge_manifest(app_name, app_version, bridge_path);
  write_host_manifest(app_name, host_path, host_manifest_path, bridge_path);

  string command;
  if (development) {
    command = browser_path
      + " --load-extension=" + bridge_path.c_str()
      + " --user-data-dir=\"" + (data_dir / browser).c_str() + "\""
      + " --app=\"http://localhost:3000\""
      + " --auto-open-devtools-for-tabs"
      + " --enable-logging=stderr"
    ;

    fs::remove_all(data_dir / browser / "Default" / "Service Worker");
  }
  else {
    command = browser_path
      + " --load-extension=" + bridge_path.c_str()
      + " --user-data-dir=\"" + (data_dir / browser).c_str() + "\""
      + " --app=\"file://" + (app_dir / "app" / "dist" / "index.html").c_str() + "\""
      + " --allow-file-access-from-files"
    ;
  }

  cout << "launching browser" << endl;
  system(command.c_str());
}
