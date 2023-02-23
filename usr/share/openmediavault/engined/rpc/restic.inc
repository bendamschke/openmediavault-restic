<?php
/**
 * Copyright (C) 2009-2013 Volker Theile <volker.theile@openmediavault.org>
 * Copyright (C) 2013-2023 OpenMediaVault Plugin Developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

class OMVRpcServiceRestic extends \OMV\Rpc\ServiceAbstract
{
    private $getProgram = '/usr/bin/restic';
    private $envFile = '/etc/restic/restic-envvar-';

    public function getName()
    {
        return "Restic";
    }

    public function initialize()
    {
        // Restic main settings
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");

        // Restic versioning
        $this->registerMethod("updateRestic");
        $this->registerMethod("getResticVersion");

        // Restic repositories
        $this->registerMethod("getRepoList");
        $this->registerMethod("enumerateRepoCandidates");
        $this->registerMethod("getRepo");
        $this->registerMethod("setRepo");
        $this->registerMethod("deleteRepo");

        // Restic environment variables
        $this->registerMethod('getEnvVarList');
        $this->registerMethod('getEnvVar');
        $this->registerMethod('setEnvVar');
        $this->registerMethod('deleteEnvVar');
    }

    public function getSettings($params, $context)
    {
        // Validate the RPC caller context
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Get the configuration object from the database "config.xml" 
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.restic.settings");

        // Get the current Restic version, and do not write it to the databsse "config.xml"
        $version = $this->getResticVersion($params, $context);
        $object->add('version', 'string', $version);

        // Return the configuration object for the "Webgui"
        return $object->getAssoc();
    }


    public function setSettings($params, $context)
    {
        // Validate the RPC caller context
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.restic.setsettings');

        // Get the configuration object from the database "config.xml" 
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.restic.settings");

        // Write to the configuration object, ensuring that the keys match that of the properties within conf.service.restic.json
        // usr/share/openmediavault/workbench/component.d/omv-services-restic-settings-form-page.yaml calls `getSettings`
        $object->setFlatAssoc([
            "enable" => $params['enable'],
        ]);

        // Write the configuration object to the database "config.xml"
        $db->set($object);

        // Return the configuration object for the "Webgui"
        return $object->getAssoc();
    }

    public function updateRestic($params, $context)
    {
        // Validate the RPC caller context
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);

        // Make a call to the Restic application to self-update
        $cmdArgs = [];
        $cmdArgs[] = $this->getProgram;
        $cmdArgs[] = "self-update";
        $cmd = new \OMV\System\Process($cmdArgs);
        $cmd->setRedirect2to1();
        $cmd->execute($output, $exitStatus);

        // Let the caller know if the command was successful (for the "successUrl" event)
        return (0 == $exitStatus) ? TRUE : FALSE;
    }

    public function getResticVersion($params, $context)
    {
        // Make a call to the Restic application, with the `version` argument
        $cmdArgs = [];
        $cmdArgs[] = $this->getProgram;
        $cmdArgs[] = "version | awk '{print $2}'";
        $cmd = new \OMV\System\Process($cmdArgs);
        $cmd->setRedirect2to1();
        $cmd->execute($output);

        return $output[0];
    }


    public function getRepoList($params, $context)
    {
        // Validate the RPC caller context.
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.getlist');

        // Get the configuration objects from the database "config.xml" 
        $db = \OMV\Config\Database::getInstance();
        $repos = $db->get('conf.service.restic.repo');
        // $snapshots = $db->get('conf.service.restic.snapshot');

        // Add additional share information
        $objectsAssoc = [];
        foreach ($repos as $repok => &$repov) {
            // Add the new property 'sharedfoldername'.
            $repov->add('sharedfoldername', 'string', gettext("n/a"));
            // Get the shared folder configuration object.
            $sfObject = $db->get(
                'conf.system.sharedfolder',
                $repov->get('sharedfolderref')
            );
            // Update the 'sharedfoldername' property.
            $repov->set('sharedfoldername', $sfObject->get('name'));

            // Get how many snapshots have been created for this repository
            $snapshotCount = 0;
            // foreach ($snapshots as $snapshotk => $snapshotv) {
            //     if ($snapshotv->get('reporef') == $repov->get('uuid')) {
            //         $snapshotCount++;
            //     }
            // }
            $repov->add('snapshotCount', 'integer', $snapshotCount);

            $objectsAssoc[] = $repov->getAssoc();
        }
        // Filter the result.
        return $this->applyFilter(
            $objectsAssoc, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']
        );
    }

    public function enumerateRepoCandidates($params, $context)
    {
        // Validate the RPC caller context.
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Get the saved repositories.
        $db = \OMV\Config\Database::getInstance();
        $repos = $db->getAssoc('conf.service.restic.repo');

        // If we are creating an environment variable, we need to allow the user to add
        // an environment variable for all repositories
        if ($params['shared'] == true) {
            $repos[] = [
                'name' => 'All Repositories',
                'uuid' => 'shared'
            ];
        }
        return $repos;
    }

    public function getRepo($params, $context)
    {
        // Validate the RPC caller context.
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        return $db->getAssoc('conf.service.restic.repo', $params['uuid']);
    }

    public function setRepo($params, $context)
    {
        // Validate the RPC caller context.
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.restic.setrepo');

        // Get the repository via its UUID as per the validation method params RPC datamodel
        $repo = new \OMV\Config\ConfigObject('conf.service.restic.repo');

        // Remove spaces from name
        $params['name'] = str_replace(' ', '_', $params['name']);
        $repo->setAssoc($params);

        // Check if this is a new repository
        $isNew = $repo->isNew();
        $db = \OMV\Config\Database::getInstance();

        if (TRUE === $isNew) {
            // Ensure that the name and shared folder references are unique
            // TODO: This might be where we get a non-user-friendly exception raised in the UI
            $db->assertIsUnique($repo, 'name');
            $db->assertIsUnique($repo, 'sharedfolderref');
        }

        // Initalise the repository, unless we opted to skip initalisation
        // if ($params['skipinit'] == true) { }
        $path = \OMV\Rpc\Rpc::call('ShareMgmt', 'getPath', [
            'uuid' => $params['sharedfolderref']
        ], $context);

        $cmdArgs = [];
        $cmdArgs[] = $this->getProgram;
        $cmdArgs[] = 'init --repo';
        $cmdArgs[] = escapeshellarg($path);

        // Attempt to initialise the repository
        $cmd = new \OMV\System\Process($cmdArgs);
        $cmd->setEnv("RESTIC_PASSWORD", escapeshellarg($params['passphrase']));
        if (!$isNew) {
            $cmd->setEnvFromFile($this->envFile . $params['uuid']);
        }
        $cmd->setEnvFromFile($this->envFile . 'shared');
        $cmd->setRedirect2to1();
        $cmdLine = $cmd->getCommandLine();
        $cmd->execute($output, $exitStatus);

        // If we cannot, we should raise an exception
        if (0 !== $exitStatus) {
            throw new \OMV\ExecException($cmdLine, $output);
        }

        // Remove skipinit and passphrase parameters
        $repo->remove('skipinit');
        // Set the configuration object.
        $db->set($repo);
        // Return the configuration object.
        return $repo->getAssoc();
    }

    public function deleteRepo($params, $context)
    {
        // Validate the RPC caller context.
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();

        // Ensure that this repository does not have any snapshots, before deleting it
        // TODO: In UI, when clicking the delete button, have a pop-up with a checkbox to delete all snapshots
        // $snapshots = $db->get('conf.service.restic.snapshot');
        // $found = false;
        // foreach ($snapshots as $snapshot) {
        //     $reporef = $snapshot->get('reporef');
        //     if ($reporef == $params['uuid']) {
        //         $found = true;
        //         break;
        //     }
        // }
        // if ($found) {
        //     throw new \OMV\ExecException(gettext(
        //       "Repository is in use. Please delete the snapshots associated with this repository first."
        //     ));
        // }

        $repo = $db->get('conf.service.restic.repo', $params['uuid']);

        // Delete repository
        $path = \OMV\Rpc\Rpc::call('ShareMgmt', 'getPath', [
            'uuid' => $repo->get('sharedfolderref')
        ], $context);
        $resticRepoFileOrFolder = array("config", "data", "index", "keys", "locks", "snapshots");

        $cmdArgs = [];
        $cmdArgs[] = "rm -rf";
        foreach ($resticRepoFileOrFolder as $fileOrFolder) {
            $cmdArgs[] = escapeshellarg($path . "/" . $fileOrFolder);
        };

        $cmd = new \OMV\System\Process($cmdArgs);
        $cmd->setQuiet(TRUE);
        $cmd->execute($output);

        // Delete the configuration object.
        $db->delete($repo);
        // Return the deleted configuration object.
        return $repo->getAssoc();
    }

    public function getEnvVarList($params, $context)
    {
        // Validate the RPC caller context.
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.getlist');

        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $envvars = $db->get('conf.service.restic.envvar');
        // Add additional environment variable informations.
        $objectsAssoc = [];
        foreach ($envvars as $envvark => &$envvarv) {
            // Add the new property 'sharedfoldername'.
            $envvarv->add('reponame', 'string', gettext("n/a"));
            // Get the shared folder configuration object.
            if ($envvarv->get('reporef') == 'shared') {
                $envvarv->set('reponame', 'All Repositories');
            } else {
                try {
                    $repoObject = $db->get('conf.service.restic.repo', $envvarv->get('reporef'));
                    // Update the 'reponame' property.
                    $envvarv->set('reponame', $repoObject->get('name'));
                } catch (Exception $e) {
                }
            }
            $objectsAssoc[] = $envvarv->getAssoc();
        }
        // Filter the result.
        return $this->applyFilter(
            $objectsAssoc, $params['start'], $params['limit'],
            $params['sortfield'], $params['sortdir']
        );
    }

    public function getEnvVar($params, $context)
    {
        // Validate the RPC caller context.
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        return $db->getAssoc('conf.service.restic.envvar', $params['uuid']);
    }

    public function setEnvVar($params, $context)
    {
        // Validate the RPC caller context.
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.restic.setenvvar');
        // Prepare the configuration object.
        $envvar = new \OMV\Config\ConfigObject('conf.service.restic.envvar');
        // Remove spaces from name
        $params['envname'] = str_replace(' ', '_', $params['envname']);
        // Set the configuration object.
        $envvar->setAssoc($params);
        $db = \OMV\Config\Database::getInstance();
        $db->set($envvar);
        // Return the configuration object.
        return $envvar->getAssoc();
    }

    public function deleteEnvVar($params, $context)
    {
        // Validate the RPC caller context.
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ['role' => OMV_ROLE_ADMINISTRATOR]);
        // Validate the parameters of the RPC service method.
        $this->validateMethodParams($params, 'rpc.common.objectuuid');
        // Get the configuration object.
        $db = \OMV\Config\Database::getInstance();
        $envvar = $db->get('conf.service.restic.envvar', $params['uuid']);
        // Delete the configuration object.
        $db->delete($envvar);
        // Return the deleted configuration object.
        return $envvar->getAssoc();
    }
}
