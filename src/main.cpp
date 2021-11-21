#include "app.h"

using namespace restinio;

int main(int argc, char *argv[]) {
  App app;
  return app.exec(argc, argv);
}
