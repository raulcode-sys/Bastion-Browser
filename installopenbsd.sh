#!/bin/sh
# ─────────────────────────────────────────────────────────────────────────────
#  Bastion Browser – OpenBSD install & build script
#  Tested target: OpenBSD 7.6+
#
#  Run as a regular user with doas configured.
# ─────────────────────────────────────────────────────────────────────────────
set -eu

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; BOLD='\033[1m'; NC='\033[0m'

info()    { printf "${CYAN}[Bastion]${NC} %s\n" "$*"; }
success() { printf "${GREEN}[OK]${NC} %s\n" "$*"; }
warn()    { printf "${YELLOW}[!]${NC} %s\n" "$*"; }
error()   { printf "${RED}[X]${NC} %s\n" "$*"; exit 1; }

printf "${BOLD}"
cat <<'BANNER'
  ██████╗  █████╗ ███████╗████████╗██╗ ██████╗ ███╗   ██╗
  ██╔══██╗██╔══██╗██╔════╝╚══██╔══╝██║██╔═══██╗████╗  ██║
  ██████╔╝███████║███████╗   ██║   ██║██║   ██║██╔██╗ ██║
  ██╔══██╗██╔══██║╚════██║   ██║   ██║██║   ██║██║╚██╗██║
  ██████╔╝██║  ██║███████║   ██║   ██║╚██████╔╝██║ ╚████║
  ╚═════╝ ╚═╝  ╚═╝╚══════╝   ╚═╝   ╚═╝ ╚═════╝ ╚═╝  ╚═══╝
BANNER
printf "${NC}"
info "OpenBSD installer"

SCRIPT_DIR="$(cd -- "$(dirname -- "$0")" && pwd)"

# ─── Sanity checks ────────────────────────────────────────────────────────────
if [ "$(uname -s)" != "OpenBSD" ]; then
    error "This script is for OpenBSD. Detected: $(uname -s)."
fi

if ! command -v doas >/dev/null 2>&1; then
    error "doas is required. Configure /etc/doas.conf first."
fi

# ─── 1. Build dependencies ────────────────────────────────────────────────────
# IMPORTANT: package name is "pkg-config" (with dash), not "pkgconf".
info "Installing build dependencies via pkg_add..."

PKGS="cmake ninja pkg-config gtk+3 webkitgtk41 libsoup3 glib2 pango git"

for pkg in $PKGS; do
    if pkg_info -e "${pkg}-*" >/dev/null 2>&1 || \
       pkg_info -e "${pkg}"   >/dev/null 2>&1; then
        printf "  - %s already installed\n" "$pkg"
    else
        printf "  - installing %s ...\n" "$pkg"
        doas pkg_add -I "$pkg" || warn "Failed to install $pkg — continuing."
    fi
done
success "Build dependencies done."

# ─── 2. Tor ───────────────────────────────────────────────────────────────────
echo ""
info "Checking Tor..."
if ! command -v tor >/dev/null 2>&1; then
    doas pkg_add -I tor || warn "Could not install tor."
fi
doas rcctl enable tor 2>/dev/null || true
doas rcctl start  tor 2>/dev/null || true
success "Tor ready (SOCKS5 on 127.0.0.1:9050)."

# ─── 3. DNS-over-TLS via unbound ──────────────────────────────────────────────
echo ""
info "Configuring unbound for DNS-over-TLS..."
UNBOUND_CONF="/var/unbound/etc/unbound.conf"
if [ -f "$UNBOUND_CONF" ]; then
    if [ ! -f "${UNBOUND_CONF}.bastion-backup" ]; then
        doas cp "$UNBOUND_CONF" "${UNBOUND_CONF}.bastion-backup"
    fi
    doas tee "$UNBOUND_CONF" > /dev/null <<'UNBOUND'
server:
    interface: 127.0.0.1
    do-ip4: yes
    do-ip6: no
    do-udp: yes
    do-tcp: yes
    access-control: 127.0.0.0/8 allow
    hide-identity: yes
    hide-version: yes
    qname-minimisation: yes
    aggressive-nsec: yes
    log-queries: no
    log-replies: no

forward-zone:
    name: "."
    forward-tls-upstream: yes
    forward-addr: 1.1.1.1@853#cloudflare-dns.com
    forward-addr: 1.0.0.1@853#cloudflare-dns.com
    forward-addr: 9.9.9.9@853#dns.quad9.net
UNBOUND
    doas rcctl enable  unbound 2>/dev/null || true
    doas rcctl restart unbound 2>/dev/null || true
    success "unbound configured."
else
    warn "unbound.conf not found; skipping DoT setup."
fi

# ─── 4. Point resolv.conf at local unbound ────────────────────────────────────
echo ""
info "Pointing /etc/resolv.conf at local unbound..."

# OpenBSD's resolvd overwrites resolv.conf — disable it first.
if rcctl check resolvd >/dev/null 2>&1; then
    info "Disabling resolvd (it manages /etc/resolv.conf)..."
    doas rcctl stop    resolvd 2>/dev/null || true
    doas rcctl disable resolvd 2>/dev/null || true
fi

# Backup
if [ -f /etc/resolv.conf ] && [ ! -f /etc/resolv.conf.bastion-backup ]; then
    doas cp /etc/resolv.conf /etc/resolv.conf.bastion-backup 2>/dev/null || true
fi

# Drop any immutable / link-locked flags
doas chflags nosunlnk /etc/resolv.conf 2>/dev/null || true
doas chflags noschg   /etc/resolv.conf 2>/dev/null || true

if printf 'nameserver 127.0.0.1\nlookup file bind\n' \
   | doas tee /etc/resolv.conf > /dev/null 2>&1; then
    success "/etc/resolv.conf set to use local unbound."
else
    warn "Couldn't write /etc/resolv.conf — DNS may still leak."
    warn "Try manually: echo 'nameserver 127.0.0.1' | doas tee /etc/resolv.conf"
fi

# ─── 5. Build ─────────────────────────────────────────────────────────────────
echo ""
info "Building Bastion Browser..."
BUILD_DIR="${SCRIPT_DIR}/build"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# OpenBSD uses clang; strip GCC-only flags.
export CXX=clang++
export CXXFLAGS="-Wno-error -O2 -fstack-protector-strong -fPIE -D_FORTIFY_SOURCE=2"
export LDFLAGS="-pie -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack"
export PKG_CONFIG_PATH="/usr/local/lib/pkgconfig:/usr/local/share/pkgconfig"

cmake .. -GNinja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=clang++ \
    || error "CMake configure failed. Likely cause: webkitgtk41 missing or wrong version."

ninja -j"$(sysctl -n hw.ncpuonline)" \
    || error "Build failed. Paste the error and I'll fix it."

success "Build complete!"

# ─── 6. Install ───────────────────────────────────────────────────────────────
echo ""
info "Installing..."
doas cp "${BUILD_DIR}/bastion" /usr/local/bin/bastion
doas chmod 755 /usr/local/bin/bastion

doas mkdir -p /usr/local/share/applications
doas tee /usr/local/share/applications/bastion.desktop > /dev/null <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=Bastion Browser
Exec=bastion %U
Icon=web-browser
Terminal=false
Categories=Network;WebBrowser;
MimeType=x-scheme-handler/http;x-scheme-handler/https;
DESKTOP

if command -v update-desktop-database >/dev/null 2>&1; then
    doas update-desktop-database /usr/local/share/applications/ 2>/dev/null || true
fi

echo ""
printf "${GREEN}${BOLD}══════════════════════════════════════════${NC}\n"
printf "${GREEN}${BOLD}  Bastion Browser installed!${NC}\n"
printf "${GREEN}${BOLD}══════════════════════════════════════════${NC}\n"
printf "  Run: ${CYAN}bastion${NC}\n"
printf "  Modes: ${CYAN}bastion --proxy=tor|i2p|none|socks5://h:port${NC}\n"
echo ""
warn "OpenBSD support is best-effort. WebKitGTK on OpenBSD lags Linux."
echo ""
