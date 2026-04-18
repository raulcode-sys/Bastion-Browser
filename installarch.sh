#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
#  Bastion Browser – Arch / CachyOS install & build script
#  Run as a regular user (sudo used internally where needed)
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

info()    { echo -e "${CYAN}[Bastion]${NC} $*"; }
success() { echo -e "${GREEN}[✓]${NC} $*"; }
warn()    { echo -e "${YELLOW}[!]${NC} $*"; }
error()   { echo -e "${RED}[✗]${NC} $*"; exit 1; }

echo -e "${BOLD}"
echo "  ██████╗  █████╗ ███████╗████████╗██╗ ██████╗ ███╗   ██╗"
echo "  ██╔══██╗██╔══██╗██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║"
echo "  ██████╔╝███████║███████╗   ██║   ██║██║   ██║██╔██╗ ██║"
echo "  ██╔══██╗██╔══██║╚════██║   ██║   ██║██║   ██║██║╚██╗██║"
echo "  ██████╔╝██║  ██║███████║   ██║   ██║╚██████╔╝██║ ╚████║"
echo "  ╚═════╝ ╚═╝  ╚═╝╚══════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝"
echo -e "${NC}"
info "Arch / CachyOS installer"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ─── Sanity: pacman must exist ────────────────────────────────────────────────
if ! command -v pacman &>/dev/null; then
    error "pacman not found. This script is for Arch-based distros (Arch, CachyOS, Manjaro, EndeavourOS). Use installdeb.sh on Debian/Ubuntu."
fi

# ─── 1. Build dependencies ────────────────────────────────────────────────────
info "Syncing package databases..."
sudo pacman -Syy --noconfirm 2>&1 | tail -3

info "Installing build dependencies..."
# webkit2gtk-4.1 is the correct package on Arch (webkit2gtk is the old 4.0)
sudo pacman -S --needed --noconfirm \
    base-devel \
    cmake \
    ninja \
    pkgconf \
    gtk3 \
    webkit2gtk-4.1 \
    libsoup3 \
    glib2 \
    pango \
    git \
    curl \
    ca-certificates
success "Build dependencies installed."

# ─── 2. Tor ───────────────────────────────────────────────────────────────────
echo ""
info "Checking Tor..."
if ! command -v tor &>/dev/null; then
    sudo pacman -S --needed --noconfirm tor
fi
sudo systemctl enable tor --now 2>/dev/null || true
success "Tor ready."

# ─── 3. dnscrypt-proxy (DNS-over-HTTPS) ───────────────────────────────────────
echo ""
info "Checking dnscrypt-proxy (DoH)..."
if ! command -v dnscrypt-proxy &>/dev/null; then
    sudo pacman -S --needed --noconfirm dnscrypt-proxy || \
        warn "dnscrypt-proxy unavailable — skipping DoH setup."
fi
if command -v dnscrypt-proxy &>/dev/null; then
    sudo systemctl enable dnscrypt-proxy --now 2>/dev/null || true
    success "dnscrypt-proxy running."
fi

# ─── 4. Kernel sysctl hardening ───────────────────────────────────────────────
echo ""
info "Applying kernel sysctl hardening..."
sudo tee /etc/sysctl.d/99-bastion.conf > /dev/null <<'SYSCTL'
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0
net.ipv4.tcp_syncookies = 1
kernel.dmesg_restrict = 1
kernel.kptr_restrict = 2
kernel.randomize_va_space = 2
net.core.bpf_jit_harden = 2
SYSCTL
sudo sysctl -p /etc/sysctl.d/99-bastion.conf >/dev/null 2>&1 || true
success "Kernel hardening applied."

# ─── 5. Build ─────────────────────────────────────────────────────────────────
echo ""
info "Building Bastion Browser..."
BUILD_DIR="${SCRIPT_DIR}/build"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"
cmake .. -GNinja -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -6
ninja -j"$(nproc)"
success "Build complete!"

# ─── 6. Install ───────────────────────────────────────────────────────────────
echo ""
info "Installing..."
sudo cp "${BUILD_DIR}/bastion" /usr/local/bin/bastion
sudo chmod 755 /usr/local/bin/bastion
sudo tee /usr/share/applications/bastion.desktop > /dev/null <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Bastion Browser
Exec=bastion %U
Icon=web-browser
Terminal=false
Categories=Network;WebBrowser;
MimeType=x-scheme-handler/http;x-scheme-handler/https;
DESKTOP
sudo update-desktop-database /usr/share/applications/ 2>/dev/null || true

echo ""
echo -e "${GREEN}${BOLD}══════════════════════════════════════════${NC}"
echo -e "${GREEN}${BOLD}  Bastion Browser installed!${NC}"
echo -e "${GREEN}${BOLD}══════════════════════════════════════════${NC}"
echo -e "  Run: ${CYAN}bastion${NC}"
echo -e "  Modes: ${CYAN}bastion --proxy=tor|i2p|none|socks5://h:port${NC}"
echo ""
