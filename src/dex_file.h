// Copyright 2011 Google Inc. All Rights Reserved.

#ifndef ART_SRC_DEX_FILE_H_
#define ART_SRC_DEX_FILE_H_

#include <map>
#include <string>
#include <vector>

#include "UniquePtr.h"
#include "globals.h"
#include "jni.h"
#include "logging.h"
#include "mem_map.h"
#include "mutex.h"
#include "stringpiece.h"
#include "utils.h"

namespace art {

class ZipArchive;

// TODO: move all of the macro functionality into the DexCache class.
class DexFile {
 public:
  static const byte kDexMagic[];
  static const byte kDexMagicVersion[];
  static const size_t kSha1DigestSize = 20;

  // name of the DexFile entry within a zip archive
  static const char* kClassesDex;

  // The value of an invalid index.
  static const uint32_t kDexNoIndex = 0xFFFFFFFF;

  // The value of an invalid index.
  static const uint16_t kDexNoIndex16 = 0xFFFF;

  // Raw header_item.
  struct Header {
    uint8_t magic_[8];
    uint32_t checksum_;
    uint8_t signature_[kSha1DigestSize];
    uint32_t file_size_;  // length of entire file
    uint32_t header_size_;  // offset to start of next section
    uint32_t endian_tag_;
    uint32_t link_size_;  // unused
    uint32_t link_off_;  // unused
    uint32_t map_off_;  // unused
    uint32_t string_ids_size_;  // number of StringIds
    uint32_t string_ids_off_;  // file offset of StringIds array
    uint32_t type_ids_size_;  // number of TypeIds, we don't support more than 65535
    uint32_t type_ids_off_;  // file offset of TypeIds array
    uint32_t proto_ids_size_;  // number of ProtoIds, we don't support more than 65535
    uint32_t proto_ids_off_;  // file offset of ProtoIds array
    uint32_t field_ids_size_;  // number of FieldIds
    uint32_t field_ids_off_;  // file offset of FieldIds array
    uint32_t method_ids_size_;  // number of MethodIds
    uint32_t method_ids_off_;  // file offset of MethodIds array
    uint32_t class_defs_size_;  // number of ClassDefs
    uint32_t class_defs_off_;  // file offset of ClassDef array
    uint32_t data_size_;  // unused
    uint32_t data_off_;  // unused
   private:
    DISALLOW_COPY_AND_ASSIGN(Header);
  };

  // Raw string_id_item.
  struct StringId {
    uint32_t string_data_off_;  // offset in bytes from the base address
   private:
    DISALLOW_COPY_AND_ASSIGN(StringId);
  };

  // Raw type_id_item.
  struct TypeId {
    uint32_t descriptor_idx_;  // index into string_ids
   private:
    DISALLOW_COPY_AND_ASSIGN(TypeId);
  };

  // Raw field_id_item.
  struct FieldId {
    uint16_t class_idx_;  // index into type_ids_ array for defining class
    uint16_t type_idx_;  // index into type_ids_ array for field type
    uint32_t name_idx_;  // index into string_ids_ array for field name
   private:
    DISALLOW_COPY_AND_ASSIGN(FieldId);
  };

  // Raw method_id_item.
  struct MethodId {
    uint16_t class_idx_;  // index into type_ids_ array for defining class
    uint16_t proto_idx_;  // index into proto_ids_ array for method prototype
    uint32_t name_idx_;  // index into string_ids_ array for method name
   private:
    DISALLOW_COPY_AND_ASSIGN(MethodId);
  };

  // Raw proto_id_item.
  struct ProtoId {
    uint32_t shorty_idx_;  // index into string_ids array for shorty descriptor
    uint16_t return_type_idx_;  // index into type_ids array for return type
    uint16_t pad_;             // padding = 0
    uint32_t parameters_off_;  // file offset to type_list for parameter types
   private:
    DISALLOW_COPY_AND_ASSIGN(ProtoId);
  };

  // Raw class_def_item.
  struct ClassDef {
    uint16_t class_idx_;  // index into type_ids_ array for this class
    uint16_t pad1_;  // padding = 0
    uint32_t access_flags_;
    uint16_t superclass_idx_;  // index into type_ids_ array for superclass
    uint16_t pad2_;  // padding = 0
    uint32_t interfaces_off_;  // file offset to TypeList
    uint32_t source_file_idx_;  // index into string_ids_ for source file name
    uint32_t annotations_off_;  // file offset to annotations_directory_item
    uint32_t class_data_off_;  // file offset to class_data_item
    uint32_t static_values_off_;  // file offset to EncodedArray
   private:
    DISALLOW_COPY_AND_ASSIGN(ClassDef);
  };

  // Raw type_item.
  struct TypeItem {
    uint16_t type_idx_;  // index into type_ids section
   private:
    DISALLOW_COPY_AND_ASSIGN(TypeItem);
  };

  // Raw type_list.
  class TypeList {
   public:
    uint32_t Size() const {
      return size_;
    }

    const TypeItem& GetTypeItem(uint32_t idx) const {
      CHECK_LT(idx, this->size_);
      return this->list_[idx];
    }

   private:
    uint32_t size_;  // size of the list, in entries
    TypeItem list_[1];  // elements of the list
    DISALLOW_COPY_AND_ASSIGN(TypeList);
  };

  // Raw code_item.
  struct CodeItem {
    uint16_t registers_size_;
    uint16_t ins_size_;
    uint16_t outs_size_;
    uint16_t tries_size_;
    uint32_t debug_info_off_;  // file offset to debug info stream
    uint32_t insns_size_in_code_units_;  // size of the insns array, in 2 byte code units
    uint16_t insns_[1];
   private:
    DISALLOW_COPY_AND_ASSIGN(CodeItem);
  };

  // Raw try_item.
  struct TryItem {
    uint32_t start_addr_;
    uint16_t insn_count_;
    uint16_t handler_off_;
   private:
    DISALLOW_COPY_AND_ASSIGN(TryItem);
  };

  typedef std::pair<const DexFile*, const DexFile::ClassDef*> ClassPathEntry;
  typedef std::vector<const DexFile*> ClassPath;

  // Search a collection of DexFiles for a descriptor
  static ClassPathEntry FindInClassPath(const StringPiece& descriptor,
                                        const ClassPath& class_path);

  // Opens a collection of .dex files
  static void OpenDexFiles(const std::vector<const char*>& dex_filenames,
                           std::vector<const DexFile*>& dex_files,
                           const std::string& strip_location_prefix);

  // Opens .dex file, guessing the container format based on file extension
  static const DexFile* Open(const std::string& filename,
                             const std::string& strip_location_prefix);

  // Opens .dex file, backed by existing memory
  static const DexFile* Open(const uint8_t* base, size_t length, const std::string& location) {
    return OpenMemory(base, length, location, NULL);
  }

  // Opens .dex file from the classes.dex in a zip archive
  static const DexFile* Open(const ZipArchive& zip_archive, const std::string& location);

  // Closes a .dex file.
  virtual ~DexFile();

  const std::string& GetLocation() const {
    return location_;
  }

  // Returns a com.android.dex.Dex object corresponding to the mapped-in dex file.
  // Used by managed code to implement annotations.
  jobject GetDexObject(JNIEnv* env) const;

  const Header& GetHeader() const {
    CHECK(header_ != NULL) << GetLocation();
    return *header_;
  }

  // Decode the dex magic version
  uint32_t GetVersion() const;

  // Returns the number of string identifiers in the .dex file.
  size_t NumStringIds() const {
    CHECK(header_ != NULL) << GetLocation();
    return header_->string_ids_size_;
  }

  // Returns the StringId at the specified index.
  const StringId& GetStringId(uint32_t idx) const {
    CHECK_LT(idx, NumStringIds()) << GetLocation();
    return string_ids_[idx];
  }

  uint32_t GetIndexForStringId(const StringId& string_id) const {
    CHECK_GE(&string_id, string_ids_) << GetLocation();
    CHECK_LT(&string_id, string_ids_ + header_->string_ids_size_) << GetLocation();
    return &string_id - string_ids_;
  }

  int32_t GetStringLength(const StringId& string_id) const;

  // Returns a pointer to the UTF-8 string data referred to by the given string_id.
  const char* GetStringDataAndLength(const StringId& string_id, int32_t* length) const;

  const char* GetStringData(const StringId& string_id) const {
    int32_t length;
    return GetStringDataAndLength(string_id, &length);
  }

  // return the UTF-8 encoded string with the specified string_id index
  const char* StringDataAndLengthByIdx(uint32_t idx, int32_t* unicode_length) const {
    if (idx == kDexNoIndex) {
      *unicode_length = 0;
      return NULL;
    }
    const StringId& string_id = GetStringId(idx);
    return GetStringDataAndLength(string_id, unicode_length);
  }

  const char* StringDataByIdx(uint32_t idx) const {
    int32_t unicode_length;
    return StringDataAndLengthByIdx(idx, &unicode_length);
  }

  // Looks up a string id for a given string
  const StringId* FindStringId(const std::string& string) const;

  // Returns the number of type identifiers in the .dex file.
  size_t NumTypeIds() const {
    CHECK(header_ != NULL) << GetLocation();
    return header_->type_ids_size_;
  }

  // Returns the TypeId at the specified index.
  const TypeId& GetTypeId(uint32_t idx) const {
    CHECK_LT(idx, NumTypeIds()) << GetLocation();
    return type_ids_[idx];
  }

  uint16_t GetIndexForTypeId(const TypeId& type_id) const {
    CHECK_GE(&type_id, type_ids_) << GetLocation();
    CHECK_LT(&type_id, type_ids_ + header_->type_ids_size_) << GetLocation();
    size_t result = &type_id - type_ids_;
    DCHECK_LT(result, 65536U) << GetLocation();
    return static_cast<uint16_t>(result);
  }

  // Get the descriptor string associated with a given type index.
  const char* StringByTypeIdx(uint32_t idx, int32_t* unicode_length) const {
    const TypeId& type_id = GetTypeId(idx);
    return StringDataAndLengthByIdx(type_id.descriptor_idx_, unicode_length);
  }

  const char* StringByTypeIdx(uint32_t idx) const {
    const TypeId& type_id = GetTypeId(idx);
    return StringDataByIdx(type_id.descriptor_idx_);
  }

  // Returns the type descriptor string of a type id.
  const char* GetTypeDescriptor(const TypeId& type_id) const {
    return StringDataByIdx(type_id.descriptor_idx_);
  }

  // Looks up a type for the given string index
  const TypeId* FindTypeId(uint32_t string_idx) const;

  // Returns the number of field identifiers in the .dex file.
  size_t NumFieldIds() const {
    CHECK(header_ != NULL) << GetLocation();
    return header_->field_ids_size_;
  }

  // Returns the FieldId at the specified index.
  const FieldId& GetFieldId(uint32_t idx) const {
    CHECK_LT(idx, NumFieldIds()) << GetLocation();
    return field_ids_[idx];
  }

  uint32_t GetIndexForFieldId(const FieldId& field_id) const {
    CHECK_GE(&field_id, field_ids_) << GetLocation();
    CHECK_LT(&field_id, field_ids_ + header_->field_ids_size_) << GetLocation();
    return &field_id - field_ids_;
  }

  // Looks up a field by its declaring class, name and type
  const FieldId* FindFieldId(const DexFile::TypeId& declaring_klass,
                             const DexFile::StringId& name,
                             const DexFile::TypeId& type) const;

  // Returns the declaring class descriptor string of a field id.
  const char* GetFieldDeclaringClassDescriptor(const FieldId& field_id) const {
    const DexFile::TypeId& type_id = GetTypeId(field_id.class_idx_);
    return GetTypeDescriptor(type_id);
  }

  // Returns the class descriptor string of a field id.
  const char* GetFieldTypeDescriptor(const FieldId& field_id) const {
    const DexFile::TypeId& type_id = GetTypeId(field_id.type_idx_);
    return GetTypeDescriptor(type_id);
  }

  // Returns the name of a field id.
  const char* GetFieldName(const FieldId& field_id) const {
    return StringDataByIdx(field_id.name_idx_);
  }

  // Returns the number of method identifiers in the .dex file.
  size_t NumMethodIds() const {
    CHECK(header_ != NULL) << GetLocation();
    return header_->method_ids_size_;
  }

  // Returns the MethodId at the specified index.
  const MethodId& GetMethodId(uint32_t idx) const {
    CHECK_LT(idx, NumMethodIds()) << GetLocation();
    return method_ids_[idx];
  }

  uint32_t GetIndexForMethodId(const MethodId& method_id) const {
    CHECK_GE(&method_id, method_ids_) << GetLocation();
    CHECK_LT(&method_id, method_ids_ + header_->method_ids_size_) << GetLocation();
    return &method_id - method_ids_;
  }

  // Looks up a method by its declaring class, name and proto_id
  const MethodId* FindMethodId(const DexFile::TypeId& declaring_klass,
                               const DexFile::StringId& name,
                               const DexFile::ProtoId& signature) const;

  // Returns the declaring class descriptor string of a method id.
  const char* GetMethodDeclaringClassDescriptor(const MethodId& method_id) const {
    const DexFile::TypeId& type_id = GetTypeId(method_id.class_idx_);
    return GetTypeDescriptor(type_id);
  }

  // Returns the prototype of a method id.
  const ProtoId& GetMethodPrototype(const MethodId& method_id) const {
    return GetProtoId(method_id.proto_idx_);
  }

  // Returns the signature of a method id.
  const std::string GetMethodSignature(const MethodId& method_id) const {
    return CreateMethodSignature(method_id.proto_idx_, NULL);
  }

  // Returns the name of a method id.
  const char* GetMethodName(const MethodId& method_id) const {
    return StringDataByIdx(method_id.name_idx_);
  }

  // Returns the shorty of a method id.
  const char* GetMethodShorty(const MethodId& method_id) const {
    return StringDataByIdx(GetProtoId(method_id.proto_idx_).shorty_idx_);
  }
  const char* GetMethodShorty(const MethodId& method_id, int32_t* length) const {
    return StringDataAndLengthByIdx(GetProtoId(method_id.proto_idx_).shorty_idx_, length);
  }
  // Returns the number of class definitions in the .dex file.
  size_t NumClassDefs() const {
    CHECK(header_ != NULL) << GetLocation();
    return header_->class_defs_size_;
  }

  // Returns the ClassDef at the specified index.
  const ClassDef& GetClassDef(uint32_t idx) const {
    CHECK_LT(idx, NumClassDefs()) << GetLocation();
    return class_defs_[idx];
  }

  uint32_t GetIndexForClassDef(const ClassDef& class_def) const {
    CHECK_GE(&class_def, class_defs_) << GetLocation();
    CHECK_LT(&class_def, class_defs_ + header_->class_defs_size_) << GetLocation();
    return &class_def - class_defs_;
  }

  // Returns the class descriptor string of a class definition.
  const char* GetClassDescriptor(const ClassDef& class_def) const {
    return StringByTypeIdx(class_def.class_idx_);
  }

  // Looks up a class definition by its class descriptor.
  const ClassDef* FindClassDef(const StringPiece& descriptor) const;

  // Looks up a class definition index by its class descriptor.
  bool FindClassDefIndex(const StringPiece& descriptor, uint32_t& idx) const;

  const TypeList* GetInterfacesList(const ClassDef& class_def) const {
    if (class_def.interfaces_off_ == 0) {
        return NULL;
    } else {
      const byte* addr = base_ + class_def.interfaces_off_;
      return reinterpret_cast<const TypeList*>(addr);
    }
  }

  // Returns a pointer to the raw memory mapped class_data_item
  const byte* GetClassData(const ClassDef& class_def) const {
    if (class_def.class_data_off_ == 0) {
      return NULL;
    } else {
      return base_ + class_def.class_data_off_;
    }
  }

  //
  const CodeItem* GetCodeItem(const uint32_t code_off) const {
    if (code_off == 0) {
      return NULL;  // native or abstract method
    } else {
      const byte* addr = base_ + code_off;
      return reinterpret_cast<const CodeItem*>(addr);
    }
  }

  const char* GetReturnTypeDescriptor(const ProtoId& proto_id) const {
    return StringByTypeIdx(proto_id.return_type_idx_);
  }

  // Returns the number of prototype identifiers in the .dex file.
  size_t NumProtoIds() const {
    CHECK(header_ != NULL) << GetLocation();
    return header_->proto_ids_size_;
  }

  // Returns the ProtoId at the specified index.
  const ProtoId& GetProtoId(uint32_t idx) const {
    CHECK_LT(idx, NumProtoIds()) << GetLocation();
    return proto_ids_[idx];
  }

  uint16_t GetIndexForProtoId(const ProtoId& proto_id) const {
    CHECK_GE(&proto_id, proto_ids_) << GetLocation();
    CHECK_LT(&proto_id, proto_ids_ + header_->proto_ids_size_) << GetLocation();
    return &proto_id - proto_ids_;
  }

  // Looks up a proto id for a given return type and signature type list
  const ProtoId* FindProtoId(uint16_t return_type_id,
                             const std::vector<uint16_t>& signature_type_idxs_) const;

  // Given a signature place the type ids into the given vector, returns true on success
  bool CreateTypeList(uint16_t* return_type_idx, std::vector<uint16_t>* param_type_idxs,
                      const std::string& signature) const;

  // Given a proto_idx decode the type list and return type into a method signature
  std::string CreateMethodSignature(uint32_t proto_idx, int32_t* unicode_length) const;

  // Returns the short form method descriptor for the given prototype.
  const char* GetShorty(uint32_t proto_idx) const {
    const ProtoId& proto_id = GetProtoId(proto_idx);
    return StringDataByIdx(proto_id.shorty_idx_);
  }

  const TypeList* GetProtoParameters(const ProtoId& proto_id) const {
    if (proto_id.parameters_off_ == 0) {
      return NULL;
    } else {
      const byte* addr = base_ + proto_id.parameters_off_;
      return reinterpret_cast<const TypeList*>(addr);
    }
  }

  const byte* GetEncodedStaticFieldValuesArray(const ClassDef& class_def) const {
    if (class_def.static_values_off_ == 0) {
      return 0;
    } else {
      return base_ + class_def.static_values_off_;
    }
  }

  static const TryItem* GetTryItems(const CodeItem& code_item, uint32_t offset) {
    const uint16_t* insns_end_ = &code_item.insns_[code_item.insns_size_in_code_units_];
    return reinterpret_cast<const TryItem*>
        (RoundUp(reinterpret_cast<uint32_t>(insns_end_), 4)) + offset;
  }

  // Get the base of the encoded data for the given DexCode.
  static const byte* GetCatchHandlerData(const CodeItem& code_item, uint32_t offset) {
    const byte* handler_data =
        reinterpret_cast<const byte*>(GetTryItems(code_item, code_item.tries_size_));
    return handler_data + offset;
  }

  // Find the handler associated with a given address, if any.
  // Initializes the given iterator and returns true if a match is
  // found. Returns end if there is no applicable handler.
  static int32_t FindCatchHandlerOffset(const CodeItem &code_item, int32_t tries_size,
                                        uint32_t address);

  // Get the pointer to the start of the debugging data
  const byte* GetDebugInfoStream(const CodeItem* code_item) const {
    if (code_item->debug_info_off_ == 0) {
      return NULL;
    } else {
      return base_ + code_item->debug_info_off_;
    }
  }

  // Callback for "new position table entry".
  // Returning true causes the decoder to stop early.
  typedef bool (*DexDebugNewPositionCb)(void* cnxt, uint32_t address, uint32_t line_num);

  // Callback for "new locals table entry". "signature" is an empty string
  // if no signature is available for an entry.
  typedef void (*DexDebugNewLocalCb)(void* cnxt, uint16_t reg,
                                     uint32_t startAddress,
                                     uint32_t endAddress,
                                     const char* name,
                                     const char* descriptor,
                                     const char* signature);

  static bool LineNumForPcCb(void* cnxt, uint32_t address, uint32_t line_num);

  // Debug info opcodes and constants
  enum {
    DBG_END_SEQUENCE         = 0x00,
    DBG_ADVANCE_PC           = 0x01,
    DBG_ADVANCE_LINE         = 0x02,
    DBG_START_LOCAL          = 0x03,
    DBG_START_LOCAL_EXTENDED = 0x04,
    DBG_END_LOCAL            = 0x05,
    DBG_RESTART_LOCAL        = 0x06,
    DBG_SET_PROLOGUE_END     = 0x07,
    DBG_SET_EPILOGUE_BEGIN   = 0x08,
    DBG_SET_FILE             = 0x09,
    DBG_FIRST_SPECIAL        = 0x0a,
    DBG_LINE_BASE            = -4,
    DBG_LINE_RANGE           = 15,
  };

  struct LocalInfo {
    LocalInfo() : name_(NULL), descriptor_(NULL), signature_(NULL), start_address_(0),
        is_live_(false) {}

    const char* name_;  // E.g., list
    const char* descriptor_;  // E.g., Ljava/util/LinkedList;
    const char* signature_;  // E.g., java.util.LinkedList<java.lang.Integer>
    uint16_t start_address_;  // PC location where the local is first defined.
    bool is_live_;  // Is the local defined and live.

   private:
    DISALLOW_COPY_AND_ASSIGN(LocalInfo);
  };

  struct LineNumFromPcContext {
    LineNumFromPcContext(uint32_t address, uint32_t line_num) :
                           address_(address), line_num_(line_num) {}
    uint32_t address_;
    uint32_t line_num_;
   private:
    DISALLOW_COPY_AND_ASSIGN(LineNumFromPcContext);
  };

  void InvokeLocalCbIfLive(void* cnxt, int reg, uint32_t end_address,
                           LocalInfo* local_in_reg, DexDebugNewLocalCb local_cb) const {
    if (local_cb != NULL && local_in_reg[reg].is_live_) {
      local_cb(cnxt, reg, local_in_reg[reg].start_address_, end_address,
          local_in_reg[reg].name_, local_in_reg[reg].descriptor_,
          local_in_reg[reg].signature_ != NULL ? local_in_reg[reg].signature_ : "");
    }
  }

  // Determine the source file line number based on the program counter.
  // "pc" is an offset, in 16-bit units, from the start of the method's code.
  //
  // Returns -1 if no match was found (possibly because the source files were
  // compiled without "-g", so no line number information is present).
  // Returns -2 for native methods (as expected in exception traces).
  //
  // This is used by runtime; therefore use art::Method not art::DexFile::Method.
  int32_t GetLineNumFromPC(const Method* method, uint32_t rel_pc) const;

  void DecodeDebugInfo(const CodeItem* code_item, bool is_static, uint32_t method_idx,
                       DexDebugNewPositionCb posCb, DexDebugNewLocalCb local_cb,
                       void* cnxt) const;

  const char* GetSourceFile(const ClassDef& class_def) const {
    if (class_def.source_file_idx_ == 0xffffffff) {
      return NULL;
    } else {
      return StringDataByIdx(class_def.source_file_idx_);
    }
  }

  void ChangePermissions(int prot) const;

 private:

  // Opens a .dex file
  static const DexFile* OpenFile(const std::string& filename,
                                 const std::string& original_location,
                                 const std::string& strip_location_prefix);

  // Opens a dex file from within a .jar, .zip, or .apk file
  static const DexFile* OpenZip(const std::string& filename,
                                const std::string& strip_location_prefix);

  // Opens a .dex file at the given address backed by a MemMap
  static const DexFile* OpenMemory(const std::string& location,
                                   MemMap* mem_map) {
    return OpenMemory(mem_map->GetAddress(),
                      mem_map->GetLength(),
                      location,
                      mem_map);
  }

  // Opens a .dex file at the given address, optionally backed by a MemMap
  static const DexFile* OpenMemory(const byte* dex_file,
                                   size_t length,
                                   const std::string& location,
                                   MemMap* mem_map);

  DexFile(const byte* base, size_t length, const std::string& location, MemMap* mem_map)
      : base_(base),
        length_(length),
        location_(location),
        mem_map_(mem_map),
        dex_object_lock_("a dex_object_lock_"),
        dex_object_(NULL),
        header_(0),
        string_ids_(0),
        type_ids_(0),
        field_ids_(0),
        method_ids_(0),
        proto_ids_(0),
        class_defs_(0) {
    CHECK(base_ != NULL) << GetLocation();
    CHECK_GT(length_, 0U) << GetLocation();
  }

  // Top-level initializer that calls other Init methods.
  bool Init();

  // Caches pointers into to the various file sections.
  void InitMembers();

  // Builds the index of descriptors to class definitions.
  void InitIndex();

  // Returns true if the byte string equals the magic value.
  bool CheckMagic(const byte* magic);

  // Returns true if the header magic is of the expected value.
  bool IsMagicValid();

  void DecodeDebugInfo0(const CodeItem* code_item, bool is_static, uint32_t method_idx,
      DexDebugNewPositionCb posCb, DexDebugNewLocalCb local_cb,
      void* cnxt, const byte* stream, LocalInfo* local_in_reg) const;


  // The index of descriptors to class definition indexes (as opposed to type id indexes)
  typedef std::map<const StringPiece, uint32_t> Index;
  Index index_;

  // The base address of the memory mapping.
  const byte* base_;

  // The size of the underlying memory allocation in bytes.
  size_t length_;

  // Typically the dex file name when available, alternatively some identifying string.
  //
  // The ClassLinker will use this to match DexFiles the boot class
  // path to DexCache::GetLocation when loading from an image.
  const std::string location_;

  // Manages the underlying memory allocation.
  UniquePtr<MemMap> mem_map_;

  // A cached com.android.dex.Dex instance, possibly NULL. Use GetDexObject.
  mutable Mutex dex_object_lock_;
  mutable jobject dex_object_;

  // Points to the header section.
  const Header* header_;

  // Points to the base of the string identifier list.
  const StringId* string_ids_;

  // Points to the base of the type identifier list.
  const TypeId* type_ids_;

  // Points to the base of the field identifier list.
  const FieldId* field_ids_;

  // Points to the base of the method identifier list.
  const MethodId* method_ids_;

  // Points to the base of the prototype identifier list.
  const ProtoId* proto_ids_;

  // Points to the base of the class definition list.
  const ClassDef* class_defs_;
};

// Iterate over a dex file's ProtoId's paramters
class DexFileParameterIterator {
 public:
  DexFileParameterIterator(const DexFile& dex_file, const DexFile::ProtoId& proto_id)
      : dex_file_(dex_file), size_(0), pos_(0) {
    type_list_ = dex_file_.GetProtoParameters(proto_id);
    if (type_list_ != NULL) {
      size_ = type_list_->Size();
    }
  }
  bool HasNext() const { return pos_ < size_; }
  void Next() { ++pos_; }
  uint16_t GetTypeIdx() {
    return type_list_->GetTypeItem(pos_).type_idx_;
  }
  const char* GetDescriptor() {
    return dex_file_.StringByTypeIdx(GetTypeIdx());
  }
 private:
  const DexFile& dex_file_;
  const DexFile::TypeList* type_list_;
  uint32_t size_;
  uint32_t pos_;
  DISALLOW_IMPLICIT_CONSTRUCTORS(DexFileParameterIterator);
};

// Iterate and decode class_data_item
class ClassDataItemIterator {
 public:
  ClassDataItemIterator(const DexFile& dex_file, const byte* raw_class_data_item)
      : dex_file_(dex_file), pos_(0), ptr_pos_(raw_class_data_item), last_idx_(0) {
    ReadClassDataHeader();
    if (EndOfInstanceFieldsPos() > 0) {
      ReadClassDataField();
    } else if (EndOfVirtualMethodsPos() > 0) {
      ReadClassDataMethod();
    }
  }
  uint32_t NumStaticFields() const {
    return header_.static_fields_size_;
  }
  uint32_t NumInstanceFields() const {
    return header_.instance_fields_size_;
  }
  uint32_t NumDirectMethods() const {
    return header_.direct_methods_size_;
  }
  uint32_t NumVirtualMethods() const {
    return header_.virtual_methods_size_;
  }
  bool HasNextStaticField() const {
    return pos_ < EndOfStaticFieldsPos();
  }
  bool HasNextInstanceField() const {
    return pos_ >= EndOfStaticFieldsPos() && pos_ < EndOfInstanceFieldsPos();
  }
  bool HasNextDirectMethod() const {
    return pos_ >= EndOfInstanceFieldsPos() && pos_ < EndOfDirectMethodsPos();
  }
  bool HasNextVirtualMethod() const {
    return pos_ >= EndOfDirectMethodsPos() && pos_ < EndOfVirtualMethodsPos();
  }
  bool HasNext() const {
    return pos_ < EndOfVirtualMethodsPos();
  }
  void Next() {
    pos_++;
    if (pos_ < EndOfStaticFieldsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataField();
    } else if (pos_ == EndOfStaticFieldsPos() && NumInstanceFields() > 0) {
      last_idx_ = 0;  // transition to next array, reset last index
      ReadClassDataField();
    } else if (pos_ < EndOfInstanceFieldsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataField();
    } else if (pos_ == EndOfInstanceFieldsPos() && NumDirectMethods() > 0) {
      last_idx_ = 0;  // transition to next array, reset last index
      ReadClassDataMethod();
    } else if (pos_ < EndOfDirectMethodsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataMethod();
    } else if (pos_ == EndOfDirectMethodsPos() && NumVirtualMethods() > 0) {
      last_idx_ = 0;  // transition to next array, reset last index
      ReadClassDataMethod();
    } else if (pos_ < EndOfVirtualMethodsPos()) {
      last_idx_ = GetMemberIndex();
      ReadClassDataMethod();
    } else {
      DCHECK(!HasNext());
    }
  }
  uint32_t GetMemberIndex() const {
    if (pos_ < EndOfInstanceFieldsPos()) {
      return last_idx_ + field_.field_idx_delta_;
    } else {
      CHECK_LT(pos_, EndOfVirtualMethodsPos());
      return last_idx_ + method_.method_idx_delta_;
    }
  }
  uint32_t GetMemberAccessFlags() const {
    if (pos_ < EndOfInstanceFieldsPos()) {
      return field_.access_flags_;
    } else {
      CHECK_LT(pos_, EndOfVirtualMethodsPos());
      return method_.access_flags_;
    }
  }
  const DexFile::CodeItem* GetMethodCodeItem() const {
    return dex_file_.GetCodeItem(method_.code_off_);
  }
  uint32_t GetMethodCodeItemOffset() const {
    return method_.code_off_;
  }
 private:
  // A dex file's class_data_item is leb128 encoded, this structure holds a decoded form of the
  // header for a class_data_item
  struct ClassDataHeader {
    uint32_t static_fields_size_;  // the number of static fields
    uint32_t instance_fields_size_;  // the number of instance fields
    uint32_t direct_methods_size_;  // the number of direct methods
    uint32_t virtual_methods_size_;  // the number of virtual methods
  } header_;

  // Read and decode header from a class_data_item stream into header
  void ReadClassDataHeader();

  uint32_t EndOfStaticFieldsPos() const {
    return header_.static_fields_size_;
  }
  uint32_t EndOfInstanceFieldsPos() const {
    return EndOfStaticFieldsPos() + header_.instance_fields_size_;
  }
  uint32_t EndOfDirectMethodsPos() const {
    return EndOfInstanceFieldsPos() + header_.direct_methods_size_;
  }
  uint32_t EndOfVirtualMethodsPos() const {
    return EndOfDirectMethodsPos() + header_.virtual_methods_size_;
  }

  // A decoded version of the field of a class_data_item
  struct ClassDataField {
    uint32_t field_idx_delta_;  // delta of index into the field_ids array for FieldId
    uint32_t access_flags_;  // access flags for the field
    ClassDataField() :  field_idx_delta_(0), access_flags_(0) {}
   private:
    DISALLOW_COPY_AND_ASSIGN(ClassDataField);
  } field_;

  // Read and decode a field from a class_data_item stream into field
  void ReadClassDataField();

  // A decoded version of the method of a class_data_item
  struct ClassDataMethod {
    uint32_t method_idx_delta_;  // delta of index into the method_ids array for MethodId
    uint32_t access_flags_;
    uint32_t code_off_;
    ClassDataMethod() : method_idx_delta_(0), access_flags_(0), code_off_(0) {}
   private:
    DISALLOW_COPY_AND_ASSIGN(ClassDataMethod);
  } method_;

  // Read and decode a method from a class_data_item stream into method
  void ReadClassDataMethod();

  const DexFile& dex_file_;
  size_t pos_;  // integral number of items passed
  const byte* ptr_pos_;  // pointer into stream of class_data_item
  uint32_t last_idx_;  // last read field or method index to apply delta to
  DISALLOW_IMPLICIT_CONSTRUCTORS(ClassDataItemIterator);
};

class ClassLinker;
class DexCache;
class Field;

class EncodedStaticFieldValueIterator {
 public:
  EncodedStaticFieldValueIterator(const DexFile& dex_file, DexCache* dex_cache,
                                  ClassLinker* linker, const DexFile::ClassDef& class_def);

  void ReadValueToField(Field* field) const;

  bool HasNext() { return pos_ < array_size_; }

  void Next();
 private:
  enum ValueType {
    kByte = 0x00,
    kShort = 0x02,
    kChar = 0x03,
    kInt = 0x04,
    kLong = 0x06,
    kFloat = 0x10,
    kDouble = 0x11,
    kString = 0x17,
    kType = 0x18,
    kField = 0x19,
    kMethod = 0x1a,
    kEnum = 0x1b,
    kArray = 0x1c,
    kAnnotation = 0x1d,
    kNull = 0x1e,
    kBoolean = 0x1f
  };

  static const byte kEncodedValueTypeMask = 0x1f;  // 0b11111
  static const byte kEncodedValueArgShift = 5;

  const DexFile& dex_file_;
  DexCache* dex_cache_;  // dex cache to resolve literal objects
  ClassLinker* linker_;  // linker to resolve literal objects
  size_t array_size_;  // size of array
  size_t pos_;  // current position
  const byte* ptr_;  // pointer into encoded data array
  byte type_;  // type of current encoded value
  jvalue jval_;  // value of current encoded value
  DISALLOW_IMPLICIT_CONSTRUCTORS(EncodedStaticFieldValueIterator);
};

class CatchHandlerIterator {
  public:
    CatchHandlerIterator(const DexFile::CodeItem& code_item, uint32_t address);
    explicit CatchHandlerIterator(const byte* handler_data) {
      Init(handler_data);
    }

    uint16_t GetHandlerTypeIndex() const {
      return handler_.type_idx_;
    }
    uint32_t GetHandlerAddress() const {
      return handler_.address_;
    }
    void Next();
    bool HasNext() const {
      return remaining_count_ != -1 || catch_all_;
    }
    // End of this set of catch blocks, convenience method to locate next set of catch blocks
    const byte* EndDataPointer() const {
      CHECK(!HasNext());
      return current_data_;
    }
  private:
    void Init(const byte* handler_data);

    struct CatchHandlerItem {
      uint16_t type_idx_;  // type index of the caught exception type
      uint32_t address_;  // handler address
    } handler_;
    const byte *current_data_;  // the current handler in dex file.
    int32_t remaining_count_;   // number of handlers not read.
    bool catch_all_;            // is there a handler that will catch all exceptions in case
                                // that all typed handler does not match.
};

}  // namespace art

#endif  // ART_SRC_DEX_FILE_H_
