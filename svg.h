#include <fstream>
#include <string>
#include <vector>

namespace emboarden {
  class svg {
  public:
    svg(std::string filename, int width, int height);
    ~svg();

    void write_polygon(std::vector<int> &pts);

  private:   
    std::ofstream out;
    int width, height;
  };
}
