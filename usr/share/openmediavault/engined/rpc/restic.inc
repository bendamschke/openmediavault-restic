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

    public function getName()
    {
        return "Restic";
    }

    public function initialize()
    {
        $this->registerMethod("getSettings");
        $this->registerMethod("setSettings");
        $this->registerMethod("updateRestic");
    }

    public function getSettings($params, $context)
    {
        // Validate the RPC caller context
        // i.e. you must be an logged on as an administrator to perform this function
        $this->validateMethodContext($context, ["role" => OMV_ROLE_ADMINISTRATOR]);
        
        // Get the configuration object from the database "config.xml" 
        $db = \OMV\Config\Database::getInstance();
        $object = $db->get("conf.service.restic.settings");
        
        // Make a call to the Restic application, with the `version` argument
        $cmdArgs = [];
        $cmdArgs[] = $this->getProgram;
        $cmdArgs[] = "version | awk '{print $2}'";
        $cmd = new \OMV\System\Process($cmdArgs);
        $cmd->setRedirect2to1();
        $cmd->execute($output);

        // Write to the configuration object, ensuring that the keys match that of the properties within conf.service.restic.json
        // usr/share/openmediavault/workbench/component.d/omv-services-restic-settings-form-page.yaml calls `getSettings`
		$object->setFlatAssoc([
            "enable" => $object->get("enable"),
			"version" => $output[0],
		]);

		// Write the configuration object to the database "config.xml"
		$db->set($object);

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
			"version" => $params['version'],
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
}
