<?php

/**
 * @file
 * Extension to output formatted HTML.
 *
 * Requires the following extension structures in the global DDDNS config array:
 *
 * $global_config = array(
 *   'output' => array(
 *     'html' => array(
 *       // The following uses the template file
 *       //   "../templates/DDDNS.output.html.default.inc"
 *       'formatter' => 'default',
 *     ),
 *   ),
 * );
 *
 */

require_once dirname(__FILE__) . '/class.dddns.output.inc';

class DDDNSoutputHtml extends DDDNSoutput {

  protected $tpl;

  function __construct($config, $headers) {

    // Override content type header.
    $headers['Content-type'] = 'text/html';

    parent::__construct($config, $headers);

    $this->tpl = dirname(__FILE__) . "/../templates/html.{$this->config['html']['formatter']}.tpl.php";
    if (!is_file($this->tpl) || !is_readable($this->tpl)) {
      throw new DDDNSexception('Output template "%s" not found or not readable.', $this->tpl);
    }

  }

  /**
   * Simple phptemplate parser.
   *
   * @param DDDNSresult $resultObject
   *   DDDNS Result object.
   * @param bool $send
   *   (Optional) Whether to send the output directly to stdout.
   *   Defaults to FALSE.
   *
   * @return string
   *   The parsed markup.
   */
  public function output(DDDNSresult $resultObject, $send = FALSE) {

    $vars = $resultObject->get($this->outputFields);

    // Import template variables.
    extract($vars);

    // Parse template into and return buffer.
    ob_start();
    include $this->tpl;
    $ret = ob_get_clean();

    if ($send) {
      $this->sendHeaders();
      echo $ret;
    }

    return $ret;

  }

}
