#include <cstring>

#include "Common.h"
#include "Xbe.h"

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
    Xbe *XbeFile = new Xbe(szXbeFilename);
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