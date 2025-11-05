#!/bin/sh

#############################################################
# sngrep bootstrap script
# This script prepares the build environment and installs
# required dependencies on Red Hat/CentOS/Fedora systems
#############################################################

echo "======================================"
echo " sngrep Bootstrap Script"
echo "======================================"
echo ""

check_for_app() {
        $1 --version 2>&1 >/dev/null
        if [ $? != 0 ]
        then
                echo "Please install $1 and run bootstrap.sh again!"
                exit 1
        fi
}

# Check and install libpcap dependencies on Red Hat based systems
check_and_install_libpcap() {
        # Determine package manager (dnf is preferred over yum on newer systems)
        PKG_MANAGER=""
        if command -v dnf >/dev/null 2>&1; then
                PKG_MANAGER="dnf"
        elif command -v yum >/dev/null 2>&1; then
                PKG_MANAGER="yum"
        fi
        
        # Check if we're on a Red Hat based system
        if [ ! -z "$PKG_MANAGER" ]; then
                echo "Detected Red Hat based system using $PKG_MANAGER"
                echo "Checking for required development dependencies..."
                
                # List of required packages
                PACKAGES="libpcap-devel libpcap ncurses-devel"
                # Optional but recommended packages
                OPTIONAL_PACKAGES="openssl-devel pcre-devel"
                MISSING_PACKAGES=""
                MISSING_OPTIONAL=""
                
                # Check which packages are missing
                for pkg in $PACKAGES; do
                        rpm -q $pkg >/dev/null 2>&1
                        if [ $? != 0 ]; then
                                MISSING_PACKAGES="$MISSING_PACKAGES $pkg"
                        else
                                echo "  ✓ $pkg is installed"
                        fi
                done
                
                # Check optional packages
                echo "Checking optional packages..."
                for pkg in $OPTIONAL_PACKAGES; do
                        rpm -q $pkg >/dev/null 2>&1
                        if [ $? != 0 ]; then
                                MISSING_OPTIONAL="$MISSING_OPTIONAL $pkg"
                                echo "  ○ $pkg is not installed (optional)"
                        else
                                echo "  ✓ $pkg is installed"
                        fi
                done
                
                # Install missing packages if any
                if [ ! -z "$MISSING_PACKAGES" ]; then
                        echo "Missing packages:$MISSING_PACKAGES"
                        echo "Attempting to install missing packages..."
                        
                        # Check if we have sudo privileges
                        if [ "$EUID" -ne 0 ]; then
                                echo "Installing packages requires root privileges."
                                echo "Running: sudo $PKG_MANAGER install -y$MISSING_PACKAGES"
                                sudo $PKG_MANAGER install -y $MISSING_PACKAGES
                        else
                                $PKG_MANAGER install -y $MISSING_PACKAGES
                        fi
                        
                        # Check if installation was successful
                        FAILED=""
                        for pkg in $MISSING_PACKAGES; do
                                rpm -q $pkg >/dev/null 2>&1
                                if [ $? != 0 ]; then
                                        FAILED="$FAILED $pkg"
                                fi
                        done
                        
                        if [ ! -z "$FAILED" ]; then
                                echo "ERROR: Failed to install:$FAILED"
                                echo "Please install manually with: sudo $PKG_MANAGER install$FAILED"
                                exit 1
                        else
                                echo "All packages installed successfully!"
                        fi
                else
                        echo "All required development packages are already installed."
                fi
                
                # Ask about optional packages
                if [ ! -z "$MISSING_OPTIONAL" ]; then
                        echo ""
                        echo "Optional packages missing:$MISSING_OPTIONAL"
                        echo "These packages enable additional features:"
                        echo "  - openssl-devel: TLS/SSL support for encrypted SIP"
                        echo "  - pcre-devel: Advanced regular expressions"
                        echo ""
                        echo -n "Would you like to install optional packages? (y/N): "
                        read REPLY
                        if [ "$REPLY" = "y" ] || [ "$REPLY" = "Y" ]; then
                                if [ "$EUID" -ne 0 ]; then
                                        sudo $PKG_MANAGER install -y $MISSING_OPTIONAL
                                else
                                        $PKG_MANAGER install -y $MISSING_OPTIONAL
                                fi
                        fi
                fi
        fi
        
        # For Debian/Ubuntu systems
        if command -v apt-get >/dev/null 2>&1; then
                echo "Checking for libpcap on Debian/Ubuntu system..."
                dpkg -l | grep -q libpcap-dev
                if [ $? != 0 ]; then
                        echo "libpcap-dev not found."
                        echo "Please install with: sudo apt-get install libpcap-dev libncurses5-dev"
                fi
        fi
}

# On FreeBSD and OpenBSD, multiple autoconf/automake versions have different names.
# On Linux, environment variables tell which one to use.

case `uname -sr` in
        OpenBSD*)
                export AUTOCONF_VERSION=2.69
                export AUTOMAKE_VERSION=1.15
                ;;
        FreeBSD*)
                AUTOCONF_VERSION=2.69
                AUTOMAKE_VERSION=1.15
                export AUTOCONF_VERSION
                export AUTOMAKE_VERSION
                ;;
        *)
                ;;
esac

check_for_app autoconf
check_for_app autoheader
check_for_app automake
check_for_app aclocal

# Check and install development dependencies
check_and_install_libpcap

echo "Generating the configure script ..."

aclocal
autoconf
autoheader
automake --add-missing --copy 2>/dev/null

echo ""
echo "======================================"
echo " Bootstrap completed successfully!"
echo "======================================"
echo ""
echo "Next steps:"
echo "  1. Run: ./configure"
echo "  2. Run: make"
echo "  3. Run: sudo make install"
echo ""
echo "Optional configure flags:"
echo "  --with-openssl    Enable OpenSSL support"
echo "  --with-gnutls     Enable GnuTLS support"  
echo "  --with-pcre       Enable PCRE support"
echo "  --prefix=/path    Custom installation path"
echo ""

exit 0