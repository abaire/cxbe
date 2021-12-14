// ******************************************************************
// *
// *  This file is part of Cxbe
// *
// *  This program is free software; you can redistribute it and/or
// *  modify it under the terms of the GNU General Public License
// *  as published by the Free Software Foundation; either version 2
// *  of the License, or (at your option) any later version.
// *
// *  This program is distributed in the hope that it will be useful,
// *  but WITHOUT ANY WARRANTY; without even the implied warranty of
// *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// *  GNU General Public License for more details.
// *
// *  You should have received a copy of the GNU General Public License
// *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
// *
// *  (c) 2002-2003 Aaron Robinson <caustik@caustik.com>
// *
// *  All rights reserved
// *
// ******************************************************************
#include <string.h>

#include "Common.h"
#include "Exe.h"
#include "Xbe.h"

static bool ConvertXbe(Xbe *xbe, Exe *exe);

// program entry point
int main(int argc, char *argv[]) {
  char szErrorMessage[ERROR_LEN + 1] = {0};
  char szExeFilename[OPTION_LEN + 1] = {0};
  char szXbeFilename[OPTION_LEN + 1] = {0};
  char szDumpFilename[OPTION_LEN + 1] = {0};
  char szXbeTitle[OPTION_LEN + 1] = "Untitled";
  char szMode[OPTION_LEN + 1] = "retail";
  bool bRetail;

  const char *program = argv[0];
  const char *program_desc = "CEXE XBE to EXE (Xbox to win32) Relinker (Version: " VERSION ")";
  Option options[] = {{szXbeFilename, NULL, "xbefile"},
                      {szExeFilename, "OUT", "filename"},
                      {szDumpFilename, "DUMPINFO", "filename"},
                      {szMode, "MODE", "{debug|retail}"},
                      {NULL}};

  if (ParseOptions(argv, argc, options, szErrorMessage)) {
    goto cleanup;
  }

  if (CompareString(szMode, "RETAIL"))
    bRetail = true;
  else if (CompareString(szMode, "DEBUG"))
    bRetail = false;
  else {
    strncpy(szErrorMessage, "invalid MODE", ERROR_LEN);
    goto cleanup;
  }

  // verify we received the required parameters
  if (szXbeFilename[0] == '\0') {
    ShowUsage(program, program_desc, options);
    return 1;
  }

  // if we don't have an Exe filename, generate one from szXbeFilename
  if (szExeFilename[0] == '\0') {
    if (GenerateFilename(szExeFilename, ".exe", szXbeFilename, ".xbe")) {
      strncpy(szErrorMessage, "Unable to generate Xbe Path", ERROR_LEN);
      goto cleanup;
    }
  }

  // open and convert Exe file
  {
    Xbe *XbeFile = new Xbe(szXbeFilename);
    if (XbeFile->GetError() != 0) {
      strncpy(szErrorMessage, XbeFile->GetError(), ERROR_LEN);
      goto cleanup;
    }

    Exe *ExeFile = new Exe();
    if (ExeFile->GetError() != 0) {
      strncpy(szErrorMessage, ExeFile->GetError(), ERROR_LEN);
      goto cleanup;
    }

    if (!ConvertXbe(XbeFile, ExeFile)) {
      goto cleanup;
    }

    if (szDumpFilename[0] != 0) {
      FILE *outfile = fopen(szDumpFilename, "wt");
      XbeFile->DumpInformation(outfile);
      fclose(outfile);

      if (XbeFile->GetError() != 0) {
        if (XbeFile->IsFatal()) {
          strncpy(szErrorMessage, XbeFile->GetError(), ERROR_LEN);
          goto cleanup;
        } else {
          printf("DUMPINFO -> Warning: %s\n", XbeFile->GetError());
          XbeFile->ClearError();
        }
      }
    }

    ExeFile->Export(szExeFilename);
    if (ExeFile->GetError() != 0) {
      strncpy(szErrorMessage, ExeFile->GetError(), ERROR_LEN);
      goto cleanup;
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

static bool ConvertXbe(Xbe *xbe, Exe *exe) {
  exe->m_bzDOSStub = new uint08[sizeof(bzDOSStub)];
  memcpy(exe->m_bzDOSStub, bzDOSStub, sizeof(bzDOSStub));

  auto &dos_header = exe->m_DOSHeader;
  memcpy(&dos_header, exe->m_bzDOSStub, sizeof(dos_header));

  auto &optional_header = exe->m_OptionalHeader;
  optional_header.m_magic = 0x010B;  // PE32
  optional_header.m_linker_version_major = 14;
  optional_header.m_linker_version_minor = 0;
  optional_header.m_subsystem_version_major = 1;
  optional_header.m_subsystem_version_minor = 0;
  optional_header.m_linker_version_major = 7;
  optional_header.m_linker_version_minor = 10;
  optional_header.m_os_version_major = 5;
  optional_header.m_os_version_minor = 0;
  optional_header.m_image_version_major = 5;
  optional_header.m_image_version_minor = 0;
  optional_header.m_dll_characteristics = 0x00;  // TODO

  uint32_t address_xor = XOR_EP_RETAIL;
  optional_header.m_entry = xbe->m_Header.dwEntryAddr ^ XOR_EP_RETAIL;
  // TODO: Truly validate entry addr.
  if (optional_header.m_entry < xbe->m_Header.dwPeBaseAddr || optional_header.m_entry & 0xF0000000) {
    optional_header.m_entry = xbe->m_Header.dwEntryAddr ^ XOR_EP_DEBUG;
    address_xor = XOR_EP_DEBUG;
  }
  optional_header.m_entry -= xbe->m_Header.dwPeBaseAddr;

  optional_header.m_sizeof_stack_commit = xbe->m_Header.dwPeStackCommit;
  optional_header.m_sizeof_heap_reserve = xbe->m_Header.dwPeHeapReserve;
  optional_header.m_sizeof_heap_commit = xbe->m_Header.dwPeHeapCommit;
  optional_header.m_sizeof_image = xbe->m_Header.dwPeSizeofImage;
  optional_header.m_checksum = xbe->m_Header.dwPeChecksum;

  optional_header.m_image_base = xbe->m_Header.dwBaseAddr;
  optional_header.m_section_alignment = 0x1000;
  optional_header.m_file_alignment = 0x200;

  optional_header.m_data_directories = 16;
  optional_header.m_sizeof_headers = 0x400;  // TODO: Base on number of data directories.

  optional_header.m_subsystem = IMAGE_SUBSYSTEM_XBOX;

  auto &header = exe->m_Header;
  header.m_magic = *(uint32 *)"PE\0\0";
  header.m_machine = IMAGE_FILE_MACHINE_I386;
  header.m_sections = xbe->m_Header.dwSections;
  header.m_symbol_table_addr = 0;  // TODO
  header.m_symbols = 0;            // TODO
  header.m_sizeof_optional_header = sizeof(optional_header);
  header.m_timedate = xbe->m_Header.dwPeTimeDate;

  // IMAGE_FILE_RELOCS_STRIPPED
  // IMAGE_FILE_EXECUTABLE_IMAGE
  // IMAGE_FILE_32BIT_MACHINE
  header.m_characteristics = 0x103;

  exe->m_SectionHeader = new Exe::SectionHeader[xbe->m_Header.dwSections];
  exe->m_bzSection = new uint08 *[xbe->m_Header.dwSections];

  auto raw_offset = optional_header.m_sizeof_headers;
  for (auto i = 0; i < xbe->m_Header.dwSections; ++i) {
    auto section = xbe->m_bzSection[i];
    auto &section_header = xbe->m_SectionHeader[i];

    auto section_size = section_header.dwSizeofRaw;
    exe->m_bzSection[i] = new uint08[section_size];
    memcpy(exe->m_bzSection[i], section, section_size);

    exe->m_SectionHeader[i].m_virtual_size = section_header.dwVirtualSize;
    exe->m_SectionHeader[i].m_virtual_addr = section_header.dwVirtualAddr - optional_header.m_image_base;
    exe->m_SectionHeader[i].m_raw_addr = raw_offset;
    if (section_size % optional_header.m_file_alignment) {
      exe->m_SectionHeader[i].m_sizeof_raw =
          (section_size / optional_header.m_file_alignment + 1) * optional_header.m_file_alignment;
    } else {
      exe->m_SectionHeader[i].m_sizeof_raw = section_size;
    }
    raw_offset += exe->m_SectionHeader[i].m_sizeof_raw;

    memcpy(exe->m_SectionHeader[i].m_name, xbe->m_szSectionName[i], sizeof(exe->m_SectionHeader[i].m_name));
    if (!memcmp(exe->m_SectionHeader[i].m_name, ".text\0\0\0", 8)) {
      optional_header.m_sizeof_code = section_size;
      optional_header.m_code_base = exe->m_SectionHeader[i].m_virtual_addr;

    } else if (!memcmp(exe->m_SectionHeader[i].m_name, ".data\0\0\0", 8)) {
      // exe->m_OptionalHeader.m_sizeof_initialized_data = section_size;  // TODO: This is probably a summation.
    } else if (!memcmp(exe->m_SectionHeader[i].m_name, ".tls\0\0\0\0", 8)) {
      optional_header.m_image_data_directory[IMAGE_DIRECTORY_ENTRY_TLS].m_virtual_addr =
          exe->m_SectionHeader[i].m_virtual_addr;
      optional_header.m_image_data_directory[IMAGE_DIRECTORY_ENTRY_TLS].m_size = exe->m_SectionHeader[i].m_virtual_size;
    }
  }

  return true;
}
