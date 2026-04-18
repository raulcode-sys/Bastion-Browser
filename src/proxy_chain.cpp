#include "proxy_chain.h"
#include <sstream>
#include <iostream>

ProxyChain::ProxyChain(const ProxyConfig& cfg) : cfg_(cfg) {}

std::string ProxyChain::build_proxy_uri() const {
    std::ostringstream uri;
    switch (cfg_.mode) {
        case ProxyMode::TOR:
            // SOCKS5 through Tor
            uri << "socks5://" << cfg_.tor_host << ":" << cfg_.tor_port;
            break;

        case ProxyMode::I2P:
            // I2P HTTP proxy
            uri << "http://" << cfg_.i2p_host << ":" << cfg_.i2p_port;
            break;

        case ProxyMode::SOCKS5:
            uri << "socks5://";
            if (!cfg_.socks_user.empty()) {
                uri << cfg_.socks_user;
                if (!cfg_.socks_pass.empty()) uri << ":" << cfg_.socks_pass;
                uri << "@";
            }
            uri << cfg_.socks_host << ":" << cfg_.socks_port;
            break;

        case ProxyMode::CHAIN_TOR:
            // In chain mode, WebKit connects to Tor first.
            // The external SOCKS5 proxy is set as Tor's upstream via torrc,
            // so WebKit only needs to know about the Tor endpoint.
            uri << "socks5://" << cfg_.tor_host << ":" << cfg_.tor_port;
            break;

        case ProxyMode::NONE:
        default:
            return {};
    }
    return uri.str();
}

void ProxyChain::apply(WebKitWebContext* ctx) const {
    // Use WebKitWebsiteDataManager (the non-deprecated path in 4.1)
    WebKitWebsiteDataManager* dm = webkit_web_context_get_website_data_manager(ctx);

    if (cfg_.mode == ProxyMode::NONE) {
        webkit_website_data_manager_set_network_proxy_settings(
            dm, WEBKIT_NETWORK_PROXY_MODE_NO_PROXY, nullptr);
        std::cout << "[Bastion] Proxy disabled (direct connection).\n";
        return;
    }

    std::string proxy_uri = build_proxy_uri();
    if (proxy_uri.empty()) return;

    // Bypass proxy for localhost
    const char* ignore_hosts[] = { "localhost", "127.0.0.1", "::1", nullptr };
    WebKitNetworkProxySettings* settings =
        webkit_network_proxy_settings_new(proxy_uri.c_str(), ignore_hosts);

    webkit_website_data_manager_set_network_proxy_settings(
        dm, WEBKIT_NETWORK_PROXY_MODE_CUSTOM, settings);

    webkit_network_proxy_settings_free(settings);

    std::cout << "[Bastion] Proxy applied: " << proxy_uri << "\n";
}

std::string ProxyChain::description() const {
    switch (cfg_.mode) {
        case ProxyMode::NONE:      return "No proxy (direct)";
        case ProxyMode::TOR:       return "Tor (SOCKS5 127.0.0.1:9050)";
        case ProxyMode::I2P:       return "I2P (HTTP 127.0.0.1:4444)";
        case ProxyMode::SOCKS5: {
            std::ostringstream s;
            s << "SOCKS5 " << cfg_.socks_host << ":" << cfg_.socks_port;
            return s.str();
        }
        case ProxyMode::CHAIN_TOR: {
            std::ostringstream s;
            s << "Tor → SOCKS5 " << cfg_.socks_host << ":" << cfg_.socks_port;
            return s.str();
        }
    }
    return "Unknown";
}
