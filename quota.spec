##
# $Id$
##

Name:    quota
Version: 1.1
Release: 1

Summary: Quota utility for displaying remote NFS quotas
Group:   System Environment/Base
License: GPL
Provides: quota

BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}
BuildRequires: /usr/bin/rpcgen

Source0: %{name}-%{version}-%{release}.tgz

%description
Quota is a utility for displaying remote filesystem quota information.
This particular implentation does not report local filesystem quotas
as it is intended to run in a computing center where all user data
resides on shared NFS servers.  In addition, quota only reports
filesystems listed in the quota.conf file to reduce latency in environments 
where there are many remote filesystems that are not configured with quotas.
Quotas are reported in human-readable units: 'K', 'M', and 'T' bytes.

%prep
%setup -n %{name}-%{version}-%{release}.tgz

%build
make CFLAGS="$RPM_OPT_FLAGS"

%install
rm -rf "$RPM_BUILD_ROOT"
mkdir -p "$RPM_BUILD_ROOT/usr/bin"
mkdir -p "$RPM_BUILD_ROOT/usr/man/man1"
mkdir -p "$RPM_BUILD_ROOT/usr/man/man5"
cp quota "$RPM_BUILD_ROOT/usr/bin"
gzip quota.1 quota.conf.5
cp quota.1.gz $RPM_BUILD_ROOT/usr/man/man1
cp quota.conf.5.gz $RPM_BUILD_ROOT/usr/man/man5

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root,0755)
%doc ChangeLog README
%doc README
/usr/bin/quota
/usr/man/man1/quota.1.gz
/usr/man/man5/quota.conf.5.gz
