 # Building .deb files
 Take your source tarball, rename it to qview_#.#.orig.tar.gz and extract it to qview-#.#. Move the debian folder to the qview-#.# folder. Make sure to change the control and changelog versions, and change the maintainer name if you happen to not be me.

 Run dpkg-buildpackage to build