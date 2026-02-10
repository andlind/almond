Name:           libserdes
Version:        1.0
Release:        1%{?dist}
Summary:        Schema Registry C client library

License:        Apache-2.0
URL:            https://github.com/confluentinc/libserdes
Source0:        libserdes-1.0.tar.gz
Source1: 	avro-src-1.11.3.tar.gz
Patch0:		avro-quickstop-fix.patch


BuildRequires:  cmake, gcc, make, libcurl-devel, jansson-devel, librdkafka-devel
Requires:       librdkafka

%description
libserdes is a C library for working with Confluent's Schema Registry and Avro serialization.

%prep
%setup -q -n libserdes-1.0
tar xzf %{SOURCE1}
mv avro-src-1.11.3/lang/c avro-c
cd avro-c/examples
%patch 0 -p0
cd ../..

%build
mkdir -p avro-c/build
cd avro-c/build
cmake .. -DCMAKE_INSTALL_PREFIX=%{_builddir}/avro-c/install -DBUILD_SHARED_LIBS=ON
make
make install

# Symlink libavro.so to a standard location for linker visibility
mkdir -p %{_builddir}/lib
ln -sf %{_builddir}/avro-c/install/lib64/libavro.so %{_builddir}/lib/libavro.so

# Set environment variables so libserdes can find Avro during configure
export CFLAGS="-I%{_builddir}/avro-c/install/include"
export LDFLAGS="-L%{_builddir}/lib"
export PKG_CONFIG_PATH="%{_builddir}/avro-c/install/lib/pkgconfig"

cd ../..
./configure --prefix=%{_prefix}
make

%install
make install DESTDIR=%{buildroot}

# Include Avro C headers and libs
mkdir -p %{buildroot}/usr/include
cp -a %{_builddir}/avro-c/install/include/* %{buildroot}/usr/include/

mkdir -p %{buildroot}/usr/lib
cp -a %{_builddir}/avro-c/install/lib64/libavro.so* %{buildroot}/usr/lib/

%files
%license LICENSE

# Shared libs
/usr/lib/libserdes.so*
/usr/lib/libavro.so*

# Static lib
/usr/lib/libserdes.a

# Headers
/usr/include/libserdes/*.h
/usr/include/avro.h
/usr/include/avro/*.h

# pkg-config
/usr/lib/pkgconfig/serdes.pc
/usr/lib/pkgconfig/serdes-static.pc

%changelog
* Tue Sep 23 2025 Andreas Lindell <andreas.lindell@almond.monitor.com> - 1.0-1
- Initial RPM build
- Building libserdes with avro
