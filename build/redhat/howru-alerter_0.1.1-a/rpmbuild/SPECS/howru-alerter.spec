%define name howru-alerter
%define version 0.1.1

Summary:	HowRU monitoring alerter
Name:		%{name}	
Version: 	%{version}
Release:	1%{?dist}
Summary:	HowRU alerter

Group:	        Applications/System 	
License:	GPL
URL:		https://github.com/andlind/howru
Source0:	%{name}-%{version}.tar.gz

BuildRequires:	gcc, make
BuildRequires:  libcurl-devel
BuildRequires:  json-c, json-c-devel

Requires: json-c, curl

%description
HowRU alerting system 

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
make install DESTDIR=%{?buildroot} 
mkdir -p %{buildroot}/etc/howru/
mkdir -p %{buildroot}/var/log/howru/
cp alerting.conf %{buildroot}/etc/howru/
cp groups.conf %{buildroot}/etc/howru 
cp servers.conf %{buildroot}/etc/howru
mkdir -p %{buildroot}/opt/howru/plugins
cp mail_notification.sh.erb %{buildroot}/opt/howru/plugins
mkdir -p %{buildroot}/lib/systemd/system/
cp howru-alerter.service %{buildroot}/lib/systemd/system/

%files
%attr(0644,root,root) %config(noreplace) /etc/howru/alerting.conf
%attr(0644,root,root) %config(noreplace) /etc/howru/groups.conf
%attr(0644,root,root) %config(noreplace) /etc/howru/servers.conf
%attr(0770,root,root) /opt/howru/howru-alerter
%attr(0755,root,root) /var/log/howru/
%attr(0755,root,root) /opt/howru/plugins/
%attr(0644,root,root) /lib/systemd/system/howru-alerter.service

%doc

%changelog
* Thu Sep 16 2021 Startup
<andreas.lindell@hotmail.com>
- HowRU rpmbuid set up
