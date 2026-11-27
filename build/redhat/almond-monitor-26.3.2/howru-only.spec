%global pkgname howru
%global pkgver 26.3.2
%global _hardened_build 1
%global _annotated_build 1
%global _build_id_links none

Name:           %{pkgname}
Version:        %{pkgver}
Release:        1%{?dist}
Summary:        HowRU API, proxy and frontend for Almond Monitor
License:        MIT
Source0:        almond-monitor-%{version}.tar.gz

BuildArch:      x86_64
BuildRequires:  systemd-rpm-macros

Requires:       python3
Requires:       python3-yaml
Requires:       python3-simplejson
Requires:       python3-flask
Requires:       python3-gunicorn
Requires:       python3-cryptography
Requires:       python3-jose
Requires:       python3dist(pyotp)
Requires:       python3dist(qrcode)
Requires:       python3dist(pillow)
Requires(pre):  shadow-utils
Requires(pre):  /usr/bin/getent
Requires(post): /usr/bin/python3
%{?systemd_requires}

%description
HowRU API, proxy and frontend for Almond Monitor.

%prep
%setup -q -n almond-monitor-%{version}

%install
# Base directories
install -d %{buildroot}%{_sysconfdir}/almond
install -d %{buildroot}/var/log/almond
install -d %{buildroot}/opt/almond
install -d %{buildroot}/opt/almond/www
install -d %{buildroot}/opt/almond/utilities
install -d %{buildroot}%{_unitdir}

# Configuration: enrollment data and generated keys are not package payloads.
install -m 0644 almond.conf %{buildroot}%{_sysconfdir}/almond/almond.conf
install -m 0644 aliases.conf %{buildroot}%{_sysconfdir}/almond/aliases.conf
install -m 0644 proxyalert.conf %{buildroot}%{_sysconfdir}/almond/proxyalert.conf
install -m 0644 users.conf %{buildroot}%{_sysconfdir}/almond/users.conf

# Launcher and the per-installation secrets initializer.
install -m 0750 howru %{buildroot}/opt/almond/howru
install -m 0755 howru-init-secrets %{buildroot}/opt/almond/howru-init-secrets

# Web files
cp -r www/* %{buildroot}/opt/almond/www/

# API helper
install -m 0755 rs.sh %{buildroot}/opt/almond/www/api/rs.sh

# Systemd service
install -m 0644 system/howru.service %{buildroot}%{_unitdir}/howru.service

# Utilities
cp -r utilities/* %{buildroot}/opt/almond/utilities/

# Enable mods
install -d %{buildroot}/opt/almond/www/api/mods/enabled
install -m 0750 www/api/mods/modxml.py %{buildroot}/opt/almond/www/api/mods/enabled/modxml.py
install -m 0750 www/api/mods/modyaml.py %{buildroot}/opt/almond/www/api/mods/enabled/modyaml.py

%files
%defattr(-,root,root,-)
%dir /opt/almond
/opt/almond/utilities
%attr(0750,almond,almond) /opt/almond/howru
%attr(0755,root,root) /opt/almond/howru-init-secrets

# List the web tree once. Retain modes assigned during installation.
%defattr(-,almond,almond,-)
/opt/almond/www
%defattr(-,root,root,-)

# The workers create their enrollment, SQLite and lock files in this directory.
%dir %attr(0750,almond,almond) %{_sysconfdir}/almond
%config(noreplace) %attr(0644,almond,almond) %{_sysconfdir}/almond/almond.conf
%config(noreplace) %attr(0644,almond,almond) %{_sysconfdir}/almond/proxyalert.conf
%config(noreplace) %attr(0644,almond,almond) %{_sysconfdir}/almond/users.conf
%config(noreplace) %attr(0644,almond,almond) %{_sysconfdir}/almond/aliases.conf

# Preserve the registration used by older packages without shipping enrollments.
%ghost %config(noreplace,missingok) %attr(0600,almond,almond) %{_sysconfdir}/almond/auth2fa.enc

%dir %attr(0755,almond,almond) /var/log/almond
%attr(0644,root,root) %{_unitdir}/howru.service

%pre
/usr/bin/getent group almond >/dev/null || /usr/sbin/groupadd -r almond
if ! /usr/bin/getent passwd almond >/dev/null ; then
    /usr/sbin/useradd -r -g almond -d /opt/almond -s /sbin/nologin \
        -c "User running the Almond monitoring process" almond
fi

%post
# Older packages installed an empty enrollment placeholder. Remove only that
# empty regular file when no key file exists; preserve all non-empty data.
if [ ! -e %{_sysconfdir}/almond/howru-secrets.env ] && \
   [ ! -L %{_sysconfdir}/almond/howru-secrets.env ] && \
   [ -f %{_sysconfdir}/almond/auth2fa.enc ] && \
   [ ! -L %{_sysconfdir}/almond/auth2fa.enc ] && \
   [ ! -s %{_sysconfdir}/almond/auth2fa.enc ]; then
    rm -f %{_sysconfdir}/almond/auth2fa.enc || exit 1
fi

# Runs on fresh installs and upgrades. Existing valid keys are preserved.
/opt/almond/howru-init-secrets \
    --config-dir %{_sysconfdir}/almond || exit 1

# Systemd reads this file as root and passes the values to Gunicorn workers.
chown root:root %{_sysconfdir}/almond/howru-secrets.env || exit 1
chmod 0600 %{_sysconfdir}/almond/howru-secrets.env || exit 1

# Correct the old packaged enrollment permissions without replacing its content.
if [ -f %{_sysconfdir}/almond/auth2fa.enc ] && \
   [ ! -L %{_sysconfdir}/almond/auth2fa.enc ]; then
    chown almond:almond %{_sysconfdir}/almond/auth2fa.enc || exit 1
    chmod 0600 %{_sysconfdir}/almond/auth2fa.enc || exit 1
fi

%systemd_post howru.service

%preun
%systemd_preun howru.service

%postun
%systemd_postun_with_restart howru.service

# Keep the service account: persistent data and other Almond packages may use it.

%changelog
* Fri Nov 27 2026 26.3.2
<andreas.lindell@almondmonitor.com>
- New templates, refined GUI
- Updated secrets control
- Buggfixes
* Mon Nov 09 2026 26.3.1
<andreas.lindell@almondmonitor.com>
- Refactor of admin_page
- Buggfixea
* Mon Nov 02 2026 26.3.0
<andreas.lindell©almondmonitor.com>
- Template updates to view metrics
- Buggfixes
* Mon Oct 05 2026 26.2.0
<andreas.lindell@almondmonitor.com>
- admin_page update for labels
- Buggfixes
* Wed Sep 16 2026 26.1.2
<andreas.lindell@almondmonitor.com>
- Update admin_page and templates
* Thu Sep 03 2026 26.1.1-2
<andreas.lindell@almondmonitor.com>
- Adding HowRU proxy alerting
* Thu Jul 16 2026 0.9.30-2
<andreaslindell@almondmonitor.com>
- HowRU API for labels
- Updated templates
* Tue Apr 21 2026 0.9.28
<andreas.lindell@almondmonitor.com>
- Updated look howru
- HowRU iam and roles
- Minor fixes
* Wed Mar 25 2026 0.9.26
<andreas.lindell@almondmonitor.com>
- Receiver function for almond posts
- Otel API integration and exporter
* Mon Mar 16 2026 0.9.25
<andreas.lindell@almondmonitor.com>
- Buggfix API calls
* Tue Feb 17 2026 0.9.24
<andreas.lindell@almondmonitor.com>
- New version
* Thu Feb 05 2026 0.9.23
<andreas.lindell@almondmonitor.com>
- Howru only installation

