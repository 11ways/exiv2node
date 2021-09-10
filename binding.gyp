{
  'variables': {
    'EXIV2_LIB': '<!(pkg-config --variable=libdir exiv2)'
  },
  'targets': [
    {
      'target_name': 'exiv2',
      'sources': [
        'exiv2node.cc'
      ],
      'include_dirs' : [
        '<!@(pkg-config --variable=includedir exiv2)',
        "<!(node -e \"require('nan')\")"
      ],
      'xcode_settings': {
        'MACOSX_DEPLOYMENT_TARGET': '10.12',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'OTHER_CPLUSPLUSFLAGS': [
            '-stdlib=libc++',
            '-fcxx-exceptions',
            '-frtti',
            '-fexceptions'
        ],
      },
      "cflags": [
        "-Wdeprecated-declarations",
        "-fexceptions",
        "-frtti"
      ],
      'cflags_cc': [
        '-fexceptions',
        "-frtti",
        "-Wdeprecated-declarations"
      ],
      'conditions': [[
        'OS=="win"',
		{
          'libraries': [
            '<(EXIV2_LIB)/exiv2.lib'
          ],
        },
        {
          'libraries': [
            '<!@(pkg-config --libs exiv2)'
          ]
        }
      ]]
    }
  ]
}
