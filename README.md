# openmediavault-restic

Restic plugin for OpenMediaVault

![Screenshot_20230225_193445](https://user-images.githubusercontent.com/31949120/221347571-56dba5cb-eb32-4a0f-a73a-cbe31bcfd787.png)
![Screenshot_20230225_193500](https://user-images.githubusercontent.com/31949120/221347573-9c041b85-2768-47d1-9c3c-18d6250c067a.png)
![Screenshot_20230225_193508](https://user-images.githubusercontent.com/31949120/221347574-bb8dd40c-bd64-4624-96f1-3040c8b9b38f.png)
![Screenshot_20230225_193517](https://user-images.githubusercontent.com/31949120/221347575-7d54adef-3589-4414-a4b5-f25cbe151c1a.png)
![Screenshot_20230225_193526](https://user-images.githubusercontent.com/31949120/221347576-e1038c70-31da-4ebd-b6ca-22fc975118d0.png)

## Development process

0. Do not use your personal OpenMediaVault server
1. Enable SSH for your development OpenMediaVault server
2. Install build dependencies: `sudo apt install devscripts debhelper build-essential`
3. Clone this repository into the OpenMediaVault server instance, e.g. `git clone https://github.com/xxxx/openmediavault-restic.git /srv/dev-disk-by-uuid-xxxx/Storage/openmediavault-restic`
4. Change directory: `cd srv/dev-disk-by-uuid-xxxx/Storage/openmediavault-restic`

The following now assume you are in the `openmediavault-restic` folder:

### Add a changelog

1. To add a changelog, run e.g. `dch --create v 1.0.0`

### Build and install

The following will install this plugin for development purposes:

0. If installed previously, purge the Debian package: `sudo apt purge openmediavault-restic`
1. Install the Restic dependency: `sudo apt install restic`
2. Mark the shell scripts as executable: `chmod +x usr/share/openmediavault/confdb/create.d/conf.service.restic.sh`
3. Build the Debian package: `debuild -us -uc`
4. Install the Debian package: `sudo dpkg -i ../openmediavault-restic_0.1_all.deb`

It will now be possible to access "Restic" under the "Services" tab.

### Documentation:

* [Development docs](https://docs.openmediavault.org/en/latest/development/index.html)
* [Workbench models](https://github.com/openmediavault/openmediavault/tree/master/deb/openmediavault/workbench/src/app/core/components/intuition/models)
* [diypluginguide3.x](https://github.com/skyajal/diypluginguide3.x) - Note: This is for OpenMediaVault 3, and does not cover SaltStack for deployments. 

### Debugging

It is possible to view potential issues with the plugin via:

* `sudo systemctl status openmediavault-engined.service`
* `sudo /usr/sbin/omv-engined -f` OR `sudo /usr/sbin/omv-engined -d -f`
* `sudo /usr/sbin/omv-confdbadm read conf.service.restic`
    * If you get an error saying e.g. `openmediavault.exceptions.AssertException: The property 'repos.repo.0.passphrase' does not exist in the model 'conf.service.restic'.`, it is because there is an old entry in config.xml which once had the "passphrase" (or any other property). You need to remove it via `sudo nano /etc/openmediavault/config.xml`.
