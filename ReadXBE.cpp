// Dumps information about an XBE file in a format similar to readpe.
//
// See https://xboxdevwiki.net/Xbe

#include <cstring>
#include <iomanip>
#include <iostream>
#include <list>
#include <string>

#include "Common.h"
#include "Xbe.h"

static void PrintInfo(const std::string &header, const std::list<std::pair<std::string, std::string>> &fields);
static void ExtractXBEHeader(const Xbe::Header &header, std::list<std::pair<std::string, std::string>> &header_fields);

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

    std::list<std::pair<std::string, std::string>> header_fields;
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

static void PrintInfo(const std::string &header, const std::list<std::pair<std::string, std::string>> &fields) {
  printf("%s\n", header.c_str());

  int max_length = 0;
  for (auto &entry : fields) {
    if (entry.first.size() > max_length) {
      max_length = static_cast<int>(entry.first.size());
    }
  }

  for (auto &entry : fields) {
    std::cout << kEntryPrefix << std::setw(max_length) << entry.first << kLabelValueSeparator << entry.second
              << std::endl;
  }
}

template <typename T>
static std::string HexString(T value) {
  char buf[16] = {0};
  snprintf(buf, 15, "0x%08x", value);
  return buf;
}

template <typename T>
static std::string IntHexString(T value) {
  char buf[32] = {0};
  snprintf(buf, 31, "%d (0x%x)", value, value);
  return buf;
}

template <typename T>
static std::string IntString(T value) {
  char buf[16] = {0};
  snprintf(buf, 15, "%d", value);
  return buf;
}

static std::string TimeDate(uint32_t value) {
  char buf[16] = {0};
  snprintf(buf, 15, "0x%08x", value);
  return buf;
}

static void ExtractXBEHeader(const Xbe::Header &header, std::list<std::pair<std::string, std::string>> &header_fields) {
  {
    char buf[64] = {0};
    snprintf(buf, 63, "0x%08x (%.4s)", header.dwMagic, reinterpret_cast<char const *>(&header.dwMagic));
    header_fields.emplace_back("Magic number", buf);
  }

  header_fields.emplace_back("Base address", HexString(header.dwBaseAddr));
  header_fields.emplace_back("Size of headers", IntHexString(header.dwSizeofHeaders));
  header_fields.emplace_back("Size of image", IntHexString(header.dwSizeofImage));
  header_fields.emplace_back("Size of image header", IntHexString(header.dwSizeofImageHeader));
  header_fields.emplace_back("Date/time stamp", TimeDate(header.dwTimeDate));
  header_fields.emplace_back("Certificate address", HexString(header.dwCertificateAddr));
  header_fields.emplace_back("Number of sections", IntString(header.dwSections));
  header_fields.emplace_back("Section headers address", HexString(header.dwSectionHeadersAddr));
  //  header_fields.emplace_back("Initialization flags", HexString(header.dwInitFlags));
  //  header_fields.emplace_back("Entry point", HexString(header.dwEntryAddr));
  header_fields.emplace_back("TLS address", HexString(header.dwTLSAddr));
  //  header_fields.emplace_back("Stack size", HexString(header.));
  header_fields.emplace_back("PE heap reserve", HexString(header.dwPeHeapReserve));
  header_fields.emplace_back("PE heap commit", HexString(header.dwPeHeapCommit));
  header_fields.emplace_back("PE base address", HexString(header.dwPeBaseAddr));
  header_fields.emplace_back("PE size of image", HexString(header.dwPeSizeofImage));
  header_fields.emplace_back("PE checksum", HexString(header.dwPeChecksum));
  header_fields.emplace_back("PE date/time stamp", TimeDate(header.dwPeTimeDate));
  header_fields.emplace_back("Debug path address", HexString(header.dwDebugPathnameAddr));
  header_fields.emplace_back("Debug filename address", HexString(header.dwDebugFilenameAddr));
  header_fields.emplace_back("Debug UTF-16 filename address", HexString(header.dwDebugUnicodeFilenameAddr));
  //  header_fields.emplace_back("Kernel thunk address", HexString(header.dwKernelImageThunkAddr));
  header_fields.emplace_back("Non-kernel import directory address", HexString(header.dwNonKernelImportDirAddr));
  header_fields.emplace_back("Number of library versions", HexString(header.dwLibraryVersions));
  header_fields.emplace_back("Library versions address", HexString(header.dwLibraryVersionsAddr));
  header_fields.emplace_back("Kernel library version address", HexString(header.dwKernelLibraryVersionAddr));
  header_fields.emplace_back("XAPI library version address", HexString(header.dwXAPILibraryVersionAddr));
  header_fields.emplace_back("Logo bitmap address", HexString(header.dwLogoBitmapAddr));
  header_fields.emplace_back("Logo bitmap size", IntString(header.dwSizeofLogoBitmap));

  if (header.dwSizeofImageHeader > 0x178) {
    //    header_fields.emplace_back("Unknown 1_1", HexString(header.dw));
    //    header_fields.emplace_back("Unknown 1_2", HexString(header.dw));
  }
  if (header.dwSizeofImageHeader > 0x180) {
    //    header_fields.emplace_back("Unknown 2", HexString(header.dw));
  }
}
