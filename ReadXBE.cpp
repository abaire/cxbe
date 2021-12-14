// Dumps information about an XBE file in a format similar to readpe.
//
// See https://xboxdevwiki.net/Xbe

#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <memory>
#include <string>

#include "Common.h"
#include "Xbe.h"

static constexpr char kEntryPrefix[] = "    ";
static constexpr char kLabelValueSeparator[] = ":  ";

class Value {
 public:
  friend std::ostream &operator<<(std::ostream &os, const Value &base) { return base.WriteStream(os); }

  void SetPrefixWidth(uint32_t width) { prefix_width_ = width; }

 protected:
  virtual std::ostream &WriteStream(std::ostream &os) const = 0;

 protected:
  uint32_t prefix_width_{0};
};
typedef std::pair<std::string, std::shared_ptr<Value>> NamedValue;

class DecimalValue : public Value {
 public:
  enum Format {
    HEX,
    INT,
    INT_HEX,
    HEX_CHAR,
  };

  explicit DecimalValue(uint32_t value, Format format = HEX) : value_(value), format_(format) {}

 protected:
  std::ostream &WriteStream(std::ostream &os) const override {
    std::ios init(nullptr);
    init.copyfmt(os);

    switch (format_) {
      case HEX_CHAR: {
        char buf[64] = {0};
        snprintf(buf, 63, "0x%08x (%.4s)", value_, reinterpret_cast<char const *>(&value_));
        os << buf;
      } break;

      case INT:
        os << value_;
        break;

      case INT_HEX:
        os << value_ << " (0x" << std::hex << std::setw(8) << std::setfill('0') << value_ << ")";
        break;

      case HEX:
        os << "0x" << std::hex << std::setw(8) << std::setfill('0') << value_;
        break;
    }

    os.copyfmt(init);

    return os;
  }

 private:
  uint32_t value_;
  Format format_;
};

class TimeDateValue : public Value {
 public:
  explicit TimeDateValue(uint32_t value) : value_(value) {}

 protected:
  std::ostream &WriteStream(std::ostream &os) const override {
    char buf[64] = "<INVALID>";
    struct tm *t = gmtime((time_t *)&value_);
    if (t) {
      strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S UTC", t);
    }
    os << buf;

    return os;
  }

 private:
  uint32_t value_;
};

class XORAddressValue : public Value {
 public:
  explicit XORAddressValue(uint32_t value, uint32_t xor_value) : value_(value), xor_value_(xor_value) {}

 protected:
  std::ostream &WriteStream(std::ostream &os) const override {
    std::ios init(nullptr);
    init.copyfmt(os);
    os << "0x" << std::hex << std::setw(8) << std::setfill('0') << (value_ ^ xor_value_) << " (0x" << value_ << ")";
    os.copyfmt(init);
    return os;
  }

 private:
  uint32_t value_;
  uint32_t xor_value_;
};

class InitializationFlagsValue : public Value {
 public:
  explicit InitializationFlagsValue(Xbe::Header::InitFlags value) : value_(value) {}

 protected:
  std::ostream &WriteStream(std::ostream &os) const override {
    std::ios init(nullptr);
    init.copyfmt(os);
    os << "0x" << std::hex << std::setw(8) << std::setfill('0') << *reinterpret_cast<const uint32_t *>(&value_);
    os.copyfmt(init);

    std::string spacer(prefix_width_ + 2, ' ');
    if (value_.bMountUtilityDrive) {
      os << std::endl << spacer << "MOUNT_UTILITY_DRIVE";
    }
    if (value_.bFormatUtilityDrive) {
      os << std::endl << spacer << "FORMAT_UTILITY_DRIVE";
    }
    if (value_.bLimit64MB) {
      os << std::endl << spacer << "LIMIT_64_MEGS_RAM";
    }
    if (value_.bDontSetupHarddisk) {
      os << std::endl << spacer << "DO_NOT_SETUP_HARDDISK";
    }

    return os;
  }

 private:
  Xbe::Header::InitFlags value_;
};

class LibraryVersionValue : public Value {
 public:
  explicit LibraryVersionValue(const Xbe::LibraryVersion *value) : value_(value) {}

 protected:
  std::ostream &WriteStream(std::ostream &os) const override {
    os << value_->wMajorVersion << "." << value_->wMinorVersion << "." << value_->wBuildVersion;

    const auto &flags = value_->dwFlags;

    std::string spacer(prefix_width_, ' ');
    if (flags.QFEVersion) {
      os << std::endl << spacer << "QFE_VERSION: " << flags.QFEVersion;
    }
    if (flags.Approved) {
      os << std::endl << spacer << "APPROVED_STATUS: " << flags.Approved;
    }
    if (flags.bDebugBuild) {
      os << std::endl << spacer << "DEBUG_BUILD";
    }
    return os;
  }

 private:
  const Xbe::LibraryVersion *value_;
};

class SectionFlagsValue : public Value {
 public:
  explicit SectionFlagsValue(Xbe::SectionHeader::_Flags value) : value_(value) {}

 protected:
  std::ostream &WriteStream(std::ostream &os) const override {
    std::ios init(nullptr);
    init.copyfmt(os);
    os << "0x" << std::hex << std::setw(8) << std::setfill('0') << *reinterpret_cast<const uint32_t *>(&value_);
    os.copyfmt(init);

    std::string spacer(prefix_width_ + 2, ' ');
    if (value_.bWritable) {
      os << std::endl << spacer << "WRITE";
    }
    if (value_.bPreload) {
      os << std::endl << spacer << "PRELOAD";
    }
    if (value_.bExecutable) {
      os << std::endl << spacer << "EXECUTE";
    }
    if (value_.bInsertedFile) {
      os << std::endl << spacer << "INSERTED_FILE";
    }
    if (value_.bHeadPageRO) {
      os << std::endl << spacer << "HEAD_PAGE_READ_ONLY";
    }
    if (value_.bTailPageRO) {
      os << std::endl << spacer << "TAIL_PAGE_READ_ONLY";
    }
    return os;
  }

 private:
  Xbe::SectionHeader::_Flags value_;
};

class SectionHeaderValue : public Value {
 public:
  explicit SectionHeaderValue(const Xbe::SectionHeader *value) : value_(value) {}

 protected:
  std::ostream &WriteStream(std::ostream &os) const override {
    os << std::endl;

    std::list<NamedValue> fields;
    fields.emplace_back("Virtual address", std::make_shared<DecimalValue>(value_->dwVirtualAddr));
    fields.emplace_back("Virtual size", std::make_shared<DecimalValue>(value_->dwVirtualSize, DecimalValue::INT_HEX));
    fields.emplace_back("Raw address", std::make_shared<DecimalValue>(value_->dwRawAddr));
    fields.emplace_back("Raw size", std::make_shared<DecimalValue>(value_->dwSizeofRaw, DecimalValue::INT_HEX));
    fields.emplace_back("Section name address", std::make_shared<DecimalValue>(value_->dwSectionNameAddr));
    fields.emplace_back("Section reference count",
                        std::make_shared<DecimalValue>(value_->dwSectionRefCount, DecimalValue::INT));
    fields.emplace_back("Head shared reference count address",
                        std::make_shared<DecimalValue>(value_->dwHeadSharedRefCountAddr));
    fields.emplace_back("Tail shared reference count address",
                        std::make_shared<DecimalValue>(value_->dwTailSharedRefCountAddr));
    fields.emplace_back("Flags", std::make_shared<SectionFlagsValue>(value_->dwFlags));

    int max_length = 0;
    for (auto &entry : fields) {
      if (entry.first.size() > max_length) {
        max_length = static_cast<int>(entry.first.size());
      }
    }

    static constexpr char kInnerEntryPrefix[] = "        ";
    uint32_t indent = sizeof(kInnerEntryPrefix) + max_length + sizeof(kLabelValueSeparator);
    for (auto &entry : fields) {
      entry.second->SetPrefixWidth(indent);
      os << kInnerEntryPrefix << std::setw(max_length) << entry.first << kLabelValueSeparator << *entry.second
         << std::endl;
    }

    return os;
  }

 private:
  const Xbe::SectionHeader *value_;
};

static void PrintInfo(const std::string &header, const std::list<NamedValue> &fields);
static void ExtractXBEHeader(const Xbe::Header &header, std::list<NamedValue> &header_fields);
static void ExtractXBELibraryVersions(Xbe *xbe, std::list<NamedValue> &fields);
static void ExtractTLSDirectory(Xbe *xbe, std::list<NamedValue> &fields);
static void ExtractSectionHeaders(Xbe *xbe, std::list<NamedValue> &fields);

int main(int argc, char *argv[]) {
  char szErrorMessage[ERROR_LEN + 1] = {0};
  char szXbeFilename[OPTION_LEN + 1] = {0};
  bool bRetail;

  const char *program = argv[0];
  const char *program_desc = "XBE information dumper (Version: " VERSION ")";
  Option options[] = {{szXbeFilename, nullptr, "xbefile"}, {nullptr}};

  if (ParseOptions(argv, argc, options, szErrorMessage)) {
    goto cleanup;
  }

  if (szXbeFilename[0] == '\0') {
    ShowUsage(program, program_desc, options);
    return 1;
  }

  {
    Xbe *xbe = new Xbe(szXbeFilename);
    if (xbe->GetError() != 0) {
      strncpy(szErrorMessage, xbe->GetError(), ERROR_LEN);
      goto cleanup;
    }

    {
      std::list<NamedValue> fields;
      ExtractXBEHeader(xbe->m_Header, fields);
      PrintInfo("XBE Header", fields);
    }
    {
      std::list<NamedValue> fields;
      ExtractXBELibraryVersions(xbe, fields);
      if (!fields.empty()) {
        PrintInfo("Library versions", fields);
      }
    }
    {
      std::list<NamedValue> fields;
      ExtractTLSDirectory(xbe, fields);
      if (!fields.empty()) {
        PrintInfo("Thread local storage directory", fields);
      }
    }
    {
      std::list<NamedValue> fields;
      ExtractSectionHeaders(xbe, fields);
      if (!fields.empty()) {
        PrintInfo("Sections", fields);
      }
    }
  }

cleanup:

  if (szErrorMessage[0] != 0) {
    ShowUsage(program, program_desc, options);

    printf("\n");
    printf(" *  Error : %s\n", szErrorMessage);

    return 1;
  }

  return 0;
}

static void PrintInfo(const std::string &header, const std::list<NamedValue> &fields) {
  printf("%s\n", header.c_str());

  int max_length = 0;
  for (auto &entry : fields) {
    if (entry.first.size() > max_length) {
      max_length = static_cast<int>(entry.first.size());
    }
  }

  uint32_t indent = sizeof(kEntryPrefix) + max_length + sizeof(kLabelValueSeparator);
  for (auto &entry : fields) {
    entry.second->SetPrefixWidth(indent);
    std::cout << kEntryPrefix << std::setw(max_length) << entry.first << kLabelValueSeparator << *entry.second
              << std::endl;
  }
}

static void ExtractXBEHeader(const Xbe::Header &header, std::list<NamedValue> &header_fields) {
  uint32_t address_xor = XOR_EP_RETAIL;
  auto entry = header.dwEntryAddr ^ XOR_EP_RETAIL;
  // TODO: Truly validate entry addr.
  if (entry < header.dwBaseAddr || entry & 0xF0000000) {
    address_xor = XOR_EP_DEBUG;
  }

  header_fields.emplace_back("Magic number", std::make_shared<DecimalValue>(header.dwMagic, DecimalValue::HEX_CHAR));

  header_fields.emplace_back("Base address", std::make_shared<DecimalValue>(header.dwBaseAddr));
  header_fields.emplace_back("Size of headers",
                             std::make_shared<DecimalValue>(header.dwSizeofHeaders, DecimalValue::INT_HEX));
  header_fields.emplace_back("Size of image",
                             std::make_shared<DecimalValue>(header.dwSizeofImage, DecimalValue::INT_HEX));
  header_fields.emplace_back("Size of image header",
                             std::make_shared<DecimalValue>(header.dwSizeofImageHeader, DecimalValue::INT_HEX));
  header_fields.emplace_back("Date/time stamp", std::make_shared<TimeDateValue>(header.dwTimeDate));
  header_fields.emplace_back("Certificate address", std::make_shared<DecimalValue>(header.dwCertificateAddr));
  header_fields.emplace_back("Number of sections",
                             std::make_shared<DecimalValue>(header.dwSections, DecimalValue::INT));
  header_fields.emplace_back("Section headers address", std::make_shared<DecimalValue>(header.dwSectionHeadersAddr));
  header_fields.emplace_back("Initialization flags", std::make_shared<InitializationFlagsValue>(header.dwInitFlags));
  header_fields.emplace_back("Entry point", std::make_shared<XORAddressValue>(header.dwEntryAddr, address_xor));
  header_fields.emplace_back("TLS address", std::make_shared<DecimalValue>(header.dwTLSAddr));
  header_fields.emplace_back("Stack size", std::make_shared<DecimalValue>(header.dwPeStackCommit));
  header_fields.emplace_back("PE heap reserve", std::make_shared<DecimalValue>(header.dwPeHeapReserve));
  header_fields.emplace_back("PE heap commit", std::make_shared<DecimalValue>(header.dwPeHeapCommit));
  header_fields.emplace_back("PE base address", std::make_shared<DecimalValue>(header.dwPeBaseAddr));
  header_fields.emplace_back("PE size of image", std::make_shared<DecimalValue>(header.dwPeSizeofImage));
  header_fields.emplace_back("PE checksum", std::make_shared<DecimalValue>(header.dwPeChecksum));
  header_fields.emplace_back("PE date/time stamp", std::make_shared<TimeDateValue>(header.dwPeTimeDate));
  header_fields.emplace_back("Debug path address", std::make_shared<DecimalValue>(header.dwDebugPathnameAddr));
  header_fields.emplace_back("Debug filename address", std::make_shared<DecimalValue>(header.dwDebugFilenameAddr));
  header_fields.emplace_back("Debug UTF-16 filename address",
                             std::make_shared<DecimalValue>(header.dwDebugUnicodeFilenameAddr));
  header_fields.emplace_back("Kernel thunk address",
                             std::make_shared<XORAddressValue>(header.dwKernelImageThunkAddr, address_xor));
  header_fields.emplace_back("Non-kernel import directory address",
                             std::make_shared<DecimalValue>(header.dwNonKernelImportDirAddr));
  header_fields.emplace_back("Number of library versions",
                             std::make_shared<DecimalValue>(header.dwLibraryVersions, DecimalValue::INT));
  header_fields.emplace_back("Library versions address", std::make_shared<DecimalValue>(header.dwLibraryVersionsAddr));
  header_fields.emplace_back("Kernel library version address",
                             std::make_shared<DecimalValue>(header.dwKernelLibraryVersionAddr));
  header_fields.emplace_back("XAPI library version address",
                             std::make_shared<DecimalValue>(header.dwXAPILibraryVersionAddr));
  header_fields.emplace_back("Logo bitmap address", std::make_shared<DecimalValue>(header.dwLogoBitmapAddr));
  header_fields.emplace_back("Logo bitmap size",
                             std::make_shared<DecimalValue>(header.dwSizeofLogoBitmap, DecimalValue::INT));

  if (header.dwSizeofImageHeader > 0x178) {
    //    header_fields.emplace_back("Unknown 1_1", std::make_shared<DecimalValue>(header.dw));
    //    header_fields.emplace_back("Unknown 1_2", std::make_shared<DecimalValue>(header.dw));
  }
  if (header.dwSizeofImageHeader > 0x180) {
    //    header_fields.emplace_back("Unknown 2", std::make_shared<DecimalValue>(header.dw));
  }
}

static void ExtractXBELibraryVersions(Xbe *xbe, std::list<NamedValue> &fields) {
  const Xbe::Header &header = xbe->m_Header;
  if (xbe->m_LibraryVersion) {
    const Xbe::LibraryVersion *info = xbe->m_LibraryVersion;
    for (auto i = 0; i < header.dwLibraryVersions; ++i, ++info) {
      char buf[16] = {0};
      strncpy(buf, info->szName, 8);
      fields.emplace_back(buf, std::make_shared<LibraryVersionValue>(info));
    }
  }

  if (xbe->m_KernelLibraryVersion) {
    fields.emplace_back("Kernel library version", std::make_shared<LibraryVersionValue>(xbe->m_KernelLibraryVersion));
  }

  if (xbe->m_XAPILibraryVersion) {
    fields.emplace_back("XAPI library version", std::make_shared<LibraryVersionValue>(xbe->m_XAPILibraryVersion));
  }
}

static void ExtractTLSDirectory(Xbe *xbe, std::list<NamedValue> &fields) {
  if (!xbe->m_TLS) {
    return;
  }

  const auto &entry = *xbe->m_TLS;
  fields.emplace_back("Data start address", std::make_shared<DecimalValue>(entry.dwDataStartAddr));
  fields.emplace_back("Data end address", std::make_shared<DecimalValue>(entry.dwDataEndAddr));
  fields.emplace_back("Index address", std::make_shared<DecimalValue>(entry.dwTLSIndexAddr));
  fields.emplace_back("Callback table address", std::make_shared<DecimalValue>(entry.dwTLSCallbackAddr));
  fields.emplace_back("Size of zero fill", std::make_shared<DecimalValue>(entry.dwSizeofZeroFill, DecimalValue::INT));
  fields.emplace_back("Alignment", std::make_shared<DecimalValue>(entry.dwCharacteristics));
}

static void ExtractSectionHeaders(Xbe *xbe, std::list<NamedValue> &fields) {
  if (!xbe->m_SectionHeader) {
    return;
  }

  const Xbe::Header &header = xbe->m_Header;
  const auto *entry = xbe->m_SectionHeader;
  const auto *entry_name = xbe->m_szSectionName;
  for (auto i = 0; i < header.dwSections; ++i, ++entry, ++entry_name) {
    char name[16] = {0};
    strncpy(name, *entry_name, 8);
    fields.emplace_back(name, std::make_shared<SectionHeaderValue>(entry));
  }
}
