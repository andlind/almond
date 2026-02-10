Name:           avro-c
Version:        1.11.3
Release:        1%{?dist}
Summary:        Apache Avro C library

License:        Apache-2.0
URL:            https://avro.apache.org/
Source0:        https://archive.apache.org/dist/avro/avro-%{version}/avro-src-%{version}.tar.gz

# EL9 auto-generates debuginfo/debugsource, but Avro C installs no sources
%global debug_package %{nil}

BuildRequires:  cmake
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  jansson-devel
BuildRequires:  zlib-devel

# Optional compression libs disabled to avoid pkgconfig(@SNAPPY_PKG@) issues
# If you want Snappy/LZMA, install snappy-devel/xz-devel and remove these flags.

%description
Apache Avro is a data serialization system. This package contains the C
implementation of the Avro serialization library.

%prep
%autosetup -n avro-src-%{version}
# Disable building examples (they fail on modern compilers)
sed -i 's|add_subdirectory(examples)|# add_subdirectory(examples)|' lang/c/CMakeLists.txt
sed -i 's|add_subdirectory(tests)|# add_subdirectory(tests)|' lang/c/CMakeLists.txt

%build
cd lang/c
mkdir build
cd build

cmake .. \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DCMAKE_INSTALL_LIBDIR=%{_libdir} 
make -j$(nproc)

%install
cd lang/c/build
make install DESTDIR=%{buildroot}
# Fix broken pkgconfig placeholders in the installed file
sed -i \
    -e 's/@LZMA_PKG@//g' \
    -e 's/@SNAPPY_PKG@//g' \
    -e 's/@ZLIB_PKG@/zlib/g' \
    %{buildroot}%{_libdir}/pkgconfig/avro-c.pc

%files
%license lang/c/LICENSE
%doc %{_docdir}/%{name}/index.html

# CLI tools
%{_bindir}/avroappend
%{_bindir}/avrocat
%{_bindir}/avromod
%{_bindir}/avropipe

# Runtime library
%{_libdir}/libavro.so*

%package devel
Summary: Development files for Avro C
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
Headers and development files for the Avro C library.

%files devel
%license lang/c/LICENSE
%{_includedir}/avro/
%{_includedir}/avro.h
%{_libdir}/libavro.a
%{_libdir}/pkgconfig/avro-c.pc

%changelog
* Thu Feb 05 2026 Andreas Lindell <andreas.lindell@almondmonitor.com@example.com> - 1.11.3
- Initial EL9 package
