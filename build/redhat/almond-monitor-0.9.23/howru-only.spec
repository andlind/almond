%global pkgname howru
%global pkgver 0.9.23
%global _hardened_build 1
%global _annotated_build 1
%global _build_id_links none

Name:           %{pkgname}
Version:        %{pkgver}
Release:        1%{?dist}
Summary:        Howru API, proxy and frontend for Almond Monitor
License:        MIT
Source0:        almond-monitor-%{version}.tar.gz

BuildArch: x86_64

Requires:       python3
Requires:       python3-yaml
Requires:       python3-simplejson
Requires:       python3-flask
Requires:       python3-gunicorn
Requires:       python3-cryptography
Requires(pre):  shadow-utils

%description
Howru API, proxy and frontend for Almond Monitor.

%prep
%setup -q -n almond-monitor-%{version}

%install
# Base directories
install -d %{buildroot}/etc/almond
install -d %{buildroot}/var/log/almond
install -d %{buildroot}/opt/almond
install -d %{buildroot}/opt/almond/www
install -d %{buildroot}/opt/almond/utilities
install -d %{buildroot}/usr/lib/systemd/system

# Config files
install -m 0644 almond.conf %{buildroot}/etc/almond/almond.conf
install -m 0644 aliases.conf %{buildroot}/etc/almond/aliases.conf
install -m 0644 users.conf %{buildroot}/etc/almond/users.conf
install -m 0600 auth2fa.enc %{buildroot}/etc/almond/auth2fa.enc

# Main executable
install -m 0750 howru %{buildroot}/opt/almond/howru

# Web files
cp -r www/* %{buildroot}/opt/almond/www/

# API helper
install -m 0755 rs.sh %{buildroot}/opt/almond/www/api/rs.sh

# Systemd service
install -m 0644 system/howru.service %{buildroot}/usr/lib/systemd/system/howru.service

# Utilities
cp -r utilities/* %{buildroot}/opt/almond/utilities/

# Enable mods
install -d %{buildroot}/opt/almond/www/api/mods/enabled
install -m 0750 www/api/mods/modxml.py %{buildroot}/opt/almond/www/api/mods/enabled/modxml.py
install -m 0750 www/api/mods/modyaml.py %{buildroot}/opt/almond/www/api/mods/enabled/modyaml.py

%files
%dir /opt/almond
/opt/almond/www
/opt/almond/utilities
%attr(0750,almond,almond) /opt/almond/howru
%exclude /opt/almond/www/api/rs.sh
%exclude /opt/almond/www/api/mods/enabled/modxml.py
%exclude /opt/almond/www/api/mods/enabled/modyaml.py

%attr(0755,almond,almond) /opt/almond/www/api/rs.sh
%attr(0750,almond,almond) /opt/almond/www/api/mods/enabled/modxml.py
%attr(0750,almond,almond) /opt/almond/www/api/mods/enabled/modyaml.py

%config(noreplace) %attr(0644,almond,almond) /etc/almond/almond.conf
%config(noreplace) %attr(0644,almond,almond) /etc/almond/users.conf
%config(noreplace) %attr(0644,almond,almond) /etc/almond/aliases.conf
%config(noreplace) %attr(0600,almond,almond) /etc/almond/auth2fa.enc

%dir %attr(0755,almond,almond) /var/log/almond
/usr/lib/systemd/system/howru.service

%pre
/usr/bin/getent group almond >/dev/null || /usr/sbin/groupadd -r almond
if ! /usr/bin/getent passwd almond >/dev/null ; then
    /usr/sbin/useradd -r -g almond -d /opt/almond -s /sbin/nologin \
        -c "User running the Almond monitoring process" almond
fi

%post
%systemd_post howru.service

%preun
%systemd_preun howru.service

%postun
%systemd_postun_with_restart howru.service

# Only delete user on final removal
if [ $1 -eq 0 ]; then
    /usr/sbin/userdel almond >/dev/null 2>&1 || :
fi

%changelog
* Thu Feb 05 2026 0.9.23
<andreas.lindell@almondmonitor.com>
- Howru only installation
