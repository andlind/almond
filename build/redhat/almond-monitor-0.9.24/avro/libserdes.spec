Name:           libserdes
Version:        6.2.0
Release:        2%{?dist}
Summary:        Confluent Schema Registry C client library

License:        Apache-2.0
URL:            https://github.com/confluentinc/libserdes
Source0:        %{url}/archive/refs/tags/v%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  make
BuildRequires:  librdkafka-devel
BuildRequires:  libcurl-devel
BuildRequires:  openssl-devel
BuildRequires:  zlib-devel
BuildRequires: avro-c-devel

Requires:       librdkafka, avro-c

%description
libserdes is a C client library for the Confluent Schema Registry,
providing serialization and deserialization support for Avro-encoded
Kafka messages.

%prep
%autosetup -n %{name}-%{version}

%build
./configure --prefix=%{_prefix} --libdir=%{_libdir}
%make_build

%install
%make_install

#
# --- Main runtime package ---
#
%files
%license LICENSE
%doc README.md
%{_libdir}/libserdes.so*

#
# --- Development subpackage ---
#
%package devel
Summary: Development files for libserdes
Requires: %{name}%{?_isa} = %{version}-%{release}

%description devel
Development headers and static library for libserdes.

%files devel
%{_includedir}/libserdes/
%{_libdir}/libserdes.a

%changelog
* Mon Feb 16 2026 Andreas Lindell <andreas.lindell@almondmonitor.com> - 6.2.0-2
- Compile with avro 1.12 headers
* Thu Feb 05 2026 Andreas Lindell <andreas.lindell@almondmonitor.com> - 6.2.0-1
- Initial EL9 package
