#!/bin/bash

PREREQS_ENV=/etc/profile.d/prereqs.sh
touch $PREREQS_ENV

OPENSSL_VERSION=1.0.2o
MAVEN_VERSION=3.6.1

install_mvn() {
	echo "Installing Maven"
	MVN=`which mvn`
	if [ -z $MVN ]; then
		wget -nv https://www-us.apache.org/dist/maven/maven-3/$MAVEN_VERSION/binaries/apache-maven-$MAVEN_VERSION-bin.tar.gz -P /tmp &&
		tar xf /tmp/apache-maven-*.tar.gz -C /opt &&
		rm /tmp/apache-maven-*.tar.gz
		ln -s /opt/apache-maven-$MAVEN_VERSION /opt/maven &&
		MVN=/opt/maven/bin/mvn &&
		test -f $MVN &&
		echo "Installing Maven DONE"
	fi
	MVN_BIN_DIR=`dirname ${MVN}`
	M2_HOME=`dirname ${MVN_BIN_DIR}`
	test -d $M2_HOME  &&
	echo "export M2_HOME=$M2_HOME" >>  $PREREQS_ENV &&
	echo "export PATH=\${M2_HOME}/bin:\${PATH}" >>  $PREREQS_ENV
}

install_protobuf() {
	echo "Installing Protobuf"
	pushd /tmp
	git clone -b 3.0.x --single-branch https://github.com/google/protobuf.git &&
	pushd protobuf &&
	./autogen.sh &&
	./configure --prefix=/usr --with-pic &&
	make -j4 && make install &&
	echo "Installing Protobuf DONE"
	popd
	rm -fr /tmp/protobuf*
	popd
}

install_openssl() {
  echo "Installing OpenSSL"
	pushd /tmp
  wget https://www.openssl.org/source/openssl-$OPENSSL_VERSION.tar.gz &&
	tar -xvzf openssl-$OPENSSL_VERSION.tar.gz &&
  cd openssl-$OPENSSL_VERSION &&
  CFLAGS=-fPIC ./config -fPIC -shared &&
  make && make install && echo "Installing OpenSSL DONE"
	popd
	echo "export OPENSSL_ROOT_DIR=/usr/local/ssl" >>  $PREREQS_ENV
  rm -fr /tmp/openss*l
}

install_curl() {
	echo "Installing CURL"
	pushd /tmp
  git clone https://github.com/curl/curl.git &&
	cd curl &&
  autoreconf -i &&
  ./configure --enable-lib-only &&
  make && make install && echo "Installing CURL DONE"
  popd
  rm -fr /tmp/curl
}

install_uuid() {
	echo "Installing libuuid"
	pushd /tmp
	wget https://sourceforge.net/projects/libuuid/files/libuuid-1.0.3.tar.gz &&
  tar -xvzf libuuid-1.0.3.tar.gz &&
  cd libuuid-1.0.3 &&
  sed -i s/2.69/2.63/ configure.ac &&
  aclocal &&
  automake --add-missing &&
  ./configure --with-pic CFLAGS="-I/usr/include/x86_64-linux-gnu" --disable-shared --enable-static &&
  autoreconf -i -f &&
  make && make install && echo "Installing libuuid DONE"
	popd
	rm -fr /tmp/libuuid*
}

apt_get_install() {
	apt-get --version && source install_ubuntu_prereqs.sh && install_prerequisites_ubuntu
}

yum_install() {
	yum version && source install_centos_prereqs.sh && install_prerequisites_centos
}

install_os_prerequisites() {
	case `uname` in
		Linux )
			apt_get_install || yum_install
			;;
		Darwin )
			echo "Mac OS not yet supported"
			exit 1
	esac
}

install_prerequisites() {
	install_os_prerequisites &&
	source $PREREQS_ENV &&
	install_mvn &&
	install_protobuf
}

install_prerequisites $2
if [[ $1 ]]; then
	echo "Installing static libraries"
	install_openssl &&
	install_curl &&
	install_uuid
fi
