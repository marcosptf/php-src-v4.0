<?php
// +----------------------------------------------------------------------+
// | PHP version 4.0                                                      |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997, 1998, 1999, 2000, 2001 The PHP Group             |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available at through the world-wide-web at                           |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Ulf Wendel <ulf.wendel@phpdoc.de>                           |
// |          Christian Stocker <chregu@nomad.ch>                         |
// +----------------------------------------------------------------------+

require_once 'Cache/Output.php';

/**
* Cache using Output Buffering and contnet (gz) compression.
*
* Based upon a case study from Christian Stocker and inspired by jpcache.
*
* @version  $Id: OutputCompression.php,v 1.5 2001/08/10 16:12:34 sebastian Exp $
* @author   Ulf Wendel <ulf.wendel@phpdoc.de>, Christian Stocker <chregu@nomad.ch>
* @access   public
* @package  Cache
*/
class Cache_OutputCompression extends Cache_Output {
    
    /**
    * Encoding, what the user (its browser) of your website accepts
    * 
    * "auto" stands for test using $HTTP_ACCEPT_ENCODING.
    *
    * @var  string
    * @see  Cache_OutputCompression(), setEncoding()
    */
    var $encoding = 'auto';
 
    
    /**
    * Method used for compression
    *
    * @var  string
    * @see  isCompressed()
    */ 
    var $compression = '';

    
    /**
    * Sets the storage details and the content encoding used (if not autodetection)
    * 
    * @param    string  Name of container class
    * @param    array   Array with container class options
    * @param    string  content encoding mode - auto => test which encoding the user accepts
    */    
    function Cache_OutputCompression($container, $container_options = '', $encoding = 'auto') {
    
        $this->setEncoding($encoding);
        $this->Cache($container, $container_options);
        
    } // end constructor

    
    /**
    * Call parent deconstructor.
    */
    function _Cache_OutputCompression() {
        $this->_Cache();
    } // end deconstructor
    

    function generateID($variable) {
        
        $this->compression = $this->getEncoding();
        
        return md5(serialize($variable) . serialize($this->compression));
    } // end generateID

    
    function get($id, $group) {
        $this->content = '';
        
        if (!$this->caching)
            return '';
        
        if ($this->isCached($id, $group) && !$this->isExpired($id, $group))
            $this->content = $this->load($id, $group);
            
        return $this->content;
    } // end func get
    
    
    /**
    * Stops the output buffering, saves it to the cache and returns the _compressed_ content. 
    *
    * If you need the uncompressed content for further procession before
    * it's saved in the cache use endGet(). endGet() does _not compress_.
    */    
    function end($expire = 0, $userdata = '') {
        $content = ob_get_contents();
        ob_end_clean();

        // store in the cache
        if ($this->caching) {
            $this->extSave($this->output_id, $content, $userdata, $expire, $this->output_group);
            return $this->content;                
        }
            
        return $content;        
    } // end func end()
    
    
    function endPrint($expire = 0, $userdata = '') {
        $this->printContent($this->end($expire, $userdata));
    } // end func endPrint

    
    /**
    * Saves the given data to the cache.
    * 
    */   
    function extSave($id, $cachedata, $userdata, $expires = 0, $group = 'default') {
        if (!$this->caching)
            return true;

        if ($this->compression) {            
            
            $len = strlen($cachedata);            
            $crc = crc32($cachedata);
            $cachedata = gzcompress($cachedata, 9);
            $this->content = substr($cachedata, 0, strlen($cachedata) - 4) . pack('V', $crc) . pack('V', $len);
            
        } else {
            
            $this->content = $cachedata;
            
        }
        return $this->container->save($id, $this->content, $expires, $group, $userdata);
    } // end func extSave
    
    /**
    * Sends the compressed data to the user.
    * 
    * @param    string
    * @access   public
    * @global   $HTTP_SERVER_VARS
    */    
    function printContent($content = '') {
        global $HTTP_SERVER_VARS;

        if ('' == $content)
            $content = &$this->content;
         
        if ($this->compression && $this->caching) {
   
            $etag = 'PEAR-Cache-' . md5(substr($content, -40));
            header("ETag: $etag");
            if (strstr(stripslashes($HTTP_SERVER_VARS['HTTP_IF_NONE_MATCH']), $etag)) {
                // not modified
                header('HTTP/1.0 304');
                return;
            } else {
   
                // client acceppts some encoding - send headers & data
                header("Content-Encoding: {$this->compression}");
                header('Vary: Accept-Encoding');
                print "\x1f\x8b\x08\x00\x00\x00\x00\x00";
            }
        
        }
        
        print $content;
    } // end func printContent
    
    
    /**
    * Returns the encoding method of the current dataset. 
    *
    * @access   public
    * @return   string  Empty string (which evaluates to false) means no compression
    */
    function isCompressed() {
        return $this->compression;
    } // end func isCompressed

    /**
    * Sets the encoding to be used.
    * 
    * @param    string  "auto" means autodetect for every client
    * @access   public
    * @see      $encoding
    */
    function setEncoding($encoding = 'auto') {
        $this->encoding = $encoding;
    } // end func setEncoding
    
    
    /**
    * Returns the encoding to be used for the data transmission to the client.
    *
    * @global   $HTTP_ACCEPT_ENCODING
    * @see      setEncoding()
    */    
    function getEncoding() {
        global $HTTP_ACCEPT_ENCODING;
        
        // encoding set by user    
        if ('auto' != $this->encoding)
            return $this->encoding;
        
        // check what the client accepts
        if (false !== strpos($HTTP_ACCEPT_ENCODING, 'x-gzip'))
            return 'x-gzip';
        if (false !== strpos($HTTP_ACCEPT_ENCODING, 'gzip'))
            return 'gzip';
            
        // no compression
        return '';
        
    } // end func getEncoding

 
} // end class OutputCompression
?>
