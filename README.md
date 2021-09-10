# Exiv2

Exiv2 is a native C++ extension for [node.js](https://nodejs.org) that provides
support for reading and writing image metadata via the [Exiv2 library](http://www.exiv2.org).

## Dependencies

To build this addon you'll need the Exiv2 library and headers so if you're using
a package manager you might need to install an additional "-dev" packages.

### Debian

    apt-get install zlib1g-dev

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

email: dberesford at gmail
twitter: @dberesford
