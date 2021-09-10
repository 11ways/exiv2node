#include <v8.h>
#include <node.h>
#include <node_buffer.h>
#include <nan.h>
#ifdef _MSC_VER
# include <direct.h>
# include <process.h>
#else
# include <unistd.h>
# include <stdint.h>
#endif
#include <string>
#include <map>
#include <exception>
#include <exiv2/image.hpp>
#include <exiv2/exif.hpp>
#include <exiv2/preview.hpp>

using namespace node;
using namespace v8;

// Create a map of strings for passing them back and forth between the V8 and
// worker threads.
typedef std::map<std::string, std::string> tag_map_t;

class Exiv2Worker : public Nan::AsyncWorker {
 public:
  Exiv2Worker(Nan::Callback *callback, const std::string fileName)
    : Nan::AsyncWorker(callback), isBuf(false), fileName(fileName) {}
  Exiv2Worker(Nan::Callback *callback, const char* buf, const long len)
    : Nan::AsyncWorker(callback), isBuf(true), buf(reinterpret_cast<const Exiv2::byte*>(buf)), bufLen(len) {}
  ~Exiv2Worker() {}

 protected:
  const bool isBuf;
  const Exiv2::byte* buf = nullptr;
  const long bufLen = -1;
  const std::string fileName = "";
  std::string exifException;
  Exiv2::Image::AutoPtr image() {
    return this->isBuf
      ? Exiv2::ImageFactory::open(this->buf, this->bufLen)
      : Exiv2::ImageFactory::open(this->fileName);
  }
};

// - - -

class GetTagsWorker : public Exiv2Worker {
 public:
  GetTagsWorker(Nan::Callback *callback, const std::string fileName)
    : Exiv2Worker(callback, fileName) {}
  GetTagsWorker(Nan::Callback *callback, const char* buf, const long len)
    : Exiv2Worker(callback, buf, len) {}
  ~GetTagsWorker() {}

  // Should become protected...
  tag_map_t tags;

  // Executed inside the worker-thread. Not safe to access V8, or V8 data
  // structures here.
  void Execute () {
    try {
      Exiv2::Image::AutoPtr image = this->image();
      assert(image.get() != 0);
      image->readMetadata();

      Exiv2::ExifData &exifData = image->exifData();
      if (exifData.empty() == false) {
        Exiv2::ExifData::const_iterator end = exifData.end();
        for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
          tags.insert(std::pair<std::string, std::string> (i->key(), i->value().toString()));
        }
      }

      Exiv2::IptcData &iptcData = image->iptcData();
      if (iptcData.empty() == false) {
        Exiv2::IptcData::const_iterator end = iptcData.end();
        for (Exiv2::IptcData::const_iterator i = iptcData.begin(); i != end; ++i) {
          tags.insert(std::pair<std::string, std::string> (i->key(), i->value().toString()));
        }
      }

      Exiv2::XmpData &xmpData = image->xmpData();
      if (xmpData.empty() == false) {
        Exiv2::XmpData::const_iterator end = xmpData.end();
        for (Exiv2::XmpData::const_iterator i = xmpData.begin(); i != end; ++i) {
          tags.insert(std::pair<std::string, std::string> (i->key(), i->value().toString()));
        }
      }
    } catch (std::exception& e) {
      exifException.append(e.what());
    }
  }

  // This function will be run inside the main event loop so it is safe to use
  // V8 again.
  void HandleOKCallback () {
    Nan::HandleScope scope;

    Local<Value> argv[2] = { Nan::Null(), Nan::Null() };

    if (!exifException.empty()){
      argv[0] = Nan::New<String>(exifException.c_str()).ToLocalChecked();
    } else if (!tags.empty()) {
      Local<Object> hash = Nan::New<Object>();
      // Copy the tags out.
      for (tag_map_t::iterator i = tags.begin(); i != tags.end(); ++i) {
        Nan::Set(
          hash,
          Nan::New<String>(i->first.c_str()).ToLocalChecked(),
          Nan::New<String>(i->second.c_str()).ToLocalChecked()
        );
      }
      argv[1] = hash;
    }

    // Pass the argv array object to our callback function.
    Nan::Call(callback->GetFunction(), Nan::GetCurrentContext()->Global(), 2, argv);
  };
};

NAN_METHOD(GetImageTags) {
  Nan::HandleScope scope;

  /* Usage arguments */
  if (info.Length() <= 1 || !info[1]->IsFunction())
    return Nan::ThrowTypeError("Usage: <filename/buffer> <callback function>");

  Nan::Callback *callback = new Nan::Callback(info[1].As<Function>());
  if (info[0]->IsString()) {
    std::string filename = std::string(*Nan::Utf8String(info[0]));
    Nan::AsyncQueueWorker(new GetTagsWorker(callback, filename));
  } else {
    Local<Object> buf = info[0]->ToObject(v8::Isolate::GetCurrent());
    Nan::AsyncQueueWorker(new GetTagsWorker(callback, Buffer::Data(buf), Buffer::Length(buf)));
  }
  return;
}

// - - -

class SetTagsWorker : public Exiv2Worker {
 public:
  SetTagsWorker(Nan::Callback *callback, const std::string fileName)
    : Exiv2Worker(callback, fileName) {}
  SetTagsWorker(Nan::Callback *callback, const char* buf, const long len)
    : Exiv2Worker(callback, buf, len) {}
  ~SetTagsWorker() {}

  // Should become protected...
  tag_map_t tags;

  // Executed inside the worker-thread. Not safe to access V8, or V8 data
  // structures here.
  void Execute () {
    try {
      Exiv2::Image::AutoPtr image = this->image();
      assert(image.get() != 0);

      image->readMetadata();
      Exiv2::ExifData &exifData = image->exifData();
      Exiv2::IptcData &iptcData = image->iptcData();
      Exiv2::XmpData &xmpData = image->xmpData();

      // Assign the tags.
      for (tag_map_t::iterator i = tags.begin(); i != tags.end(); ++i) {
        if (i->first.compare(0, 5, "Exif.") == 0) {
          exifData[i->first].setValue(i->second);
        } else if (i->first.compare(0, 5, "Iptc.") == 0) {
          iptcData[i->first].setValue(i->second);
        } else if (i->first.compare(0, 4, "Xmp.") == 0) {
          xmpData[i->first].setValue(i->second);
        } else {
          //std::cerr << "skipping unknown tag " << i->first << std::endl;
        }
      }

      // Write the tag data the image file.
      image->setExifData(exifData);
      image->setIptcData(iptcData);
      image->setXmpData(xmpData);
      image->writeMetadata();
    } catch (std::exception& e) {
      exifException.append(e.what());
    }
  }

  // This function will be run inside the main event loop so it is safe to use
  // V8 again.
  void HandleOKCallback () {
    Nan::HandleScope scope;

    // Create an argument array for any errors.
    Local<Value> argv[1] = { Nan::Null() };
    if (!exifException.empty()) {
      argv[0] = Nan::New<String>(exifException.c_str()).ToLocalChecked();
    }

    // Pass the argv array object to our callback function.
    Nan::Call(callback->GetFunction(), Nan::GetCurrentContext()->Global(), 1, argv);
  };
};

NAN_METHOD(SetImageTags) {
  Nan::HandleScope scope;

  /* Usage arguments */
  if (info.Length() <= 2 || !info[2]->IsFunction())
    return Nan::ThrowTypeError("Usage: <filename> <tags> <callback function>");

  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());
  std::string filename = std::string(*Nan::Utf8String(info[0]));

  SetTagsWorker *worker;
  if (info[0]->IsString()) {
    std::string filename = std::string(*Nan::Utf8String(info[0]));
    worker = new SetTagsWorker(callback, filename);
  } else {
    Local<Object> buf = info[0]->ToObject(v8::Isolate::GetCurrent());
    worker = new SetTagsWorker(callback, Buffer::Data(buf), Buffer::Length(buf));
  }

  Local<Object> tags = Local<Object>::Cast(info[1]);
  Local<Array> keys = Nan::GetOwnPropertyNames(tags).ToLocalChecked();
  for (unsigned i = 0; i < keys->Length(); i++) {
    Local<Value> key = Nan::Get(keys, i).ToLocalChecked();
    worker->tags.insert(std::pair<std::string, std::string> (
      *Nan::Utf8String(key),
      *Nan::Utf8String(Nan::Get(tags, key).ToLocalChecked())
    ));
  }

  Nan::AsyncQueueWorker(worker);
  return;
}

// - - -

class DeleteTagsWorker : public Exiv2Worker {
 public:
  DeleteTagsWorker(Nan::Callback *callback, const std::string fileName)
    : Exiv2Worker(callback, fileName) {}
  DeleteTagsWorker(Nan::Callback *callback, const char* buf, const long len)
    : Exiv2Worker(callback, buf, len) {}
  ~DeleteTagsWorker() {}

  // Should become protected...
  std::vector<std::string> tags;

  // Executed inside the worker-thread. Not safe to access V8, or V8 data
  // structures here.
  void Execute () {
    try {
      Exiv2::Image::AutoPtr image = this->image();
      assert(image.get() != 0);

      image->readMetadata();
      Exiv2::ExifData &exifData = image->exifData();
      Exiv2::IptcData &iptcData = image->iptcData();
      Exiv2::XmpData &xmpData = image->xmpData();

      // Erase the tags.
      for (std::vector<std::string>::iterator i = tags.begin(); i != tags.end(); ++i) {
        if (i->compare(0, 5, "Exif.") == 0) {
          Exiv2::ExifKey *k = new Exiv2::ExifKey(*i);
          exifData.erase(exifData.findKey(*k));
          delete k;
        } else if (i->compare(0, 5, "Iptc.") == 0) {
          Exiv2::IptcKey *k = new Exiv2::IptcKey(*i);
          iptcData.erase(iptcData.findKey(*k));
          delete k;
        } else if (i->compare(0, 4, "Xmp.") == 0) {
          Exiv2::XmpKey *k = new Exiv2::XmpKey(*i);
          xmpData.erase(xmpData.findKey(*k));
          delete k;
        } else {
          //std::cerr << "skipping unknown tag " << i << std::endl;
        }
      }

      // Write the tag data the image file.
      image->setExifData(exifData);
      image->setIptcData(iptcData);
      image->setXmpData(xmpData);
      image->writeMetadata();
    } catch (std::exception& e) {
      exifException.append(e.what());
    }
  }

  // This function will be run inside the main event loop so it is safe to use
  // V8 again.
  void HandleOKCallback () {
    Nan::HandleScope scope;

    // Create an argument array for any errors.
    Local<Value> argv[1] = { Nan::Null() };
    if (!exifException.empty()) {
      argv[0] = Nan::New<String>(exifException.c_str()).ToLocalChecked();
    }

    // Pass the argv array object to our callback function.
    Nan::Call(callback->GetFunction(), Nan::GetCurrentContext()->Global(), 1, argv);
  };
};

NAN_METHOD(DeleteImageTags) {
  Nan::HandleScope scope;

  /* Usage arguments */
  if (info.Length() <= 2 || !info[2]->IsFunction())
    return Nan::ThrowTypeError("Usage: <filename> <tags> <callback function>");

  Nan::Callback *callback = new Nan::Callback(info[2].As<Function>());
  std::string filename = std::string(*Nan::Utf8String(info[0]));

  DeleteTagsWorker *worker;
  if (info[0]->IsString()) {
    std::string filename = std::string(*Nan::Utf8String(info[0]));
    worker = new DeleteTagsWorker(callback, filename);
  } else {
    Local<Object> buf = info[0]->ToObject(v8::Isolate::GetCurrent());
    worker = new DeleteTagsWorker(callback, Buffer::Data(buf), Buffer::Length(buf));
  }

  Local<Array> keys = Local<Array>::Cast(info[1]);
  for (unsigned i = 0; i < keys->Length(); i++) {
    Local<v8::Value> key = Nan::Get(keys, i).ToLocalChecked();
    worker->tags.push_back(*Nan::Utf8String(key));
  }

  Nan::AsyncQueueWorker(worker);
  return;
}

// - - -

class GetPreviewsWorker : public Exiv2Worker {
 public:
  GetPreviewsWorker(Nan::Callback *callback, const std::string fileName)
    : Exiv2Worker(callback, fileName) {}
  GetPreviewsWorker(Nan::Callback *callback, const char* buf, const long len)
    : Exiv2Worker(callback, buf, len) {}
  ~GetPreviewsWorker() {}

  // Executed inside the worker-thread. Not safe to access V8, or V8 data
  // structures here.
  void Execute () {
    try {
      Exiv2::Image::AutoPtr image = this->image();
      assert(image.get() != 0);
      image->readMetadata();

      Exiv2::PreviewManager manager(*image);
      Exiv2::PreviewPropertiesList list = manager.getPreviewProperties();

      // Make sure we're not reusing a baton with memory allocated.
      for (Exiv2::PreviewPropertiesList::iterator pos = list.begin(); pos != list.end(); pos++) {
        Exiv2::PreviewImage image = manager.getPreviewImage(*pos);

        previews.push_back(Preview(image.mimeType(), image.height(),
          image.width(), (char*) image.pData(), image.size()));
      }
    } catch (std::exception& e) {
      exifException.append(e.what());
    }
  }

  // This function will be run inside the main event loop so it is safe to use
  // V8 again.
  void HandleOKCallback () {
    Nan::HandleScope scope;

    Local<Value> argv[2] = { Nan::Null(), Nan::Null() };
    if (!exifException.empty()){
      argv[0] = Nan::New<String>(exifException.c_str()).ToLocalChecked();
    } else {
      // Convert the data into V8 values.
      Local<Array> array = Nan::New<Array>(previews.size());
      for (size_t i = 0; i < previews.size(); ++i) {
        Local<Object> preview = Nan::New<Object>();
        Nan::Set(preview, Nan::New<String>("mimeType").ToLocalChecked(), Nan::New<String>(previews[i].mimeType.c_str()).ToLocalChecked());
        Nan::Set(preview, Nan::New<String>("height").ToLocalChecked(), Nan::New<Number>(previews[i].height));
        Nan::Set(preview, Nan::New<String>("width").ToLocalChecked(), Nan::New<Number>(previews[i].width));
        Nan::Set(preview, Nan::New<String>("data").ToLocalChecked(), Nan::CopyBuffer(previews[i].data, previews[i].size).ToLocalChecked());

        Nan::Set(array, i, preview);
      }
      argv[1] = array;
    }

    // Pass the argv array object to our callback function.
    Nan::Call(callback->GetFunction(), Nan::GetCurrentContext()->Global(), 2, argv);
  };

 protected:

  // Holds a preview image when we copy it from the worker to V8.
  struct Preview {
    Preview(const Preview &p)
      : mimeType(p.mimeType), height(p.height), width(p.width), size(p.size)
    {
      data = new char[size];
      memcpy(data, p.data, size);
    }
    Preview(std::string type_, uint32_t height_, uint32_t width_, const char *data_, size_t size_)
      : mimeType(type_), height(height_), width(width_), size(size_)
    {
      data = new char[size];
      memcpy(data, data_, size);
    }
    ~Preview() {
      delete[] data;
    }

    std::string mimeType;
    uint32_t height;
    uint32_t width;
    size_t size;
    char* data;
  };

  std::vector<Preview> previews;
};

NAN_METHOD(GetImagePreviews) {
  Nan::HandleScope scope;

  /* Usage arguments */
  if (info.Length() <= 1 || !info[1]->IsFunction())
    return Nan::ThrowTypeError("Usage: <filename/buffer> <callback function>");

  Nan::Callback *callback = new Nan::Callback(info[1].As<Function>());
  std::string filename = std::string(*Nan::Utf8String(info[0]));

  if (info[0]->IsString()) {
    std::string filename = std::string(*Nan::Utf8String(info[0]));
    Nan::AsyncQueueWorker(new GetPreviewsWorker(callback, filename));
  } else {
    Local<Object> buf = info[0]->ToObject(v8::Isolate::GetCurrent());
    Nan::AsyncQueueWorker(new GetPreviewsWorker(callback, Buffer::Data(buf), Buffer::Length(buf)));
  }
  return;
}

// - - -
void InitAll(Local<Object> target) {
  Nan::Set(
    target,
    Nan::New<String>("getImageTags").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(GetImageTags)).ToLocalChecked()
  );
  Nan::Set(
    target,
    Nan::New<String>("setImageTags").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(SetImageTags)).ToLocalChecked()
  );
  Nan::Set(
    target,
    Nan::New<String>("deleteImageTags").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(DeleteImageTags)).ToLocalChecked()
  );
  Nan::Set(
    target,
    Nan::New<String>("getImagePreviews").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(GetImagePreviews)).ToLocalChecked()
  );
}
NODE_MODULE(exiv2, InitAll)
