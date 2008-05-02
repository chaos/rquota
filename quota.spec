##
# $Id$
##

Name:    
Version: 
Release: 

Summary: Quota utility for displaying remote NFS quotas
Group:   System Environment/Base
License: GPL
# we replace the redhat quota program
Conflicts: quota
#BuildRequires: /usr/bin/rpcgen
BuildRequires: lustre

BuildRoot: %{_tmppath}/%{name}-%{version}

Source0: %{name}-%{version}-%{release}.tgz

%description
Quota is a utility for displaying remote filesystem quota information.
This particular implentation does not report local filesystem quotas
as it is intended to run in a computing center where all user data
resides on shared NFS or Lustre servers.  In addition, quota only reports
filesystems listed in the quota.conf file to reduce latency in environments 
where there are many remote filesystems that are not configured with quotas.
Quotas are reported in human-readable units: 'K', 'M', and 'T' bytes.

%prep
%setup

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man1
mkdir -p $RPM_BUILD_ROOT%{_mandir}/man5

cp quota $RPM_BUILD_ROOT%{_bindir}
cp repquota $RPM_BUILD_ROOT%{_bindir}
cp quota.1 $RPM_BUILD_ROOT%{_mandir}/man1
cp repquota.1 $RPM_BUILD_ROOT%{_mandir}/man1
cp quota.conf.5 $RPM_BUILD_ROOT%{_mandir}/man5

%clean
rm -rf "$RPM_BUILD_ROOT"

%files
%defattr(-,root,root,0755)
%doc ChangeLog README
%doc README
%{_bindir}/quota
%{_bindir}/repquota
%{_mandir}/man1/quota.1*
%{_mandir}/man1/repquota.1*
%{_mandir}/man5/quota.conf.5*
