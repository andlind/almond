FROM rockylinux/rockylinux:10

SHELL ["/bin/bash", "-c"]

ENV HOME=/root
ENV PKG_CONFIG_PATH=/usr/lib64/pkgconfig

# ------------------------------------------------------------
# Base development tools
# ------------------------------------------------------------
RUN dnf -y update && \
    dnf -y groupinstall "Development Tools" && \
    dnf -y install \
        pkgconfig openssl-devel zlib-devel libcurl-devel \
        cyrus-sasl-devel \
        rpmdevtools rpm-build \
        dnf-plugins-core && \
    dnf config-manager --set-enabled crb && \
    dnf -y install \
    rpm-build git wget tar gzip cmake gcc gcc-c++ \
    openssl-devel zlib-devel cyrus-sasl-devel \
    libzstd-devel lz4-devel \
    autoconf automake libtool \ 
    jansson-devel libcurl-devel \
    && dnf clean all
# ------------------------------------------------------------
# Prepare rpmbuild tree
# ------------------------------------------------------------
RUN rpmdev-setuptree

# ------------------------------------------------------------
# Build json-c RPM (using Fedora SPEC)
# ------------------------------------------------------------
# ------------------------------------------------------------
# Build json-c RPM (Rocky Linux 10 compatible)
# ------------------------------------------------------------
RUN wget https://github.com/json-c/json-c/archive/json-c-0.17.tar.gz \
        -O /root/rpmbuild/SOURCES/json-c-0.17.tar.gz

# Write a Rocky-compatible SPEC file directly inside Dockerfile
RUN cat > /root/rpmbuild/SPECS/json-c.spec << 'EOF'
Name:           json-c
Version:        0.17
Release:        1%{?dist}
Summary:        JSON implementation in C
License:        MIT
URL:            https://github.com/json-c/json-c
Source0:        json-c-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  cmake
BuildRequires:  pkgconfig

%description
A JSON implementation in C.

%package devel
Summary:        Development files for json-c
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
This package contains the header files and libraries needed to 
compile applications using json-c.

%prep
%setup -q -n json-c-json-c-%{version}

%build
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j%{_smp_build_ncpus}

%install
cd build
make DESTDIR=%{buildroot} install

%files
/usr/lib64/libjson-c.so.*
/usr/lib64/libjson-c.so
/usr/lib64/libjson-c.a
/usr/lib64/pkgconfig/json-c.pc
/usr/include/json-c/*
/usr/lib64/cmake/json-c/json-c-config.cmake
/usr/lib64/cmake/json-c/json-c-targets.cmake
/usr/lib64/cmake/json-c/json-c-targets-debug.cmake

%files devel
%{_includedir}/json-c/*
%{_libdir}/libjson-c.so
%{_libdir}/libjson-c.a
%{_libdir}/pkgconfig/json-c.pc
%{_libdir}/cmake/json-c/*.cmake

%changelog
* Thu Apr 30 2026 Docker Build <andreas.lindell@almondmonitor.com> - 0.17-1
- Initial build for Rocky Linux 10
EOF

#RUN rpmbuild -ba /root/rpmbuild/SPECS/json-c.spec && \
#     dnf -y install /root/rpmbuild/RPMS/x86_64/json-c-*.rpm \
#     dnf -y install /root/rpmbuild/RPMS/x86_64/json-c-devel*.rpm
RUN rpmbuild -ba /root/rpmbuild/SPECS/json-c.spec && \
    dnf -y install /root/rpmbuild/RPMS/x86_64/json-c-0.17*.rpm \
                   /root/rpmbuild/RPMS/x86_64/json-c-devel*.rpm

# ------------------------------------------------------------
# Build Snappy RPM (using Fedora SPEC)
# ------------------------------------------------------------
RUN wget https://github.com/google/snappy/archive/refs/tags/1.1.10.tar.gz \
        -O /root/rpmbuild/SOURCES/snappy-1.1.10.tar.gz
RUN cat > /root/rpmbuild/SPECS/snappy.spec << 'EOF'
Name:           snappy
Version:        1.1.10
Release:        1%{?dist}
Summary:        Fast compression and decompression library
License:        BSD
URL:            https://github.com/google/snappy
Source0:        snappy-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  make
BuildRequires:  cmake

%description
Snappy is a fast compressor/decompressor library.

%prep
%setup -q -n snappy-%{version}

%build
mkdir build
cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DSNAPPY_BUILD_TESTS=OFF \
    -DSNAPPY_BUILD_BENCHMARKS=OFF \
    -DBUILD_SHARED_LIBS=ON 
make -j%{_smp_build_ncpus}

%install
cd build
make DESTDIR=%{buildroot} install

# Runtime package
%files
/usr/lib64/libsnappy.so
/usr/lib64/libsnappy.so.1
/usr/lib64/libsnappy.so.1.1.10
#/usr/lib64/libsnappy.a

# Development package
%package devel
Summary: Development files for snappy
Requires: snappy%{?_isa} = %{version}-%{release}

%description devel
Development headers and libraries for snappy.

%files devel
/usr/include/snappy*.h
/usr/lib64/cmake/Snappy/*

%changelog
* Thu Apr 30 2026 Docker Build <andreas.lindell@almondmonitor.com> - 1.1.10-1
- Initial build for Rocky Linux 10
EOF

RUN rpmbuild -ba /root/rpmbuild/SPECS/snappy.spec && \
    dnf -y install /root/rpmbuild/RPMS/x86_64/snappy-*.rpm

# ------------------------------------------------------------
# Build librdkafka RPM
# ------------------------------------------------------------

#RUN mkdir -p /root/rpmbuild/{SOURCES,SPECS,RPMS,SRPMS,BUILD,BUILDROOT}
# Ensure you have this file in your current directory on your Mac
COPY librdkafka-2.7.0.tar.gz /root/rpmbuild/SOURCES/

WORKDIR /root/rpmbuild/SOURCES

RUN cat <<EOF > /root/rpmbuild/SPECS/librdkafka.spec
%define debug_package %{nil}

Name:           librdkafka
Version:        2.7.0
Release:        1%{?dist}
Summary:        The Apache Kafka C/C++ library
License:        BSD
URL:            https://github.com/edenhill/librdkafka
Source0:        librdkafka-2.7.0.tar.gz

BuildRequires:  cmake gcc gcc-c++ openssl-devel zlib-devel cyrus-sasl-devel libzstd-devel lz4-devel
Requires:       openssl zlib cyrus-sasl libzstd lz4

%description
librdkafka is a high performance C/C++ library for Apache Kafka.

%package devel
Summary:        Development files for librdkafka
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description devel
Contains headers and static libraries for librdkafka.

%prep
%setup -q
sed -i '1i #include <openssl/rand.h>' src/rdkafka_admin.c

%build
mkdir -p build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr \
      -DCMAKE_INSTALL_LIBDIR=%{_libdir} \
      -DRDKAFKA_BUILD_EXAMPLES=OFF \
      -DRDKAFKA_BUILD_TESTS=OFF \
      -DWITH_ZSTD=ON \
      -DWITH_LZ4=ON \
      -DCMAKE_C_FLAGS="-fPIC -DIOV_MAX=1024 -Wno-int-conversion -Wno-implicit-function-declaration -Wno-error" \
      -DCMAKE_CXX_FLAGS="-fPIC -Wno-error" ..
make -j\$(nproc)

%install
cd build
%make_install

%files
# Use a wildcard to catch both LICENSE and LICENSES.txt
%license LICENSE*
%{_libdir}/librdkafka.so.*
%{_libdir}/librdkafka++.so.*

%files devel
%{_includedir}/librdkafka/
%{_libdir}/librdkafka.so
%{_libdir}/librdkafka++.so
%{_libdir}/pkgconfig/rdkafka*.pc
%{_libdir}/cmake/RdKafka/

%changelog
* Tue May 05 2026 Build User <builder@example.com> - 2.7.0-1
- Final build with license and changelog fixes
EOF

RUN rpmbuild -ba /root/rpmbuild/SPECS/librdkafka.spec
RUN dnf -y install /root/rpmbuild/RPMS/x86_64/librdkafka-*.rpm

RUN dnf clean all && dnf -y makecache

RUN dnf -y install cmake gcc gcc-c++ make wget jansson-devel zlib-devel libzstd-devel snappy-devel pkgconfig rpm-build && \
    wget  https://archive.apache.org/dist/avro/avro-1.11.3/c/avro-c-1.11.3.tar.gz
    #wget https://downloads.apache.org/avro/avro-1.11.3/c/avro-c-1.11.3.tar.gz -O ~/rpmbuild/SOURCES/avro-c-1.11.3.tar.gz
    #wget https://github.com/apache/avro/archive/refs/tags/release-1.11.3.tar.gz -O ~/rpmbuild/SOURCES/avro-1.11.3.tar.gz && \ 
    #mv ~/rpmbuild/SOURCES/avro-1.11.3.tar.gz ~/rpmbuild/SOURCES/avro-c-1.11.3.tar.gz

    # create SPEC (see block above) saved to ~/rpmbuild/SPECS/avro-c.spec && \
    # download already saved to ~/rpmbuild/SOURCES/avro-1.11.3.tar.gz
RUN cat > ~/rpmbuild/SPECS/avro-c.spec <<'EOF'
Name:           avro-c
Version:        1.11.3
Release:        1%{?dist}
Summary:        Apache Avro C library
License:        Apache-2.0
Source0:        %{name}-%{version}.tar.gz
#Patch0:         remove-examples.patch
BuildRequires: cmake
BuildRequires: gcc
BuildRequires: gcc-c++
BuildRequires: make
BuildRequires: pkgconfig
BuildRequires: jansson-devel
BuildRequires: zlib-devel
BuildRequires: snappy-devel
BuildRequires: xz-devel

%description
Apache Avro is a data serialization system.

%prep
%setup -q -n avro-c-1.11.3
#%patch0 -p1
# Fix broken pkgconfig placeholders
#sed -i \
#  -e 's/@LZMA_PKG@/xz/' \
#  -e 's/@SNAPPY_PKG@/snappy/' \
#  -e 's/@ZLIB_PKG@/zlib/' \
#  avro-c.pc.in

# Remove broken examples to avoid build failure
rm -rf examples

# Disable examples in CMakeLists.txt
sed -i '/add_subdirectory(.*examples.*)/d' CMakeLists.txt

%build
#cd lang/c
mkdir -p build
cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=%{_prefix} \
  -DAVRO_BUILD_EXAMPLES=OFF \
  -DAVRO_BUILD_TESTS=OFF \
  -DLZMA_PKG=liblzma \
  -DSNAPPY_PKG=snappy \
  -DZLIB_PKG=zlib
make -j$(nproc)

%install
#cd lang/c/build
cd build
make DESTDIR=%{buildroot} install
# Fix broken pkgconfig placeholders in installed avro-c.pc
sed -i \
  -e 's/@LZMA_PKG@/liblzma/' \
  -e 's/@SNAPPY_PKG@/snappy/' \
  -e 's/@ZLIB_PKG@/zlib/' \
  %{buildroot}/usr/lib64/pkgconfig/avro-c.pc


%files
/usr/include/avro.h
/usr/include/avro/*.h
/usr/lib64/libavro*
/usr/lib64/pkgconfig/avro-c.pc
/usr/bin/avroappend
/usr/bin/avrocat
/usr/bin/avropipe
/usr/bin/avromod
/usr/share/doc/avro-c/index.html


%changelog
* Fri Jun 12 2026 Packager <packager@example.com> - 1.11.3-1
- initial package
EOF

# Add patch to disable examples (Avro 1.11.3 bug)
#RUN cat > ~/rpmbuild/SOURCES/remove-examples.patch <<'EOF'
#--- a/CMakeLists.txt
#+++ b/CMakeLists.txt
#@@ -150,7 +150,6 @@ add_subdirectory(src)
# add_subdirectory(tests)
#
# # Examples
#-add_subdirectory(examples)
#
# # Documentation
# add_subdirectory(doc)
#EOF

RUN rpmbuild -bb ~/rpmbuild/SPECS/avro-c.spec && \
    dnf -y install ~/rpmbuild/RPMS/x86_64/avro-c-1.11.3*.rpm


# ------------------------------------------------------------
# Build libserdes RPM (create simple SPEC)
# ------------------------------------------------------------
RUN cat > ~/rpmbuild/SPECS/libserdes.spec << 'EOF'
Name: libserdes
Version: 1.0
Release: 1%{?dist}
Summary: Serdes library for Avro + Kafka
License: BSD
URL: https://github.com/confluentinc/libserdes
Source0: libserdes.tar.gz
BuildRequires: librdkafka-devel
BuildRequires: json-c-devel
BuildRequires: snappy-devel

%description
Library for schema-based serialization/deserialization.

%prep
# Comment out setup so it doesn't look for a tarball
# %setup -q -n libserdes
# Just ensure we are in the right spot
#cd %{_builddir}/libserdes
#rm -rf %{_builddir}/libserdes
#cp -r /tmp/libserdes %{_builddir}/libserdes
#mkdir -p %{_builddir}/libserdes
#rm -rf %{_builddir}/libserdes
#mkdir -p %{_builddir}/libserdes
#cp -r /tmp/libserdes/* %{_builddir}/libserdes/

%build
#cd %{_builddir}/libserdes
#if [ ! -f ./bootstrap.sh ]; then
#    echo "Files in current directory:"
#    ls -la
#    exit 1
#fi
#chmod +x bootstrap.sh
#./bootstrap.sh
#./configure --prefix=/usr --libdir /usr/lib64
#make -j$(nproc)
#make DESTDIR=%{buildroot} install
#make

%install
#cd %{_builddir}/libserdes
#make DESTDIR=%{buildroot} install
#mkdir -p %{buildroot}/usr/lib64
#mkdir -p %{buildroot}/usr/include/libserdes
#cp -r %{_builddir}/libserdes/src/*.h %{buildroot}/usr/include/libserdes/
#cp -r %{_builddir}/libserdes/src/libserdes.{a,so*} %{buildroot}/usr/lib64/ 2>/dev/null || :
cd %{_builddir}/libserdes
#make DESTDIR=%{buildroot} install
make DESTDIR=%{buildroot} INSTALL="install" install

%files
#%{_libdir}/libserdes.so.*
#%{_libdir}/libserdes.so
#%{_includedir}/libserdes/
/usr/lib64/*
/usr/include/libserdes/*
EOF

#RUN git clone https://github.com/confluentinc/libserdes.git /tmp/libserdes && \
#    cd /tmp/libserdes && \
#    git archive --format=tar.gz --prefix=libserdes/ HEAD > ~/rpmbuild/SOURCES/libserdes.tar.gz && \
#    rpmbuild -ba ~/rpmbuild/SPECS/libserdes.spec && \
#    dnf -y install ~/rpmbuild/RPMS/x86_64/libserdes*.rpm && \
#    rm -rf /tmp/libserdes

# Temporary switch to LEGACY to allow the git clone handshake
#RUN update-crypto-policies --set LEGACY && \
#    git -c http.sslVerify=false clone https://github.com/confluentinc/libserdes.git /tmp/libserdes && \
#    cd /tmp/libserdes && \
#    git archive --format=tar.gz --prefix=libserdes/ HEAD > ~/rpmbuild/SOURCES/libserdes.tar.gz && \
#    # Remove the --prefix=libserdes/
#    #git archive --format=tar.gz HEAD > ~/rpmbuild/SOURCES/libserdes.tar.gz && \
#    rpmbuild -ba ~/rpmbuild/SPECS/libserdes.spec && \
#    dnf -y install ~/rpmbuild/RPMS/x86_64/libserdes*.rpm && \
#    rm -rf /tmp/libserdes && \
#    update-crypto-policies --set DEFAULT

RUN update-crypto-policies --set LEGACY && \
    # 1. Clone with submodules
    git -c http.sslVerify=false clone --recursive https://github.com/confluentinc/libserdes.git /tmp/libserdes_source && \
    cd /tmp/libserdes_source && \
    # 2. Skip bootstrap, go straight to configure
    ./configure --prefix=/usr --libdir=/usr/lib64 && \
    # 3. Build
    make -j$(nproc) && \
    # 4. Prepare for RPM build
    mkdir -p ~/rpmbuild/BUILD/libserdes && \
    cp -r /tmp/libserdes_source/* ~/rpmbuild/BUILD/libserdes/ && \
    # 5. Build RPM and Install
    rpmbuild -bb --noclean ~/rpmbuild/SPECS/libserdes.spec && \
    dnf -y install ~/rpmbuild/RPMS/x86_64/libserdes*.rpm && \
    update-crypto-policies --set DEFAULT

# ------------------------------------------------------------
# Ready for building almond-monitor RPM
# ------------------------------------------------------------
WORKDIR /workspace
CMD ["/bin/bash"]

