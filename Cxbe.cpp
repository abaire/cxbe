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
  const char *program_desc = "CXBE EXE to XBE (win32 to Xbox) Relinker (Version: " VERSION ")";
  Option options[] = {{szExeFilename, NULL, "exefile"},         {szXbeFilename, "OUT", "filename"},
                      {szDumpFilename, "DUMPINFO", "filename"}, {szXbeTitle, "TITLE", "title"},
                      {szMode, "MODE", "{debug|retail}"},       {NULL}};

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

  if (strlen(szXbeTitle) > 40) {
    printf("WARNING: Title too long, trimming\n");
    szXbeTitle[40] = '\0';
  }

  // verify we received the required parameters
  if (szExeFilename[0] == '\0') {
    ShowUsage(program, program_desc, options);
    return 1;
  }

  // if we don't have an Xbe filename, generate one from szExeFilename
  if (szXbeFilename[0] == '\0') {
    if (GenerateFilename(szXbeFilename, ".xbe", szExeFilename, ".exe")) {
      strncpy(szErrorMessage, "Unable to generate Exe Path", ERROR_LEN);
      goto cleanup;
    }
  }

  // open and convert Exe file
  {
    Exe *ExeFile = new Exe(szExeFilename);

    if (ExeFile->GetError() != 0) {
      strncpy(szErrorMessage, ExeFile->GetError(), ERROR_LEN);
      goto cleanup;
    }

    Xbe *XbeFile = new Xbe(ExeFile, szXbeTitle, bRetail);

    if (XbeFile->GetError() != 0) {
      strncpy(szErrorMessage, XbeFile->GetError(), ERROR_LEN);
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

    XbeFile->Export(szXbeFilename);

    if (XbeFile->GetError() != 0) {
      strncpy(szErrorMessage, XbeFile->GetError(), ERROR_LEN);
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
