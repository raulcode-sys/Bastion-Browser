#include <gtk/gtk.h>
#include <iostream>
#include <memory>
#include <cstring>
#include "browser_window.h"
#include "privacy_engine.h"
#include "app_state.h"

// ─────────────────────────────────────────────────────────────────────────────
//  Command-line proxy configuration
//  Usage:
//    bastion                   → Tor mode (default)
//    bastion --proxy=tor       → Tor SOCKS5 on 127.0.0.1:9050
//    bastion --proxy=i2p       → I2P HTTP proxy on 127.0.0.1:4444
//    bastion --proxy=none      → No proxy (direct)
//    bastion --proxy=socks5://user:pass@host:port  → Custom SOCKS5
//    bastion --proxy=chain://host:port → Tor + custom SOCKS5
// ─────────────────────────────────────────────────────────────────────────────
static ProxyConfig parse_proxy_args(int argc, char** argv) {
    ProxyConfig cfg;
    cfg.mode = ProxyMode::TOR; // default: Tor

    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg.rfind("--proxy=", 0) != 0) continue;
        std::string val = arg.substr(8);

        if (val == "tor") {
            cfg.mode = ProxyMode::TOR;

        } else if (val == "i2p") {
            cfg.mode = ProxyMode::I2P;

        } else if (val == "none" || val == "direct") {
            cfg.mode = ProxyMode::NONE;

        } else if (val.rfind("socks5://", 0) == 0) {
            cfg.mode = ProxyMode::SOCKS5;
            // Parse socks5://[user:pass@]host:port
            std::string rest = val.substr(9);
            auto at = rest.rfind('@');
            if (at != std::string::npos) {
                std::string creds = rest.substr(0, at);
                rest = rest.substr(at + 1);
                auto colon = creds.find(':');
                if (colon != std::string::npos) {
                    cfg.socks_user = creds.substr(0, colon);
                    cfg.socks_pass = creds.substr(colon + 1);
                } else {
                    cfg.socks_user = creds;
                }
            }
            auto colon = rest.rfind(':');
            if (colon != std::string::npos) {
                cfg.socks_host = rest.substr(0, colon);
                cfg.socks_port = std::stoi(rest.substr(colon + 1));
            }

        } else if (val.rfind("chain://", 0) == 0) {
            cfg.mode = ProxyMode::CHAIN_TOR;
            std::string rest = val.substr(8);
            auto colon = rest.rfind(':');
            if (colon != std::string::npos) {
                cfg.socks_host = rest.substr(0, colon);
                cfg.socks_port = std::stoi(rest.substr(colon + 1));
            }
        }
    }
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
struct AppData {
    std::shared_ptr<PrivacyEngine>  engine;
    std::shared_ptr<AppState>       state;
    std::unique_ptr<BrowserWindow>  window;
};

static void on_activate(GtkApplication* app, gpointer user_data) {
    auto* data = static_cast<AppData*>(user_data);

    if (data->window) {
        gtk_window_present(GTK_WINDOW(data->window->widget()));
        return;
    }

    data->window = std::make_unique<BrowserWindow>(app, data->engine, data->state);
}

// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    // Print startup banner
    std::cout << "\n"
              << "  ██████╗  █████╗ ███████╗████████╗██╗ ██████╗ ███╗   ██╗\n"
              << "  ██╔══██╗██╔══██╗██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║\n"
              << "  ██████╔╝███████║███████╗   ██║   ██║██║   ██║██╔██╗ ██║\n"
              << "  ██╔══██╗██╔══██║╚════██║   ██║   ██║██║   ██║██║╚██╗██║\n"
              << "  ██████╔╝██║  ██║███████║   ██║   ██║╚██████╔╝██║ ╚████║\n"
              << "  ╚═════╝ ╚═╝  ╚═╝╚══════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝\n"
              << "  BROWSER — Privacy-first. Ephemeral. Hardened.\n\n";

    ProxyConfig proxy_cfg = parse_proxy_args(argc, argv);

    std::cout << "[Bastion] Starting with proxy: ";
    switch (proxy_cfg.mode) {
        case ProxyMode::TOR:       std::cout << "Tor\n";          break;
        case ProxyMode::I2P:       std::cout << "I2P\n";          break;
        case ProxyMode::SOCKS5:    std::cout << "Custom SOCKS5\n"; break;
        case ProxyMode::CHAIN_TOR: std::cout << "Tor + SOCKS5 chain\n"; break;
        case ProxyMode::NONE:      std::cout << "None (direct)\n"; break;
    }

    AppData data;
    data.engine = std::make_shared<PrivacyEngine>(proxy_cfg);
    data.state  = std::make_shared<AppState>();

    GtkApplication* app = gtk_application_new(
        "io.bastion.browser",
        G_APPLICATION_DEFAULT_FLAGS);

    g_signal_connect(app, "activate", G_CALLBACK(on_activate), &data);

    // GTK processes only its own args; pass argc/argv so GTK can consume --gtk-*
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
