%define name howru-monitor
%define version 0.3.2

Summary:	HowRU monitoring scheduler and api
Name:		%{name}	
Version: 	%{version}
Release:	1%{?dist}
Summary:	HowRU monitoring

Group:	        Applications/System 	
License:	GPL
URL:		https://github.com/andlind/howru
Source0:	%{name}-%{version}.tar.gz

BuildRequires:	gcc
BuildRequires:  make
Requires:	python3, python3-flask, ksh, sysstat 

%description
HowRU scheduler and API, compatible with Nagios plugins

%global debug_package %{nil} 

%prep
%setup -q

%build
make %{?_smp_mflags}

%install
make install DESTDIR=%{?buildroot} 
mkdir -p %{buildroot}/etc/howru/
mkdir -p %{buildroot}/var/log/howru/
mkdir -p %{buildroot}/opt/howru/
mkdir -p %{buildroot}/opt/howru/plugins/
mkdir -p %{buildroot}/opt/howru/templates/
mkdir -p %{buildroot}/opt/howru/www
mkdir -p %{buildroot}/lib/systemd/system/
cp howruc.conf %{buildroot}/etc/howru/
cp plugins.conf %{buildroot}/etc/howru/ 
cp gardener.py %{buildroot}/opt/howru/
cp metrics.template %{buildroot}/opt/howru/templates/
cp -r www/* %{buildroot}/opt/howru/www/
cp -r system/* %{buildroot}/lib/systemd/system/
cp -r plugins/* %{buildroot}/opt/howru/plugins/

%files
%attr(0644,root,root) %config(noreplace) /etc/howru/howruc.conf
%attr(0644,root,root) %config(noreplace) /etc/howru/plugins.conf
%attr(0755,root,root) /opt/howru/gardener.py
%attr(0644,root,root) /opt/howru/templates/metrics.template
%attr(0770,root,root) /opt/howru/howru-scheduler
%attr(0755,root,root) /var/log/howru/
%attr(0755,root,root) /opt/howru/plugins/
%attr(0644,root,root) /lib/systemd/system/howru-scheduler.service
%attr(0644,root,root) /lib/systemd/system/howru-api.service
%defattr(755,root,root,755)
/opt/howru/www/*

%doc

%changelog
* Fri Feb 03 2023 Update
<andreas.lindell@hotmail.com>
- Prometheus export
- Update small gui
- Bugg fixes
* Tue Nov 29 2022 Metrics collection
<andreas.lindell@hotmail.com>
- Option to collect metrics instead of json
* Wed Jun 29 2022 Docker logs option
<andreas.lindell@hotmail.com>
- Option to log to stdout
* Wed Jun 22 2022 Api server flag added
<andreas.lindell@hotmail.com>
- Option to use servername argument with api
* Fri Jun 03 2022 Store feature
<andreas.lindell@hotmail.com>
- Option to store plugin results
* Tue May 31 2022 Serverfile
<andreas.lindell@hotmail.com>
- Adding server file feature
* Wed Feb 09 2022 Multimode
<andreas.lindell@hotmail.com>
- Multimode added
* Fri Sep 10 2021 Startup
<andreas.lindell@hotmail.com>
- HowRU rpmbuid set up
