# Bastion Browser

A private, ephemeral web browser for Linux. No history, no disk writes, Tor by default.

Built on WebKitGTK because writing a browser engine from scratch is insane and I'm not that bored.

---

## What it actually does

- **No history.** Not "private mode" — there's no history recording anywhere in the code. It's not a toggle.
- **No disk writes during browsing.** Cookies, cache, session data all live in RAM and vanish when you close the browser. Bookmarks are the one exception (opt-in, off with one click).
- **Routes through Tor by default.** SOCKS5 on `127.0.0.1:9050`. I2P, direct, custom SOCKS5, and Tor-over-SOCKS5 chains all work too.
- **DNS over HTTPS with DNSSEC.** dnscrypt-proxy handles all DNS, nothing leaks in cleartext.
- **400+ tracker domains blocked** via WebKit's native content blocker.
- **Canvas, WebGL, AudioContext, and Navigator fingerprinting all spoofed.** Every page loads with a script that poisons the fingerprinting surface before any other JS runs.
- **WebRTC can't leak your IP.** Patched to force relay-only with empty ICE servers.
- **Tracking params stripped** from every URL — `utm_*`, `fbclid`, `gclid`, `msclkid`, the whole family.
- **HTTPS-only.** Plain HTTP gets auto-upgraded. TLS errors hard-block with no "proceed anyway" button.
- **Hardened binary.** PIE, full RELRO, stack canaries, Intel CET, `_FORTIFY_SOURCE=2`.

---

## Install

### Arch / CachyOS / Manjaro / EndeavourOS

```bash
unzip bastion-browser.zip
cd bastion-browser
chmod +x installarch.sh
./installarch.sh
```

### Debian / Ubuntu / Mint / Pop!_OS

```bash
unzip bastion-browser.zip
cd bastion-browser
chmod +x installdeb.sh
./installdeb.sh
```

The install script installs build deps, sets up Tor and dnscrypt-proxy, applies some kernel sysctl hardening, compiles the browser, and drops the binary in `/usr/local/bin/bastion`. You'll get prompted for sudo once.

macOS and Windows aren't supported and probably never will be — the whole thing is built on GTK.

---

## Usage

```bash
bastion                          # Tor (default)
bastion --proxy=i2p              # I2P
bastion --proxy=none             # direct, no anonymization
bastion --proxy=socks5://host:port
bastion --proxy=chain://host:port  # Tor → SOCKS5 chain
```

### Internal pages

- `bastion://home` — search, bookmarks, session stats
- `bastion://bookmarks`
- `bastion://downloads`
- `bastion://settings` — theme, search engine, privacy toggles

### Keyboard shortcuts

| Key | Action |
|---|---|
| Ctrl+T | New tab |
| Ctrl+W | Close tab |
| Ctrl+L | Focus address bar |
| Ctrl+F | Find in page |
| Ctrl+D | Toggle bookmark |
| Ctrl+J | Downloads |
| Ctrl+, | Settings |
| Ctrl++ / Ctrl+- | Zoom |
| Ctrl+0 | Reset zoom |
| F5 | Reload |
| Alt+← / Alt+→ | Back / Forward |

---

## Themes

Three, switchable live in settings:

- **Modern Dark** (default)
- **Modern Light**
- **Retro Windows Vista Aero** — because sometimes you just want glossy blue gradients in your life

---

## Search engines

- DuckDuckGo (default)
- Brave Search
- Startpage
- SearXNG (via searx.be)

No Google. Swap in Settings.

---

## What's under the hood

```
src/
├── main.cpp              # entry point, CLI arg parsing
├── browser_window.cpp    # GTK UI, tabs, toolbar, shortcuts
├── privacy_engine.cpp    # ephemeral contexts, hardening
├── proxy_chain.cpp       # Tor / I2P / SOCKS5 routing
├── content_filter.cpp    # tracker blocklist + URL param stripping
├── download_manager.cpp  # downloads with progress tracking
├── app_state.cpp         # settings + bookmarks (the only persistent state)
├── internal_pages.cpp    # HTML for bastion:// pages
├── themes.h              # CSS for the three themes
└── spoof_script.h        # the fingerprint poisoning JS injected into every page
```

Build system is CMake + Ninja. Deps: GTK3, WebKitGTK 4.1, libsoup3, glib2, pango. Nothing exotic.

---

## What it doesn't do

I want to be upfront about this:

- **It's not magic.** If you log into your Google account through Tor, Google still knows who you are. Tor hides your IP, not your identity when you hand it over.
- **It's not a Tor Browser replacement.** Tor Browser does more — like blocking JS entirely in "Safest" mode, shipping a fixed window size to defeat viewport fingerprinting, and rotating circuits per-tab. Bastion is a privacy-hardened general-purpose browser, not an anonymity tool for threat models that require real OPSEC.
- **It's session-only by design.** Close it and your tabs are gone. If you want session restore, use a different browser.
- **It doesn't sync anywhere.** No account, no cloud. If you want your bookmarks on another machine, copy `~/.config/bastion/state.txt` across.
- **It's built on WebKitGTK**, which isn't Chrome or Firefox. Some sites will look slightly off. Most won't.

---

## Problems

If the build fails, paste the full error somewhere and I'll probably tell you what's wrong. Common ones:

- **`libwebkit2gtk-4.1-dev` not found on Ubuntu < 22.04** — your distro is too old, WebKitGTK 4.1 requires a recent release.
- **Tor connection times out** — give it 30 seconds after `systemctl start tor` to bootstrap. Check with `curl --socks5-hostname 127.0.0.1:9050 https://check.torproject.org/api/ip`.
- **dnscrypt-proxy won't start** — it fights with systemd-resolved for port 53. The install script handles this on most distros but edge cases exist.

---

## License

Do whatever you want with it.

Only works on Linux

The only thing with this one is, it's not universal like Raiser Browser, Raiser works on macOS, Windows and Linux, so theres no debate in which ones better, but Bastion offers true security and privacy, because let's be honest, Linux is more advanced in privacy and security, so you can be truly anonymous on the internet.

Images of how it looks + logo:

<img width="1024" height="1024" alt="2026_04_17_10n_Kleki" src="https://github.com/user-attachments/assets/4e9e64d5-f969-4c30-a5fe-de8aa2c124d1" />
<img width="1291" height="918" alt="ss5" src="https://github.com/user-attachments/assets/ee6c0de4-ddc8-4dfb-905c-e1164f2c79ef" />
<img width="1288" height="934" alt="ss1" src="https://github.com/user-attachments/assets/004165c0-40b8-4cae-ad4f-2c258906f552" />
<img width="1287" height="938" alt="ss4" src="https://github.com/user-attachments/assets/a0257888-abe8-43e8-bbec-8ccf98c46cc3" />
<img width="1292" height="931" alt="ss3" src="https://github.com/user-attachments/assets/bbd1a36d-9f8e-42a6-8c08-d46a4df34b7f" />
<img width="1274" height="931" alt="ss2" src="https://github.com/user-attachments/assets/33af7ff8-9a95-46e1-b9a1-63101e1365ac" />



also yeah it was inspired by minecraft
