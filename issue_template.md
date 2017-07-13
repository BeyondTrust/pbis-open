**Version**: 8.5.4.334
**OS/Distro**: Ubuntu desktop 16.04 - unity desktop
**Issue/Impact**:
*Note: replace content with your own*
When reporting an issue it's important that we have as much detail as you can provide. The following is a list of commands to check.
 1. systemctl status lwsmd.service
 2. /opt/pbis/bin/lwsm list
 3. /opt/pbis/domainjoin-cli query
 4. pbis status
 5. /opt/pbis/bin/enum-users
 6. attach logs
  - /opt/pbis/bin/lwsm set-log-target -p lsass -  file /tmp/lsass.log
  - /opt/pbis/bin/lwsm set-log-level -p lsass - debug
  - attach log

**Output/Error**:


**Steps to Reproduce**:
 1. install command
 2. Domainjoin command
 3. Command that returns issue
