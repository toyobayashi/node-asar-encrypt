{
  'variables': {
    'no_asar%': 'false'
  },
  'targets': [
    {
      'target_name': 'main',
      'sources': [
        'main.cpp',
        'base64.c',
        'aes/aes.c'
      ],
      'includes': [
        './common.gypi'
      ],
      'defines': [
        'CBC=1',
        'AES256=1',
        'JS_ASAR_REQUIRE_FROM_MAIN="<(require_from_main)"'
      ],
      'conditions': [
        ["'<(no_asar)' == 'true'", {
          'defines': [
            'JS_ASAR_CONFIG_NO_ASAR=1'
          ]
        }]
      ]
    }
  ]
}
