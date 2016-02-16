{
  "targets": [
    {
      "target_name": "switch_bindings",
      "sources": [ 
      "src/switch_bindings.cpp",
      "src/log-utils/logging.c",
      "src/log-utils/log_core.c",
      "src/common-utils/common_utils.c",
      "src/shm_core/shm_dup.c",
      "src/shm_core/shm_data.c",
      "src/shm_core/shm_mutex.c",
      "src/shm_core/shm_datatypes.c",
      "src/shm_core/shm_apr.c",
      "src/http-utils/http_client.c",
      "src/stomp-utils/stomp_utils.c",
      "src/stomp-utils/stomp.c",
      "src/xml-core/xml_core.c",
      "src/xml-core/token_utils.c",
      "src/config_core2.c",
      "src/config_messaging_parsing2.c",
      "src/config_messaging2.c",
      "src/config_bindings2.c",
      "src/config_error2.c",
      "src/switch_config_core.c",
      "src/switch_config_xml.c",
      "src/switch_config.c",
      "src/db_core2.c",
      "src/libswitch_conf.c",
      "src/switch_types.c",
      "src/sqlite3.c"
      ],
      "include_dirs": [ 
      "./src",
      "./src/common-utils",
      "./src/apache-utils",
      "./src/shm_core",
      "./src/log-utils",
	  "./src/stomp-utils",
	  "./src/xml-core",
	  "./src/http-utils",
	  "/usr/include/apr-1" 
      ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'conditions': [
        ['OS=="linux"', {
		  'libraries': [ '-lapr-1 -laprutil-1 -lexpat -lz -lpcreposix -lcurl' ]
        }],
        ['OS=="mac"', {
          'libraries': [ '-lapr-1 -laprutil-1 -lexpat -lz -lpcreposix -lcurl' ],
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'GCC_ENABLE_CPP_RTTI': 'NO',
            'INSTALL_PATH': '@rpath',
            'LD_DYLIB_INSTALL_NAME': '',
            'OTHER_LDFLAGS': [
              '-Wl,-search_paths_first',
              '-Wl,-rpath,<(module_root_dir)/src/http-utils/osx',
              '-L<(module_root_dir)/src/http-utils/osx'
            ]
          }
        }]
      ]
    }
  ]
}
