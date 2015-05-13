pkgname=pocketvox-git
pkgver=20150507
pkgrel=1
pkgdesc="pocket vox speech recognition."
arch=('any')
url="https://github.com/sonar-gnu-linux/pocketVox"
license=('GPLv3')
groups=()
depends=('gstreamer')
makedepends=('pocketsphinx-git' 'libappindicator3' 'gtk-doc' 'intltool' 'autoconf' 'binutils' 'git' 'glib2' 'gobject-introspection' 'gobject-introspection-runtime' 'espeak' 'bison' 'python2' 'libnotify' 'gst-plugins-base' 'gtk3' 'gtk-doc' ) 
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=("git://github.com/sonar-gnu-linux/pocketVox.git")
_pythondir="$pkgdir/usr/lib/python2.7/site-packages"
md5sums=('SKIP') #generate with 'makepkg -g'
build() {
        cd "$srcdir/pocketVox/"
        ./autogen.sh --prefix=/usr PYTHON=/usr/bin/python2
        make
}
package() {
          cd "$srcdir/pocketVox/"
          install -d $_pythondir
          PYTHONPATH=$_pythondir make DESTDIR="$pkgdir" install
          install -d "$pkgdir/usr/share/licenses/$pkgname"
          install -m644 COPYING "$pkgdir/usr/share/licenses/$pkgname/"
}