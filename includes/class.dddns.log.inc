<?php

/**
 * @file
 * Base class for Logging.
 */

require_once dirname(__FILE__) . '/class.dddns.extension.inc';

abstract class DDDNSlog extends DDDNSextension {

  // Data provided
  // Use this base for building a DB specific schema etc.
  protected $schemaKeys = array(

    // Never provided from an incoming DDDNSresult.
    // Must be constructed by the extension, if necessary.
    // 'id'       // bigint unsigned auto_increment
    //
    // Read only (should be returned, but will not be provided):
    // 'timestamp'// datetime not null default current_timestamp

    'nameserver' => 'varchar(256) NOT NULL', // varchar(256) not null ; RFC 1123
    'host' => 'varchar(256) NOT NULL',       // dto.
    'remoteHost' => 'varchar(256) NOT NULL', // dto.
    'action' => 'varchar(128) NOT NULL',     // varchar(128) not null
    'user' => 'varchar(128) NOT NULL',       // varchar(128) null default null
    'remoteIP' => 'varchar(64) NOT NULL',   // varchar(64) not null ; ipv6 in full length / RFC 2460
    'updateIPs' => 'varchar(2048) DEFAULT NULL',  // varchar(2048) null default null ; ~30 ipV6, comma separated
    'deleteIPs' => 'varchar(2048) DEFAULT NULL',  // dto.
    'currentIPs' => 'varchar(2048) DEFAULT NULL', // dto.
    'previousIPs' => 'varchar(2048) DEFAULT NULL',// dto.
    'clientNote' => 'varchar(512) DEFAULT NULL', // varchar(512) null default null ; free text
    'clientKey' => 'varchar(512) DEFAULT NULL',  // dto.
    'clientExtra' => 'varchar(512) DEFAULT NULL',// dto.
    'needUpdate' => 'tinyint(1) DEFAULT NULL', // bool
    'success' => 'tinyint(1) DEFAULT NULL',    // dto.
  );

  /**
   * Pass a result data record to the attached log interface.
   *
   * @param DDDNSresult $result
   */
  abstract public function write(DDDNSresult $result);

  /**
   * Return the most recent update/add entry.
   *
   * @param string $host
   *   The host to look up.
   * @param array $actions
   *   (Optional.)
   * @param string $cKey
   *   Optional filter for "clientKey" field.
   *
   * @return DDDNSresult
   *   The most recent entry for $host.
   */
  abstract public function check($host, $actions = array('add', 'update'), $cKey = '%');

  /**
   * Create/update database schema.
   *
   * @return object
   *   $this for chaining.
   */
  abstract public function install();

}
