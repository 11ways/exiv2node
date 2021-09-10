<h1 align="center">
  <b>@11ways/exiv2node</b>
</h1>
<div align="center">
  <!-- CI - Github Actions -->
  <a href="https://github.com/11ways/exiv2node/actions/workflows/unit_test.yaml">
    <img src="https://github.com/11ways/exiv2node/actions/workflows/unit_test.yaml/badge.svg" alt="Node.js CI (Linux, MacOS, Windows)" />
  </a>

  <!-- Coverage - Codecov -->
  <a href="https://codecov.io/gh/11ways/exiv2node">
    <img src="https://img.shields.io/codecov/c/github/11ways/exiv2node/master.svg" alt="Codecov Coverage report" />
  </a>

  <!-- DM - Snyk -->
  <a href="https://snyk.io/test/github/11ways/exiv2node?targetFile=package.json">
    <img src="https://snyk.io/test/github/11ways/exiv2node/badge.svg?targetFile=package.json" alt="Known Vulnerabilities" />
  </a>

  <!-- DM - David -->
  <a href="https://david-dm.org/11ways/exiv2node">
    <img src="https://david-dm.org/11ways/exiv2node/status.svg" alt="Dependency Status" />
  </a>
</div>

<div align="center">
  <!-- Version - npm -->
  <a href="https://www.npmjs.com/package/@11ways/exiv2node">
    <img src="https://img.shields.io/npm/v/exiv2node.svg" alt="Latest version on npm" />
  </a>

  <!-- License - MIT -->
  <a href="https://github.com/11ways/exiv2node#license">
    <img src="https://img.shields.io/github/license/11ways/exiv2node.svg" alt="Project license" />
  </a>
</div>
<br>
<div align="center">
  👷🏼 Eleven Ways' exiv2node library
</div>
<div align="center">
  <sub>
    Coded with ❤️ by <a href="#authors">Eleven Ways</a>.
  </sub>
</div>

# Exiv2

Exiv2 is a native C++ extension for [node.js](https://nodejs.org) that provides
support for reading and writing image metadata via the [Exiv2 library](http://www.exiv2.org).

It was created by Damian Beresford

## Dependencies

To build this addon you'll need the Exiv2 library and headers so if you're using
a package manager you might need to install an additional "-dev" packages.

### Debian

    apt-get install exiv2 libexiv2-dev

### OS X

You'll also need to install pkg-config to help locate the library and headers.

[MacPorts](http://macports.org/):

    port install pkgconfig exiv2

[Homebrew](http://github.com/mxcl/homebrew/):

    brew install pkg-config exiv2

### FreeBSD

    pkg install pkgconf exiv2

### Windows
Install pkg-config using [Chocolatey](https://chocolatey.org/):

    choco install pkgconfiglite
    
Download latest `msvc64` exiv2 build from the [Exiv2 download page](http://www.exiv2.org/download.html) and extract to a folder of your choice.

Add a system variable named `PKG_CONFIG_PATH` and set it's value to `EXIV2ROOTDIR\lib\pkgconfig` replacing `EXIV2ROOTDIR` with the path where you extracted exiv2 from the step before (e.g. `D:\src\exiv2msvs\lib\pkgconfig`).

You'll also need [windows-build-tools](https://www.npmjs.com/package/windows-build-tools) to compile this package.

For Electron apps, you'll want to copy `exiv2.dll` to the root directory of your Electron Windows build. You can automated this using the [extraFiles option](https://www.electron.build/configuration/contents#extrafiles). 

### Other systems

See the [Exiv2 download page](http://www.exiv2.org/download.html) for more
information.

## Installation Instructions

Once the dependencies are in place, you can build and install the module using
npm:

    npm install exiv2

You can verify that everything is installed and operating correctly by running
the tests:

    npm test

## Sample Usage

### Read tags:

    var ex = require('exiv2');

    ex.getImageTags('./photo.jpg', function(err, tags) {
      console.log("DateTime: " + tags["Exif.Image.DateTime"]);
      console.log("DateTimeOriginal: " + tags["Exif.Photo.DateTimeOriginal"]);
    });

    var fs = require('fs');

    ex.getImageTags(fs.readFileSync('./photo.jpg'), function(err, tags) {
      console.log("DateTime: " + tags["Exif.Image.DateTime"]);
      console.log("DateTimeOriginal: " + tags["Exif.Photo.DateTimeOriginal"]);
    });

### Load preview images:

    var ex = require('exiv2')
      , fs = require('fs');

    ex.getImagePreviews('./photo.jpg', function(err, previews) {
      // Display information about the previews.
      console.log(previews);

      // Or you can save them--though you'll probably want to check the MIME
      // type before picking an extension.
      fs.writeFile('preview.jpg', previews[0].data);
    });

### Write tags:

    var ex = require('exiv2')

    var newTags = {
      "Exif.Photo.UserComment" : "Some Comment..",
      "Exif.Canon.OwnerName" : "My Camera"
    };
    ex.setImageTags('./photo.jpg', newTags, function(err){
      if (err) {
        console.error(err);
      } else {
        console.log("setImageTags complete..");
      }
    });

### Delete tags:

    var ex = require('exiv2')

    var tagsToDelete = ["Exif.Photo.UserComment", "Exif.Canon.OwnerName"];
    ex.deleteImageTags('./photo.jpg', tagsToDelete, function(err){
      if (err) {
        console.error(err);
      } else {
        console.log("deleteImageTags complete..");
      }
    });

Take a look at the `examples/` and `test/` directories for more.

## Authors
- **Damian Beresford** - Original creator
- **Jelle De Loecker** -  *Follow* me on *Github* ([:octocat:@skerit](https://github.com/skerit)) and on  *Twitter* ([🐦@skeriten](http://twitter.com/intent/user?screen_name=skeriten))

See also the list of [contributors](https://github.com/11ways/exiv2node/contributors) who participated in this project.

@11ways/exiv2node is developed at [Eleven Ways](https://www.elevenways.be/), a team of [IAAP Certified Accessibility Specialists](https://www.accessibilityassociation.org/).

## License
This project is licensed under the MIT License - see the [LICENSE](https://github.com/11ways/exiv2node/LICENSE) file for details.
