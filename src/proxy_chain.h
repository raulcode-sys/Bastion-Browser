#pragma once
#include <string>
#include <vector>
#include <webkit2/webkit2.h>

// ─────────────────────────────────────────────────────────────────────────────
//  ProxyMode
//  NONE        : direct (no proxy)
//  TOR         : route via local Tor daemon SOCKS5 (127.0.0.1:9050)
//  I2P         : route via local I2P HTTP proxy (127.0.0.1:4444)
//  SOCKS5      : custom SOCKS5 upstream
//  CHAIN_TOR   : Tor + custom SOCKS5 upstream (Tor over SOCKS5 chain)
// ─────────────────────────────────────────────────────────────────────────────

enum class ProxyMode { NONE, TOR, I2P, SOCKS5, CHAIN_TOR };

struct ProxyConfig {
    ProxyMode mode        = ProxyMode::TOR;
    std::string tor_host  = "127.0.0.1";
    int         tor_port  = 9050;
    std::string i2p_host  = "127.0.0.1";
    int         i2p_port  = 4444;   // I2P HTTP proxy
    // Custom SOCKS5 (used by SOCKS5 and CHAIN_TOR modes)
    std::string socks_host;
    int         socks_port = 0;
    std::string socks_user;
    std::string socks_pass;
};

class ProxyChain {
public:
    explicit ProxyChain(const ProxyConfig& cfg);

    // Apply proxy settings to a WebKit web context.
    void apply(WebKitWebContext* ctx) const;

    // Human-readable description of the active proxy.
    std::string description() const;

    ProxyMode   mode()       const { return cfg_.mode; }
    bool        is_active()  const { return cfg_.mode != ProxyMode::NONE; }

private:
    ProxyConfig cfg_;
    std::string build_proxy_uri() const;
};
