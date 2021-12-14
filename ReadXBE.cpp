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

class Value {
 public:
  friend std::ostream &operator<<(std::ostream &os, const Value &base) { return base.WriteStream(os); }

  void SetPrefixWidth(uint32_t width) { prefix_width_ = width; }

 protected:
  virtual std::ostream &WriteStream(std::ostream &os) const = 0;

 protected:
  uint32_t prefix_width_{0};
};

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

typedef std::pair<std::string, std::shared_ptr<Value>> NamedValue;
static void PrintInfo(const std::string &header, const std::list<NamedValue> &fields);
static void ExtractXBEHeader(const Xbe::Header &header, std::list<NamedValue> &header_fields);

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

    std::list<NamedValue> header_fields;
    ExtractXBEHeader(xbe->m_Header, header_fields);

    PrintInfo("XBE Header", header_fields);
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

static constexpr char kEntryPrefix[] = "    ";
static constexpr char kLabelValueSeparator[] = ":  ";

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
  //  header_fields.emplace_back("Entry point", std::make_shared<DecimalValue>(header.dwEntryAddr));
  header_fields.emplace_back("TLS address", std::make_shared<DecimalValue>(header.dwTLSAddr));
  //  header_fields.emplace_back("Stack size", std::make_shared<DecimalValue>(header.));
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
  //  header_fields.emplace_back("Kernel thunk address", std::make_shared<DecimalValue>(header.dwKernelImageThunkAddr));
  header_fields.emplace_back("Non-kernel import directory address",
                             std::make_shared<DecimalValue>(header.dwNonKernelImportDirAddr));
  header_fields.emplace_back("Number of library versions", std::make_shared<DecimalValue>(header.dwLibraryVersions));
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
