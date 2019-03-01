domainjoin-cli(8) - Join a host to an Active Directory domain
=============================================================

SYNOPSIS
--------

`domainjoin-cli` [<options>] join [<joinoptions>] [<domain>] [<username>] [<password>]

`domainjoin-cli` [<options>] leave [<leaveoptions>] [<username>] [<password>]

`domainjoin-cli` [<options>] query

`domainjoin-cli` [<options>] fixfqdn

`domainjoin-cli` [<options>] setname <name>

`domainjoin-cli` [<options>] configure [<configureoptions>]

`domainjoin-cli` [<options>] get_os_type

`domainjoin-cli` [<options>] get_arch

`domainjoin-cli` [<options>] get_distro

`domainjoin-cli` [<options>] get_distro_version

DESCRIPTION
-----------

**domainjoin-cli** is the BeyondTrust AD Bridge domain join tool. **domainjoin-cli** will join the current machine to an AD domain, and enable the authentication of AD users.

**domainjoin-cli** offers fine-grained control over modifications to system configuration files that are typically required during a join, such as editing `/etc/nsswitch.conf` or the system PAM setup (see JOIN and LEAVE commands).

OPTIONS
-------

`--log` <file>

[INDENT]Log details about the operation to <file>. If <file> is `'.'` logging is directed to the console. By default, operations are logged to `/var/log/domainjoin-cli.log` or `/var/adm/domainjoin-cli.log`.

`--loglevel` <error>|<warning>|<info>|<verbose>

[INDENT]Specifies the level of logging information which should be written to the log file. The default log level is <warning>.

`--help`

[INDENT]Displays brief usage and help information.

`--help-internal`

[INDENT]Displays internal debugging and configuration commands. See CONFIGURATION AND DEBUGGING COMMANDS.

JOIN COMMAND
------------

`domainjoin-cli` [<options>] join [<joinoptions>]  [<domain>] [<username>] [<password>]

Joins the machine to the AD domain <domain> and configures AD authentication.  This operation requires valid AD credentials for <domain> to be specified as <username> and <password>. If <username> or <password> are not specified on the command line, **domainjoin-cli** will prompt for them.

[ENTERPRISE]
For **BeyondTrust AD Bridge Enterprise**, the credentials can be retrieved from a **BeyondTrust Password Safe** instance; see the `--configFile` option.
[/ENTERPRISE]

The join command supports the following options:

`--ou` <organizational_unit>

[INDENT]Joins the machine to the OU <organizational_unit> instead of the default "Computers" OU. The OU to which a machine is joined determines which users will be able to authenticate against the machine and which group policies will be applied.  **Note:** Group policy support is an **BeyondTrust AD Bridge Enterprise** feature.  If the <organizational_unit> is not supplied, you will be prompted for it. `--` can be used to indicate the OU value is not supplied and to separate it from subsequent parameters.
[ENTERPRISE]

`--assumeDefaultCell` `auto`|`no`|`force`

[INDENT]Enable/disable assume default cell mode.  In assume default cell mode, user and group information is not read from cell objects but from the user and group objects directly and global catalog searches are not used. This supports joining to a domain which does not have any named/default cells. If set to `auto`, enable this mode when no cells are found. If set to `force` enable this mode even if named/default cells exist.
[/ENTERPRISE]

`--assumeDefaultDomain` `yes`|`no`

[INDENT]If enabled, apply the default domain prefix to user and group names at logon. Enabling this allows logons to use short names, i.e. `account` in addition to `domainname\\account`.  This can also be set via `/opt/pbis/bin/config` or via Group Policy (see `Authorization and Identification - Lsass: Prepend default domain name for AD users and groups`).

[ENTERPRISE]

`--configFile` <configfile>

[INDENT]Retrieve the credentials needed to join the domain from a **BeyondTrust Password Safe** instance via the parameters in the specified config file.  A template config file is provided in `/etc/pbis/djpbps.config.template`.

[/ENTERPRISE]
`--userDomainPrefix` <shortdomainname>

[INDENT]Domain short name prefix to be used when `--assumeDefaultDomain` is enabled.

`--uac-flags` <flags>

[INDENT]The `userAccountControl` flags on the created computer account.  Defaults to `WORKSTATION_TRUST_ACCOUNT` (0x1000)

`--trustEnumerationWaitSeconds` <seconds>

[INDENT]The number of seconds to wait for trust enumeration on startup to complete. Range of 0 (disabled) to 1000 seconds.

[ENTERPRISE]
`--unprovisioned` `auto`|`no`|`force`

[INDENT]Enable/disable unprovisioned mode. In unprovisioned mode, user/group UIDs are computed from their Active Directory SIDs, and local settings are used for shell and home directory. If set to `auto`, enable this mode when no cells are found. If set to `force`, enable this mode even if named/default cells exist. The default setting is `no`.
[/ENTERPRISE]

`--enable` <module>

[INDENT]Explicitly enables the configuration of <module> during the join operation.

`--disable` <module>

[INDENT]Explicitly disables the configuration of <module> during the join operation.

[INDENT]**Note** that some modules are necessary for the proper operation of BeyondTrust AD Bridge while joined to AD. If you attempt to disable such a module, **domainjoin-cli** will refuse to proceed with a join operation.

[INDENT]For some modules, it is possible to make the relevant configuration changes by hand; **domainjoin-cli** will inform you of the necessary changes and will proceed with the module disabled if it detects that the changes have been made.

`--details` <module>

[INDENT]Provide details about <module> and what specific configuration changes it would perform during a join operation. No actual operation is performed.

`--preview`

[INDENT]Provide a summary of what configuration modules would be run during a join operation. No actual operation is performed.

`--notimesync`

[INDENT]Do not synchronize the computer's time with the domain controller.

`--nohosts`

[INDENT]Disables the `hostname` module. See `DOMAINJOIN MODULES`.

`--nogssapi`

[INDENT]Remove the `GSSAPIAuthentication` option from the modified sshd config file.

`--ignore-pam`

[INDENT]Disables the `pam` module. See `DOMAINJOIN MODULES`.

LEAVE COMMAND
-------------

`domainjoin-cli` [<options>] leave [leaveoptions] [<username>] [<password>]

Leaves the currently-joined AD domain and deconfigures AD authentication and group policy (where applicable).

In order to disable the machine account in AD, either administrative credentials for <domain> or the same credentials originally used to join the machine must be specified as <username> and <password>. If <password> is not specified on the command line, **domainjoin-cli** will prompt you for it.  

[ENTERPRISE]
For **BeyondTrust AD Bridge Enterprise**, the credentials can be retrieved from a **BeyondTrust Password Safe** instance; see the `--configFile` option.
[/ENTERPRISE]

If no credentials are specified, the machine will no longer behave as a member of <domain> but it's machine account will remain enabled in AD.

The leave command supports the following options:

[ENTERPRISE]
`--configFile`

[INDENT]Retrieve the credentials needed to disable/delete the computer account from a **BeyondTrust Password Safe** instance via the parameters in the specified config file.  A template config file is provided in `/etc/pbis/djpbps.config.template`.

`--keepLicense`

[INDENT]Retain the license information after the computer leaves the domain. By default, leaving a domain releases the associated license key.
[/ENTERPRISE]

`--deleteAccount` <username> [<password>]

[INDENT]Delete the computer account after the computer leaves the domain. If <password> is not specified on the command line, **domainjoin-cli** will prompt you for it.

[ENTERPRISE]
`--deleteAccount` `--configFile` [<file>]

[INDENT]Delete the computer account after the computer leaves the domain using credentials retrieved from a **BeyondTrust Password Safe** instance.
[/ENTERPRISE]

`--enable` <module>

[INDENT]Explicitly enables the configuration of <module> during the leave operation.

`--disable` <module>

[INDENT]Explicitly disables the configuration of <module> during the leave operation.

`--details` <module>

[INDENT]Provide details about <module> and what specific configuration changes it would perform during the leave operation. No actual operation is performed.

`--preview`

[INDENT]Provide a summary of what configuration modules would be run during the leave operation. No actual operation is performed.

`--advanced`

[INDENT]Turns on debugging information during the leave operation and provides more verbose output when using [`--preview`](#option-preview). This is generally only helpful when diagnosing unusual system or network configuration issues.


DOMAINJOIN MODULES
------------------

`domainjoin-cli` includes the following modules:

* `bash` - fixes the bash prompt for backslashes in usernames

* `dsplugin` - enables the directory services plugin on a Mac computer

* `firewall` - opens ports to the domain controller

* `gdm` - fixes gdm presession script for spaces in usernames

* `join` - joins the computer to Active Directory

* `krb5` - configures krb5.conf

* `hostname` - sets the computer hostname

* `lam-auth` - configures LAM for Active Directory authentication

* `leave` - deletes the machine account in Active Directory

* `pam-mode` - switches authentication from LAM to PAM

* `pam` - configures pam.d and pam.conf

* `nsswitch` - enables or disables nsswitch module

* `ssh` - configures ssh and sshd

* `start` - starts services after configuration

* `stop` - stops services so that the system can be configured


QUERY COMMAND
-------------

`domainjoin-cli` [<options>] query

Displays information about the currently-joined AD domain and OU. If the computer is not joined to a domain, only the hostname is shown.


FIXFQDN COMMAND
---------------

`domainjoin-cli` [<options>] fixfqdn

Makes local configuration modifications necessary to ensure that the fully-qualified domain name of the machine is forward- and backward-resolvable. This can work around domain join issues on networks with sub-optimal DNS setups.

SETNAME COMMAND
---------------

`domainjoin-cli` [<options>] setname <hostname>

Changes the hostname of this machine to <hostname>. As it is necessary to have a unique, non-generic name before joining AD, this operation is provided as a convenient way to quickly rename this computer before performing a join.


CONFIGURATION AND DEBUGGING COMMANDS
------------------------------------

`domainjoin-cli` includes commands for debugging the domain-join process and for configuring or preconfiguring a module.  For example, run the configure command to preconfigure a system before you join a domain—a useful strategy when you are deploying BeyondTrust AD Bridge in a virtual environment and you need to preconfigure the nsswitch, ssh, or PAM module of the target computers to avoid restarting them after they are added to the domain.

The following `configure` commands are supported.

**Note**: the `--testprefix` option supports testing of system configuration file changes. If supplied, the `--testprefix` <directory> will be prepended to the path of the configuration file target. .e.g. `configure` `--enable` `--testdirectory` `/testconfig` `nsswitch`  will make changes to the `/testconfig/etc/nsswitch.conf` file instead of `/etc/nsswitch.conf` file.

* `configure` `--enable` | `--disable` [--testprefix <directory>] pam

* `configure` `--enable` | `--disable` [--testprefix <directory>] nsswitch

* `configure` `--enable` | `--disable` [--testprefix <directory>] ssh

* `configure` `--enable` | `--disable` [--testprefix <directory>] [--long <longdomain>] [--short <shortdomain>] krb5

* `configure` `--enable` | `--disable` [--testprefix <directory>] eventfwdd

* `configure` `--enable` | `--disable` [--testprefix <directory>] reapsysld


The following debug commands display information about the host machine and OS:

* `get_os_type`

* `get_arch`

* `get_distro`

* `get_distro_version`

EXAMPLES
--------

Example invocations of **domainjoin-cli** and their effects follow:

`$ domainjoin-cli join sales.my-company.com Administrator rosebud`

[INDENT]Joins the AD domain `sales.my-company.com` using `Administrator` as the username and `rosebud` as the password. This is the typical join scenario.

`$ domainjoin-cli --log . leave`

[INDENT]Leaves the current AD domain without attempting to disable the machine account as no user credentials were specified. Information about the process will be logged to the console at the default logging level.

`$ domainjoin-cli join --disable nsswitch sales.my-company.com Administrator`

[INDENT]Joins the AD domain `sales.my-company.com` using `Administrator` as the username and prompting for the password. If possible, nsswitch configuration will not be modified.

`$ domainjoin-cli join --preview sales.my-company.com Administrator rosebud`

[INDENT]Show what configuration modules would be run when joining the AD domain `sales.my-company.com`.

`$ domainjoin-cli join --details pam sales.my-company.com Administrator rosebud`

[INDENT]Show what changes would be made to the system by the `pam` module when joining the AD domain `sales.my-company.com`.


SEE ALSO
--------

The full documentation is available online at https://github.com/BeyondTrust/pbis-open/wiki/Documentation and https://www.beyondtrust.com/resources/education/documentation/?subcategory=ad-bridge

VERSION
-------

Version 9.0 +.
