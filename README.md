libswitch
==========================

**libswitch is nodeJS native addon C++ library to provide the runtime switches which can be turn on/off by publishing the configuration from a central repositor without application restart.

It supports two publishing method.
1) **AUTO** : Timer based auto publish the configuation based nextRefreshTime configured in "switch-config.xml" in the remote repository.
2) **PUSH** : Publish new configuraton using activeMQ.

During the startup, libswitch sets the front and back buffer of shared memory
and spawns a separate message broker process which is responsible for download new switch config, parse and write in shared memory in intelligent manner.

Here are the some of the features supported.

Features
------------
- Supports PULL and PUSH method to publish configuration.
- Publish switches configuration instantly to all the applications.
- Supports PUSH/PULL to specific set of servers based upon regex.
- Very flexible XML format
- Efficient datastructures to store and perform switch enable/disable check.
- Auto-recovery from any typo mistake in configuration.
- Supports boolean, long, string, regex, longArray, stringArray, regexArray, longFile, stringFile and regexFile.

Initial setup
------------
- Apache ActiveMQ : (Only for PUSH method)message broker to send recieve JMS messages for configuration refresh.
   You can download and install Apache ActiveMQ from http://activemq.apache.org/download.html
- Remote repository : http/https end point to  store and provide switch configuration xml.

Supported platforms
--------------------------------------
- All the flavors of *nix platforms.
- nodeJS version v0.10.18 or higher
- 32-bit and 64-bit.

# Basic installation

* Prerequisites:
  * Python 2.7 (*not* v3.x), used by node-gyp
  * C++ Compiler toolchain (GCC, Visual Studio or similar)
* Download and install APR from http://apr.apache.org/download.cgi if already not installed .
* Download and install PCRE from http://www.pcre.org/ if already not installed.
* Download and install libcurl from http://curl.haxx.se/libcurl/ if already not installed.
* Download and install expat from http://expat.sourceforge.net/ if already not installed.

# Test
    node test/test.js
    wget http://127.0.0.1:1337/

NodeJS API :

     /* 
     * initializes the libswitch library.
     */
     initialize("./conf/libswitch.conf", function(error) { };
     
     /* 
     * returns true/false if switchValue/switchName exists in a give module.
     */
     var isSwitchEnabled(var moduleName, var switchName, var switchValue);    
    
If you want to implement switch feature in c/c++ application (other then nodeJS), simply build the stand-alone C libswitch library (static) simply run ( this will build libswitch.so in current directory /tmp/libswitch) :

    make
    make test
    ./test

To link libswitch.a in your c/c++ application, add following line in your source-code :

     #include "libswitch_conf_ext.h"
     and link statically with
     -L./ -lswitch

APIs :

     /* 
     * initializes the libswitch library.
     */
     int libswitch_initializeLibSwitch(const char* configFile);
     
     /* 
     * returns true/false if switchValue/switchName exists in a give module.
     */
     int libswitch_isSwitchEnabled(const char* moduleName, const char* switchName, const char* switchValue);    

Setup remote repository and namespace directory
-----------------------------------------------
- Setup a remote repository
	- Select any remote repository ie git, svn, alfresco
- Create a name-space directory : name must be same as specified namespace tag in config-core.xml
- Create a test.txt in the directory created in step 2) and add "hello" to test.txt
- Verify that test.txt is accessible using wget from target machine
	- wget http(s)://{remote-repository}/{name-space-directory}/test.txt
- Copy switch-config.xml from /tmp/libswitch/conf/switch-config.xml to name-space directory in remote repository

HOW TO PUBLISH NEW SWITCH CONFIGURATION
--------------------------
**PULL METHOD: Auto publish**

- Goto name-space directory in remote repostory
- Add new switch in switch-config.xml, update "nextRefreshServerRegex" 
	with matching regex of selected servers and "nextRefreshTime" attribute in following format
 	- ddd mmm dd HH:MM:ss yyyy
	- Example:Mon Jan 13 00:49:00 2014
- check-in.

**PUSH METHOD: ActiveMQ**

Build activeMQ refresh command line utility

    cd /tmp/libswitch
    make switch_command

To run refresh command ( to publish new switch configuration):

	./sc -n <namespace> -c <config-file> [-a <action, default:Refresh>]

Where
- host:port ActiveMQ host and JMS port
- username: ActiveMQ connection username
- password: ActiveMQ connection password
- namespace: Remote namespace which you want to refresh

Configuration ( libswitch.conf)
------------------------------

The distribution contains a sample file called *libswitch.conf* under conf/ directory
and this file contains all the following startup configuration.

```ruby

		#
		# Main Configuration
		#

		LIBSWITCH_SystemInfo "Libswitch test app"

		# This is the config file to specify runtime switches service
		LIBSWITCH_HomeDir         ./conf
		LIBSWITCH_ConfigCoreFile  config-core.xml
		LIBSWITCH_LogFile 	  startup.log

```

- **LIBSWITCH_SystemInfo** Short application description.

    Specify description string.

- **LIBSWITCH_HomeDir** Home directory to hold libswitch configuration and logs files.

    Specify description string.
    
- **LIBSWITCH_ConfigCoreFile** boot strap configuration

    Specify autoRefresh, activeMQ and remote repository location configuration.

- **LIBSWITCH_LogFile** log file to record startup information.

    Log file is generated upon startup and refresh and records information whats loaded in shared memory.


Boot strap configuration : config-core.xml
---------------

```XML
<?xml version="1.0"?>
<config-core namespace="myapp-switches"> <!-- namespace is unique string -->
        <sheapMapFile>/configcore.shm</sheapMapFile> <!-- shared memory segment name -->
        <sheapPageSize>200000</sheapPageSize> <!-- shared memory segment size, enought to hold 300 redirects -->
        <databaseFile>/realm.db3</databaseFile> <!-- local in-memory sqlite DB -->

        <!-- PULL mechanism where module automatically download {namespace}/switch-config.xml from remote repository -->
        <!-- you can comment this block if using PUSH mechanism -->
	<enableAutoRefresh>true</enableAutoRefresh><!-- enable auto refresh -->
	<autoRefreshWaitSeconds>20</autoRefreshWaitSeconds><!-- no. of seconds to wait until nextRefreshTime check -->
	<!-- end of PULL settings -->

	<!-- PUSH mechanism using activeMQ, you can comment this block if using PULL mechanism -->
         <messageBroker><!-- activeMQ configuration -->
                <host>127.0.0.1</host> <!-- specify your activeMQ host here -->
                <port>61613</port>
                <waitTimeSeconds>1200</waitTimeSeconds>
                <reconnectWaitMillis>10000</reconnectWaitMillis>
        </messageBroker>
        <!-- end of PUSH settings -->

        <resourceService><!-- remote repository location to download new switches in xml format -->
                <uri>https://github.dowjones.net/identity-systems/libswitch/raw/master/conf</uri>
                <timeoutSeconds>5</timeoutSeconds> <!-- download timeout -->
        </resourceService>

		<services>
			<service id="SWITCH-CONFIG" name="switchConfig">
						<param name="config-xml">switch-config.xml</param>
				</service>
		</services>
</config-core>


```

There are basic configuration to arrange the configuration on remote repository:

- **namespace** this is the directory on remote repository speicified in resourceService/uri to hold switches configuration **switch-config.xml**.

Syntax : switch-config.xml
---------------

```XML
<?xml version="1.0" encoding="UTF-8"?>
<switch-config nextRefreshTime="$$ddd mmm dd HH:MM:ss yyyy$$" nextRefreshServerRegex="$$servers-hostname-regex$$"><!-- Example Mon Jan 13 00:49:00 2014 -->
        <modules>
	        <module id="$$module1$$">
                    <switch name="$$switch-name11$$" type="$$boolean|long|string|regex$$">$$value$$</switch>
	            <switch name="$$switch-name12$$" type="$$boolean|long|string|regex$$">$$value$$</switch>
	            . . .
                    <switch name="$$switch-array-name11$$" type="$$longArray|stringArray|regexArray$$">$$comma-separated-values$$</switch>                    
                    <switch name="$$switch-array-name12$$" type="$$longArray|stringArray|regexArray$$">$$comma-separated-values$$</switch>                    
		    . . .
                    <switch name="$$switch-files-name11$$" type="$$longFile|stringFile|regexFile$$">$$file-containing-values-one-per-line$$</switch>                    
                    <switch name="$$switch-files-name12$$" type="$$longFile|stringFile|regexFile$$">$$file-containing-values-one-per-line$$</switch>                    
		    . . .
	        </module>
	        <module id="$$module2$$">
                    <switch name="$$switch-name21$$" type="$$boolean|long|string|regex$$">$$value$$</switch>
	            <switch name="$$switch-name22$$" type="$$boolean|long|string|regex$$">$$value$$</switch>
	            . . .
                    <switch name="$$switch-array-name21$$" type="$$longArray|stringArray|regexArray$$">$$comma-separated-values$$</switch>                    
                    <switch name="$$switch-array-name22$$" type="$$longArray|stringArray|regexArray$$">$$comma-separated-values$$</switch>                    
		    . . .
                    <switch name="$$switch-files-name21$$" type="$$longFile|stringFile|regexFile$$">$$file-containing-values-one-per-line$$</switch>                    
                    <switch name="$$switch-files-name21$$" type="$$longFile|stringFile|regexFile$$">$$file-containing-values-one-per-line$$</switch>                    
		    . . .
	        </module>
	        . . .
        </modules>
</switch-config>




```

Example : switch-config.xml
---------------------------

```XML
<?xml version="1.0" encoding="UTF-8"?>
<switch-config nextRefreshTime="Thu Apr 17 06:48:11 2014" nextRefreshServerRegex="sbkcomwebp07|sbkcomwebp08">
        <modules>
	        <module id="module-1">
                    <switch name="enableUuidCheck" type="boolean">on</switch>
                    <switch name="productID" type="long">1234</switch>
                    <switch name="uuid" type="string">dj_chandt</switch>
                    <switch name="uuidRegex" type="regex">dj_(.*)</switch>
                    <switch name="productIds" type="longArray">5,4,2,3,1,0</switch>
                    <switch name="uuids" type="stringArray">u4,u2,u3,u1,u5,u6</switch>
                    <switch name="uuidsRegex" type="regexArray">^a,^b,^c,^d,^e,^5</switch>
                    <switch name="productIdFile" type="longFile">pids.txt</switch>
                    <switch name="uuidFile" type="stringFile">uuids.txt</switch>
                    <switch name="uuidRegexFile" type="regexFile">uuid-regexes.txt</switch>
	        </module>
	        <!--module id="module-2">
                    <switch name="switch1">on</switch>
                    <switch name="switch2">on</switch>
	        </module>
	        <module id="module-3">
                    <switch name="switch1">off</switch>
                    <switch name="switch2">off</switch>
	        </module-->
        </modules>
</switch-config>


```

Example : test.js
---------------------------

```TXT
var http = require('http');
var switches = require('libswitch');


switches.initialize("./conf/libswitch.conf", function(error){
  if(error!=null) {
  	console.log(error);
  } else {
  	console.log('successfully initialized');
  }
});


http.createServer(function (req, res) {

  res.writeHead(200, {'Content-Type': 'text/plain'});
  var isEnabled = switches.isSwitchEnabled("module-1","uuid","dj_chandt");
  res.end('module-1/uuid/dj_chand is set to ' + isEnabled);
  
  console.log(isEnabled);
  
}).listen(1337, '127.0.0.1');

console.log('Server running at http://127.0.0.1:1337/');


```

More documentation to be followed:

Development
------------

- Source hosted at [GitHub](https://github.dowjones.net/identity-systems/libswitch)
- Report issues, questions, feature requests on [GitHub Issues](https://github.dowjones.net/identity-systems/libswitch/issues)

Diagnostics
---------------------

- Log file path
	-  	Refresh log - libswitch/logs/refresh.log 
	-  	Startup log - libswitch/startup.log

- Issues - Unable to create shared memory
	- 	Resolution - clear the shared memory using following command
	- 	``` for shmid in `ipcs -a | awk '{if ($9==0) print $2}'`; do ipcrm -m $shmid; done ```
	- 	Run "ipcs -a" and check for column name NATTACH and update column number in above command.

Authors
-------

[Tara chand Verma]

* * *
