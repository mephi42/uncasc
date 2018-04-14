#include <CascLib.h>
#include <cstdlib>
#include <cstring>
#include <errno.h>
#include <fstream>
#include <iostream>
#include <sys/stat.h>

template <typename T> struct Guard {
  ~Guard() { t(); }
  const T t;
};

template <typename T> Guard<T> guard(T t) { return Guard<T>{std::move(t)}; }

int main(int argc, char **argv) {
  bool extract = true;
  {
    int j = 1;
    for (int i = 1; i < argc; ++i) {
      if (strcmp(argv[i], "-l") == 0) {
        extract = false;
      } else {
        argv[j] = argv[i];
        ++j;
      }
    }
    argc = j;
  }
  if (argc != 2 && argc != 3) {
    std::cerr << argv[0] << ": unpack CASC into the current directory"
              << std::endl;
    std::cerr << "Usage: " << argv[0] << " path [mask]" << std::endl;
    return EXIT_FAILURE;
  }
  const char *mask = argc == 3 ? argv[2] : "*";

  HANDLE casc;
  if (!CascOpenStorage(argv[1], 0, &casc)) {
    std::cerr << "CascOpenStorage(" << argv[1] << ") failed with error "
              << GetLastError() << std::endl;
    return EXIT_FAILURE;
  }
  auto casc_guard = guard([casc]() { CascCloseStorage(casc); });

  CASC_FIND_DATA find_data;
  HANDLE find = CascFindFirstFile(casc, mask, &find_data, NULL);
  if (find != NULL) {
    auto find_guard = guard([find]() { CascFindClose(find); });
    for (bool proceed = true; proceed;
         proceed = CascFindNextFile(find, &find_data)) {
      std::cout << find_data.szFileName << std::endl;
      if (!extract) {
        continue;
      }
      for (char *sep = strchr(find_data.szFileName, '/'); sep != NULL;
           sep = strchr(sep + 1, '/')) {
        *sep = 0;
        if (mkdir(find_data.szFileName,
                  S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)) {
          if (errno != EEXIST) {
            std::cerr << "Could not create " << find_data.szFileName
                      << std::endl;
            return EXIT_FAILURE;
          }
        }
        *sep = '/';
      }
      std::ofstream f(find_data.szFileName,
                      std::ios_base::out | std::ios_base::binary);
      if (!f) {
        std::cerr << "Could not open " << find_data.szFileName << std::endl;
        return EXIT_FAILURE;
      }
      HANDLE file;
      if (!CascOpenFile(casc, find_data.szFileName, 0, 0, &file)) {
        std::cerr << "CascOpenFile(" << find_data.szFileName
                  << ") failed with error " << GetLastError() << std::endl;
        return EXIT_FAILURE;
      }
      auto file_guard = guard([file]() { CascCloseFile(file); });
      while (true) {
        char buf[8192];
        DWORD n;
        if (!CascReadFile(file, buf, sizeof(buf), &n)) {
          auto error = GetLastError();
          if (error == ERROR_FILE_ENCRYPTED) {
            std::cerr << "Skipping encrypted " << find_data.szFileName
                      << std::endl;
            break;
          } else {
            std::cerr << "CascReadFile(" << find_data.szFileName
                      << ") failed with error " << GetLastError() << std::endl;
            return EXIT_FAILURE;
          }
        }
        if (n == 0) {
          break;
        }
        f.write(buf, n);
        if (!f) {
          std::cerr << "Could not write to " << find_data.szFileName
                    << std::endl;
          return EXIT_FAILURE;
        }
      }
    }
  }
}
