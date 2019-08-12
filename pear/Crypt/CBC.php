<?php
/* vim: set ts=4 sw=4: */
// +----------------------------------------------------------------------+
// | PHP version 4.0                                                      |
// +----------------------------------------------------------------------+
// | Copyright (c) 2001 The PHP Group                                     |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available at through the world-wide-web at                           |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Colin Viebrock <colin@easydns.com>                          |
// |          Mike Glover <mpg4@duluoz.net>                               |
// +----------------------------------------------------------------------+
//

require_once 'PEAR.php';

if (function_exists('mcrypt_module_open')) {
    define('CRYPT_CBC_USING_24x', 1);
} else {
    define('CRYPT_CBC_USING_24x', 0);
}

/**
* Class to emulate Perl's Crypt::CBC module
*
* Blowfish support is not completely working, mainly because of a bug
* discovered in libmcrypt (version 2.4.8 and earlier).  If you are running
* a later version of libmcrypt > 2.4.8, you can do Blowfish encryption
* that is compatable with Perl.  However, check the libmcrypt documenation
* as to whether you should use 'BLOWFISH' or 'BLOWFISH-COMPAT' when
* specifying the cipher.
*
* If you are using libmcrypt <= 2.4.8, Blowfish encryption will work,
* but your data will not be readable by Perl scripts.  It will work
* "internally" .. i.e. this class will be able to encode/decode the data.
*
* NOTE: the cipher names in this class may change depending on how
* the author of libcrypt decides to name things internally.
*
*
* @version  $Id: CBC.php,v 1.9 2001/08/02 15:23:00 alexmerz Exp $
* @author   Colin Viebrock <colin@easydns.com>
* @author   Mike Glover <mpg4@duluoz.net>
* @access   public
* @package Crypt
*/

class Crypt_CBC extends PEAR {

    /**
    * supported procedures
    * @var array
    */
    var $known_ciphers = array (
        'DES'               => MCRYPT_DES,
        'BLOWFISH'          => MCRYPT_BLOWFISH,
        'BLOWFISH-COMPAT'   => MCRYPT_BLOWFISH_COMPAT,
    );

    /**
    * used cipher
    * @var string
    */
    var $cipher;
    /**
    * crypt resource, for 2.4.x
    * @var string
    */  
    var $TD;
    /**
    * blocksize of cipher
    * @var string
    */    
    var $blocksize;
    /**
    * keysize of cipher
    * @var int
    */    
    var $keysize;
    /**
    * mangled key
    * @var string
    */        
    var $keyhash;
    /**
    * source type of the initialization vector for creation  
    * possible types are MCRYPT_RAND or MCRYPT_DEV_URANDOM or MCRYPT_DEV_RANDOM    
    * @var int
    */            
    var $rand_source    = MCRYPT_RAND;
    /**
    * header
    * @var string
    */           
    var $header_spec    = 'RandomIV';
    /**
    * debugging
    * @var string
    */           
    var $_last_clear;
    /**
    * debugging
    * @var string
    */              
    var $_last_crypt;

    /**
    * Constructor
    * $key is the key to use for encryption. $cipher can be DES, BLOWFISH or
    * BLOWFISH-COMPAT
    *
    * @param    $key        encryption key
    * @param    $cipher     which algorithm to use, defaults to DES
    *
    * @return   $return     either a PEAR error or true
    *
    * @access   public
    *
    */

    function Crypt_CBC ($key, $cipher='DES')
    {

        if (!extension_loaded('mcrypt')) {
            return $this->raiseError('mcrypt module is not compiled into PHP', null, 
                PEAR_ERROR_DIE, null, 'compile PHP using "--with-mcrypt"', 'Crypt_CBC_Error', false );
        }

        /* seed randomizer */

        srand ((double)microtime()*1000000);

        /* initialize */

        $this->header_spec = 'RandomIV';

        /* check for key */

        if (!$key) {
            return $this->raiseError('no key specified', null, 
                PEAR_ERROR_PRINT, null, null, 'Crypt_CBC_Error', false );
        }

        /* check for cipher */

        $cipher = strtoupper($cipher);
        if (!isset($this->known_ciphers[$cipher])) {
            return $this->raiseError('unknown cipher "'.$cipher.'"', null, 
                PEAR_ERROR_PRINT, null, null, 'Crypt_CBC_Error', false );
        }

        $this->cipher = $this->known_ciphers[$cipher];

        /* initialize cipher */

        if (CRYPT_CBC_USING_24x) {
            $this->blocksize = mcrypt_get_block_size($this->cipher,'cbc');
            $this->keysize = mcrypt_get_key_size($this->cipher,'cbc');
            $this->TD = mcrypt_module_open ($this->cipher, '', 'ecb', '');
        } else {
            $this->blocksize = mcrypt_get_block_size('cbc');
            $this->keysize = mcrypt_get_key_size('cbc');
            $this->TD = false;
        }

        /* mangle key with MD5 */

        $this->keyhash = $this->_md5perl($key);
        while( strlen($this->keyhash) < $this->keysize ) {
            $this->keyhash .= $this->_md5perl($this->keyhash);
        }

        $this->key = substr($this->keyhash, 0, $this->keysize);

        return true;

    }



    /**
    * Encryption method
    *
    * @param    $clear      plaintext
    *
    * @return   $crypt      encrypted text, or PEAR error
    *
    * @access   public
    *
    */

    function encrypt($clear)
    {

        $this->last_clear = $clear;

        /* new IV for each message */

        $iv = mcrypt_create_iv($this->blocksize, $this->rand_source);

        /* create the message header */

        $crypt = $this->header_spec . $iv;

        /* pad the cleartext */

        $padsize = $this->blocksize - (strlen($clear) % $this->blocksize);
        $clear .= str_repeat(pack ('C*', $padsize), $padsize);


        /* do the encryption */

        $start = 0;
        while ( $block = substr($clear, $start, $this->blocksize) ) {
            $start += $this->blocksize;
            if (CRYPT_CBC_USING_24x) {
                mcrypt_generic_init($this->TD, $this->key, $iv);
                $cblock = mcrypt_generic($this->TD, $iv^$block );
            } else {
                $cblock = mcrypt_ECB($this->cipher, $this->key, $iv^$block, MCRYPT_ENCRYPT );
            }
            $iv = $cblock;
            $crypt .= $cblock;
        }

        $this->last_crypt = $crypt;
        return $crypt;

    }



    /**
    * Decryption method
    *
    * @param    $crypt      encrypted text
    *
    * @return   $clear      plaintext, or PEAR error
    *
    * @access   public
    *
    */

    function decrypt($crypt) {

        $this->last_crypt = $crypt;

        /* get the IV from the message header */

        $iv_offset = strlen($this->header_spec);
        $header = substr($crypt, 0, $iv_offset);
        $iv = substr ($crypt, $iv_offset, $this->blocksize);
        if ( $header != $this->header_spec ) {
            return $this->raiseError('no initialization vector', null, 
                PEAR_ERROR_PRINT, null, null, 'Crypt_CBC_Error', false );
        }

        $crypt = substr($crypt, $iv_offset+$this->blocksize);

        /* decrypt the message */

        $start = 0;
        $clear = '';

        while ( $cblock = substr($crypt, $start, $this->blocksize) ) {
            $start += $this->blocksize;
            if (CRYPT_CBC_USING_24x) {
                mcrypt_generic_init($this->TD, $this->key, $iv);
                $block = $iv ^ mdecrypt_generic($this->TD, $cblock);
            } else {
                $block = $iv ^ mcrypt_ECB($this->cipher, $this->key, $cblock, MCRYPT_DECRYPT );
            }
            $iv = $cblock;
            $clear .= $block;
        }

        /* remove the padding from the end of the cleartext */

        $padsize = ord(substr($clear, -1));
        $clear = substr($clear, 0, -$padsize);

        $this->last_clear = $clear;
        return $clear;

    }



    /**
    * Emulate Perl's MD5 function, which returns binary data
    *
    * @param    $string     string to MD5
    *
    * @return   $hash       binary hash
    *
    * @access private
    *
    */

    function _md5perl($string)
    {
        return pack('H*', md5($string));
    }


}

/**
* Inherit from PEAR_Error
* used to create a specific error message
* @access public
* @package Crypt
*/

class Crypt_CBC_Error extends PEAR_Error
{
    /**
    * Name of the this class
    * @var string
    */
    var $classname             = 'Crypt_CBC_Error';    
    /**
    * first part of the error message 
    * @var string
    */   
    var $error_message_prepend = 'Error in Crypt_CBC';

    /**
    * creates a new error object
    * @param    string  $message    error message
    * @param    int     $code       error code
    * @param    int     $mode       error mode
    * @param    int     $level      error level
    * @return   object  PEAR_Error  PEAR_Error object
    * @access   public
    */
    function Crypt_CBC_Error ($message, $code = 0, $mode = PEAR_ERROR_RETURN, $level = E_USER_NOTICE)
    {
        $this->PEAR_Error ($message, $code, $mode, $level);
    }

}


?>
