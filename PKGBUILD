pkgname=pocketvox-git
pkgver=20150507
pkgrel=1
pkgdesc="pocket vox speech recognition."
arch=('any')
url="https://github.com/benoitfragit/pocketVox"
license=('GPLv3')
groups=()
depends=('gstreamer')
makedepends=( 'git' ) 
checkdepends=()
optdepends=()
provides=()
conflicts=()
replaces=()
backup=()
options=()
install=
changelog=
source=("git://github.com/benoitfragit/pocketVox.git")
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