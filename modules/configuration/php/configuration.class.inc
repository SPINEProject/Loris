<?php
/**
 * The configuration module is used to manage the configuration of Loris.
 *
 * PHP version 5
 *
 * @category Behavioural
 * @package  Main
 * @author   Tara Campbell <tara.campbell@mail.mcill.ca>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://github.com/aces/Loris-Trunk
*/

namespace LORIS\configuration;

/**
 * Admin_Config Class
 *
 * This class is to configure the system settings
 *
 * @category Behavioural
 * @package  Main
 * @author   Tara Campbell <tara.campbell@mail.mcill.ca>
 * @license  http://www.gnu.org/licenses/gpl-3.0.txt GPLv3
 * @link     https://github.com/aces/Loris-Trunk
*/
class Configuration extends \NDB_Form
{
    /**
     * _has_access returns true
     * if the user has the specific permission
     *
     * @return boolean
     */
    function _hasAccess()
    {
        // create user object
        $user =& \User::singleton();
        return $user->hasPermission('config');
    }

     /**
     * Loads the main configuration module page.
     *
     * @param string $name       The test name being accessed
     * @param string $page       The subtest being accessed (may be null)
     * @param string $identifier The identifier for the data to load on this page
     * @param string $commentID  The CommentID to load the data for
     * @param string $formname   The name to give this form
     *
     * @return none
     */
    function _setupPage($name, $page, $identifier, $commentID, $formname)
    {
        parent::_setupPage($name, $page, $identifier, $commentID, $formname);
        $config =& \NDB_Config::singleton();
        $DB     =& \Database::singleton();

        $scans = $DB->pselect(
            "SELECT Scan_type
             FROM mri_scan_type",
            array()
        );
        foreach ($scans as $type) {
            $val = $type['Scan_type'];
            $scan_types[$val] =$val;
        }
        $scan_types = array_merge(array('' => ''), $scan_types);

        $instruments = \Utility::getAllInstruments();
        $instruments = array_merge(array('' => ''), $instruments);

        $this->tpl_data['useProjects']     = $config->getSetting("useProjects");
        $this->tpl_data['parentMenuItems'] = $this->_getParentConfigLabels();
        $this->tpl_data['config']          = $this->_getConfigSettingTree();
        $this->tpl_data['instruments']     = $instruments;
        $this->tpl_data['sandbox']         = $config->getSetting("sandbox");
        $this->tpl_data['scan_types']      = $scan_types;

    }

    /**
     * Gets an array of the top level configuration items
     * This is used to create the configuration setting category menu
     *
     * @return array of parent config items
     */
    function _getParentConfigLabels()
    {
        $this->DB = \Database::singleton();

        $parentConfigItems = $this->DB->pselect(
            "SELECT Label, Name 
             FROM ConfigSettings 
             WHERE Parent IS NULL AND Visible=1 ORDER BY OrderNumber",
            array()
        );

        return $parentConfigItems;
    }

    /**
     * Returns an array of configuration settings in a tree format.
     * The array contains all of the settings that are in config table
     * in the database. However, if the value if overridden in the config.xml,
     * the value for that setting is replaced with the value from the config.xml
     *
     * @return array of config settings
     */
    function _getConfigSettingTree()
    {
        $config =& \NDB_Config::singleton();

        // Get the names and meta-information for the config settings in the database
        $configs = $this->DB->pselect(
            "SELECT * FROM ConfigSettings WHERE Visible=1 ORDER BY OrderNumber",
            array()
        );

        // Check whether any setting is overwritten in the config.xml
        // Add this info to the array so form entries can be disabled in front end
        // Update the value for the setting to the value from the config.xml
        foreach ($configs as &$setting) {
            try {
                $setting['Disabled'] = 'Yes';
                if ($setting['Parent'] != null) {
                    $valueFromXML = $config->getSettingFromXML($setting['Name']);
                    if (!is_array($valueFromXML)) {
                        $setting['Value'][0] = $valueFromXML;
                    } else {
                        $setting['Value'] = $valueFromXML;
                    }
                }
            } catch (\ConfigurationException $e) {
                $setting['Disabled'] = 'No';
            }
        }

        // Now check for config settings from the database for the fields not
        // overridden in the config.xml
        foreach ($configs as &$setting) {
            if ($setting['Disabled'] == 'No') {
                $value = $this->DB->pselect(
                    "SELECT ID, Value FROM Config WHERE ConfigID=:ID",
                    array('ID' => $setting{'ID'})
                );
                if ($value) {
                    foreach ($value as $subvalue) {
                        $setting['Value'][$subvalue['ID']] = $subvalue['Value'];
                    }
                }
            }
        }

        // build a tree from configs array
        foreach ($configs as &$node) {
            $node['Children']  = array();
            $tree[$node['ID']] = &$node;

        }
        foreach ($configs as &$node) {
            $tree[$node['Parent']]['Children'][] = &$node;
        }

        return $configs;
    }

    /**
     * Include the column formatter required to display the feedback link colours
     * in the candidate_list menu
     *
     * @return array of javascript to be inserted
     */
    function getJSDependencies()
    {
        $factory = \NDB_Factory::singleton();
        $baseURL = $factory->settings()->getBaseURL();
        $deps    = parent::getJSDependencies();
        return array_merge(
            $deps,
            array(
             $baseURL . "/configuration/js/configuration_helper.js",
            )
        );
    }

     /**
        * Include additional CSS files:
        *  1. configuration
        *
        * @return array of CSS to be inserted
        */
    function getCSSDependencies()
    {
        $factory = \NDB_Factory::singleton();
        $baseURL = $factory->settings()->getBaseURL();
        $deps    = parent::getCSSDependencies();
        return array_merge(
            $deps,
            [$baseURL . "/configuration/css/configuration.css"]
        );
    }

}
