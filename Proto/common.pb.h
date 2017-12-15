// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: common.proto

#ifndef PROTOBUF_common_2eproto__INCLUDED
#define PROTOBUF_common_2eproto__INCLUDED

#include <string>

#include <google/protobuf/stubs/common.h>

#if GOOGLE_PROTOBUF_VERSION < 2005000
#error This file was generated by a newer version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please update
#error your headers.
#endif
#if 2005000 < GOOGLE_PROTOBUF_MIN_PROTOC_VERSION
#error This file was generated by an older version of protoc which is
#error incompatible with your Protocol Buffer headers.  Please
#error regenerate this file with a newer version of protoc.
#endif

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
// @@protoc_insertion_point(includes)

namespace Common {

// Internal implementation detail -- do not call these.
void  protobuf_AddDesc_common_2eproto();
void protobuf_AssignDesc_common_2eproto();
void protobuf_ShutdownFile_common_2eproto();

class HeadFlag;
class Head;
class CommonMsg;

// ===================================================================

class HeadFlag : public ::google::protobuf::Message {
 public:
  HeadFlag();
  virtual ~HeadFlag();

  HeadFlag(const HeadFlag& from);

  inline HeadFlag& operator=(const HeadFlag& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const HeadFlag& default_instance();

  void Swap(HeadFlag* other);

  // implements Message ----------------------------------------------

  HeadFlag* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const HeadFlag& from);
  void MergeFrom(const HeadFlag& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required fixed32 magic = 1;
  inline bool has_magic() const;
  inline void clear_magic();
  static const int kMagicFieldNumber = 1;
  inline ::google::protobuf::uint32 magic() const;
  inline void set_magic(::google::protobuf::uint32 value);

  // required fixed32 headcrc = 2;
  inline bool has_headcrc() const;
  inline void clear_headcrc();
  static const int kHeadcrcFieldNumber = 2;
  inline ::google::protobuf::uint32 headcrc() const;
  inline void set_headcrc(::google::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:Common.HeadFlag)
 private:
  inline void set_has_magic();
  inline void clear_has_magic();
  inline void set_has_headcrc();
  inline void clear_has_headcrc();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 magic_;
  ::google::protobuf::uint32 headcrc_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(2 + 31) / 32];

  friend void  protobuf_AddDesc_common_2eproto();
  friend void protobuf_AssignDesc_common_2eproto();
  friend void protobuf_ShutdownFile_common_2eproto();

  void InitAsDefaultInstance();
  static HeadFlag* default_instance_;
};
// -------------------------------------------------------------------

class Head : public ::google::protobuf::Message {
 public:
  Head();
  virtual ~Head();

  Head(const Head& from);

  inline Head& operator=(const Head& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const Head& default_instance();

  void Swap(Head* other);

  // implements Message ----------------------------------------------

  Head* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const Head& from);
  void MergeFrom(const Head& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required fixed32 bodytype = 1;
  inline bool has_bodytype() const;
  inline void clear_bodytype();
  static const int kBodytypeFieldNumber = 1;
  inline ::google::protobuf::uint32 bodytype() const;
  inline void set_bodytype(::google::protobuf::uint32 value);

  // required fixed32 bodysize = 2;
  inline bool has_bodysize() const;
  inline void clear_bodysize();
  static const int kBodysizeFieldNumber = 2;
  inline ::google::protobuf::uint32 bodysize() const;
  inline void set_bodysize(::google::protobuf::uint32 value);

  // required fixed32 bodycrc = 3;
  inline bool has_bodycrc() const;
  inline void clear_bodycrc();
  static const int kBodycrcFieldNumber = 3;
  inline ::google::protobuf::uint32 bodycrc() const;
  inline void set_bodycrc(::google::protobuf::uint32 value);

  // @@protoc_insertion_point(class_scope:Common.Head)
 private:
  inline void set_has_bodytype();
  inline void clear_has_bodytype();
  inline void set_has_bodysize();
  inline void clear_has_bodysize();
  inline void set_has_bodycrc();
  inline void clear_has_bodycrc();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::uint32 bodytype_;
  ::google::protobuf::uint32 bodysize_;
  ::google::protobuf::uint32 bodycrc_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(3 + 31) / 32];

  friend void  protobuf_AddDesc_common_2eproto();
  friend void protobuf_AssignDesc_common_2eproto();
  friend void protobuf_ShutdownFile_common_2eproto();

  void InitAsDefaultInstance();
  static Head* default_instance_;
};
// -------------------------------------------------------------------

class CommonMsg : public ::google::protobuf::Message {
 public:
  CommonMsg();
  virtual ~CommonMsg();

  CommonMsg(const CommonMsg& from);

  inline CommonMsg& operator=(const CommonMsg& from) {
    CopyFrom(from);
    return *this;
  }

  inline const ::google::protobuf::UnknownFieldSet& unknown_fields() const {
    return _unknown_fields_;
  }

  inline ::google::protobuf::UnknownFieldSet* mutable_unknown_fields() {
    return &_unknown_fields_;
  }

  static const ::google::protobuf::Descriptor* descriptor();
  static const CommonMsg& default_instance();

  void Swap(CommonMsg* other);

  // implements Message ----------------------------------------------

  CommonMsg* New() const;
  void CopyFrom(const ::google::protobuf::Message& from);
  void MergeFrom(const ::google::protobuf::Message& from);
  void CopyFrom(const CommonMsg& from);
  void MergeFrom(const CommonMsg& from);
  void Clear();
  bool IsInitialized() const;

  int ByteSize() const;
  bool MergePartialFromCodedStream(
      ::google::protobuf::io::CodedInputStream* input);
  void SerializeWithCachedSizes(
      ::google::protobuf::io::CodedOutputStream* output) const;
  ::google::protobuf::uint8* SerializeWithCachedSizesToArray(::google::protobuf::uint8* output) const;
  int GetCachedSize() const { return _cached_size_; }
  private:
  void SharedCtor();
  void SharedDtor();
  void SetCachedSize(int size) const;
  public:

  ::google::protobuf::Metadata GetMetadata() const;

  // nested types ----------------------------------------------------

  // accessors -------------------------------------------------------

  // required int32 err = 1;
  inline bool has_err() const;
  inline void clear_err();
  static const int kErrFieldNumber = 1;
  inline ::google::protobuf::int32 err() const;
  inline void set_err(::google::protobuf::int32 value);

  // @@protoc_insertion_point(class_scope:Common.CommonMsg)
 private:
  inline void set_has_err();
  inline void clear_has_err();

  ::google::protobuf::UnknownFieldSet _unknown_fields_;

  ::google::protobuf::int32 err_;

  mutable int _cached_size_;
  ::google::protobuf::uint32 _has_bits_[(1 + 31) / 32];

  friend void  protobuf_AddDesc_common_2eproto();
  friend void protobuf_AssignDesc_common_2eproto();
  friend void protobuf_ShutdownFile_common_2eproto();

  void InitAsDefaultInstance();
  static CommonMsg* default_instance_;
};
// ===================================================================


// ===================================================================

// HeadFlag

// required fixed32 magic = 1;
inline bool HeadFlag::has_magic() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void HeadFlag::set_has_magic() {
  _has_bits_[0] |= 0x00000001u;
}
inline void HeadFlag::clear_has_magic() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void HeadFlag::clear_magic() {
  magic_ = 0u;
  clear_has_magic();
}
inline ::google::protobuf::uint32 HeadFlag::magic() const {
  return magic_;
}
inline void HeadFlag::set_magic(::google::protobuf::uint32 value) {
  set_has_magic();
  magic_ = value;
}

// required fixed32 headcrc = 2;
inline bool HeadFlag::has_headcrc() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void HeadFlag::set_has_headcrc() {
  _has_bits_[0] |= 0x00000002u;
}
inline void HeadFlag::clear_has_headcrc() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void HeadFlag::clear_headcrc() {
  headcrc_ = 0u;
  clear_has_headcrc();
}
inline ::google::protobuf::uint32 HeadFlag::headcrc() const {
  return headcrc_;
}
inline void HeadFlag::set_headcrc(::google::protobuf::uint32 value) {
  set_has_headcrc();
  headcrc_ = value;
}

// -------------------------------------------------------------------

// Head

// required fixed32 bodytype = 1;
inline bool Head::has_bodytype() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void Head::set_has_bodytype() {
  _has_bits_[0] |= 0x00000001u;
}
inline void Head::clear_has_bodytype() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void Head::clear_bodytype() {
  bodytype_ = 0u;
  clear_has_bodytype();
}
inline ::google::protobuf::uint32 Head::bodytype() const {
  return bodytype_;
}
inline void Head::set_bodytype(::google::protobuf::uint32 value) {
  set_has_bodytype();
  bodytype_ = value;
}

// required fixed32 bodysize = 2;
inline bool Head::has_bodysize() const {
  return (_has_bits_[0] & 0x00000002u) != 0;
}
inline void Head::set_has_bodysize() {
  _has_bits_[0] |= 0x00000002u;
}
inline void Head::clear_has_bodysize() {
  _has_bits_[0] &= ~0x00000002u;
}
inline void Head::clear_bodysize() {
  bodysize_ = 0u;
  clear_has_bodysize();
}
inline ::google::protobuf::uint32 Head::bodysize() const {
  return bodysize_;
}
inline void Head::set_bodysize(::google::protobuf::uint32 value) {
  set_has_bodysize();
  bodysize_ = value;
}

// required fixed32 bodycrc = 3;
inline bool Head::has_bodycrc() const {
  return (_has_bits_[0] & 0x00000004u) != 0;
}
inline void Head::set_has_bodycrc() {
  _has_bits_[0] |= 0x00000004u;
}
inline void Head::clear_has_bodycrc() {
  _has_bits_[0] &= ~0x00000004u;
}
inline void Head::clear_bodycrc() {
  bodycrc_ = 0u;
  clear_has_bodycrc();
}
inline ::google::protobuf::uint32 Head::bodycrc() const {
  return bodycrc_;
}
inline void Head::set_bodycrc(::google::protobuf::uint32 value) {
  set_has_bodycrc();
  bodycrc_ = value;
}

// -------------------------------------------------------------------

// CommonMsg

// required int32 err = 1;
inline bool CommonMsg::has_err() const {
  return (_has_bits_[0] & 0x00000001u) != 0;
}
inline void CommonMsg::set_has_err() {
  _has_bits_[0] |= 0x00000001u;
}
inline void CommonMsg::clear_has_err() {
  _has_bits_[0] &= ~0x00000001u;
}
inline void CommonMsg::clear_err() {
  err_ = 0;
  clear_has_err();
}
inline ::google::protobuf::int32 CommonMsg::err() const {
  return err_;
}
inline void CommonMsg::set_err(::google::protobuf::int32 value) {
  set_has_err();
  err_ = value;
}


// @@protoc_insertion_point(namespace_scope)

}  // namespace Common

#ifndef SWIG
namespace google {
namespace protobuf {


}  // namespace google
}  // namespace protobuf
#endif  // SWIG

// @@protoc_insertion_point(global_scope)

#endif  // PROTOBUF_common_2eproto__INCLUDED
