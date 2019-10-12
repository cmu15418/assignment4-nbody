#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <iomanip>
#include "common.h"
#include "quad-tree.h"

std::string removeQuote(std::string input)
{
  if (input.length() > 0 && input.front() == '\"')
    return input.substr(1, input.length() - 2);
  return input;
}

StepParameters getBenchmarkStepParams(float spaceSize)
{
  StepParameters result;
  result.cullRadius = spaceSize / 4.0f;
  result.deltaTime = 0.2f;
  return result;
}


StartupOptions parseOptions(int argc, char *argv[])
{
    StartupOptions rs;
    for (int i = 1; i < argc; i++)
    {
        if (i < argc - 1)
        {
            if (strcmp(argv[i], "-i") == 0)
                rs.numIterations = atoi(argv[i + 1]);
            else if (strcmp(argv[i], "-s") == 0)
                rs.spaceSize = (float)atof(argv[i + 1]);
            else if (strcmp(argv[i], "-in") == 0)
                rs.inputFile = removeQuote(argv[i + 1]);
            else if (strcmp(argv[i], "-n") == 0)
                rs.numParticles = atoi(argv[i + 1]);
            else if (strcmp(argv[i], "-v") == 0)
                rs.viewportRadius = (float)atof(argv[i + 1]);
            else if (strcmp(argv[i], "-o") == 0)
                rs.outputFile = argv[i + 1];
            else if (strcmp(argv[i], "-fo") == 0)
            {
                rs.bitmapOutputDir = removeQuote(argv[i + 1]);
                rs.frameOutputStyle = FrameOutputStyle::AllFrames;
            }
            else if (strcmp(argv[i], "-ref") == 0)
                rs.referenceAnswerDir = removeQuote(argv[i + 1]);
        }
        if (strcmp(argv[i], "-mpi") == 0)
        {
            rs.simulatorType = SimulatorType::MPI;
        }
        else if (strcmp(argv[i], "-mpilb") == 0)
        {
            rs.simulatorType = SimulatorType::MPILB;
        }
    }
    return rs;
}

void Image::setSize(int w, int h)
{
  width = w;
  height = h;
  pixels.resize(w * h);
}

void Image::clear()
{
  for (auto & p : pixels)
    {
      p.r = 0;
      p.g = 0;
      p.b = 0;
      p.a = 255;
    }
}

void Image::drawRectangle(Vec2 bmin, Vec2 bmax)
{
  int minX = clamp(bmin.x, 0, width - 1);
  int minY = clamp(bmin.y, 0, height - 1);
  int maxX = clamp(bmax.x, 0, width - 1);
  int maxY = clamp(bmax.y, 0, height - 1);
  Pixel highlight = { 255, 0, 0, 255 };
  for (int x = minX; x <= maxX; x++){
    pixels[minY * width + x] = highlight;
    pixels[maxY * width + x] = highlight;
  }

  for (int y = minY; y < maxY; y++){
    pixels[y * width + minX] = highlight;
    pixels[y * width + maxX] = highlight;
  }
}

void Image::fillRectangle(int x, int y, int size)
{
  int minX = clamp(x - size, 0, width - 1);
  int minY = clamp(y - size, 0, height - 1);
  int maxX = clamp(x + size, 0, width - 1);
  int maxY = clamp(y + size, 0, height - 1);
  Pixel highlight = { 255, 255, 255, 255 };
  for (int y = minY; y <= maxY; y++)
    for (int x = minX; x <= maxX; x++)
      {
        pixels[y*width + x] = highlight;
      }
}

void Image::saveToFile(std::string fileName)
{
  int filesize = 54 + 3 * width * height;
  std::vector<unsigned char> img;
  img.resize(3 * width * height);
  for (int j = 0; j < height; j++)
    {
      Pixel * scanLine = &pixels[0] + j * width;
      for (int i = 0; i < width; i++)
        {
          int x = i;
          int y = height - 1 - j;

          int r = scanLine[i].r;
          int g = scanLine[i].g;
          int b = scanLine[i].b;
          img[(x + y * width) * 3 + 2] = (unsigned char)(r);
          img[(x + y * width) * 3 + 1] = (unsigned char)(g);
          img[(x + y * width) * 3 + 0] = (unsigned char)(b);
        }
    }

  unsigned char bmpfileheader[14] = { 'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0 };
  unsigned char bmpinfoheader[40] = { 40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0 };
  unsigned char bmppad[3] = { 0,0,0 };

  bmpfileheader[2] = (unsigned char)(filesize);
  bmpfileheader[3] = (unsigned char)(filesize >> 8);
  bmpfileheader[4] = (unsigned char)(filesize >> 16);
  bmpfileheader[5] = (unsigned char)(filesize >> 24);

  bmpinfoheader[4] = (unsigned char)(width);
  bmpinfoheader[5] = (unsigned char)(width >> 8);
  bmpinfoheader[6] = (unsigned char)(width >> 16);
  bmpinfoheader[7] = (unsigned char)(width >> 24);
  bmpinfoheader[8] = (unsigned char)(height);
  bmpinfoheader[9] = (unsigned char)(height >> 8);
  bmpinfoheader[10] = (unsigned char)(height >> 16);
  bmpinfoheader[11] = (unsigned char)(height >> 24);

  std::ofstream file;
  file.open(fileName, std::ios::out | std::ios::binary);
  if (!file)
    {
      std::cout << "error writing file \"" << fileName << "\"" << std::endl;
      return;
    }
  file.write((const char*)bmpfileheader, 14);
  file.write((const char*)bmpinfoheader, 40);
  for (int i = 0; i < height; i++)
    {
      file.write((const char*)(&img[0] + (width * i * 3)), 3 * width);
      file.write((const char*)bmppad, (4 - (width * 3) % 4) % 4);
    }
  file.close();
}

bool loadFromFile(std::string fileName, std::vector<Particle>& particles)
{
  std::ifstream inFile;
  inFile.open(fileName);
  if (!inFile)
    return false;

  std::string line;
  while (std::getline(inFile, line)) {
    Particle particle;
    std::stringstream sstream(line);
    std::string str;
    std::getline(sstream, str, ' ');
    particle.mass = (float)atof(str.c_str());
    std::getline(sstream, str, ' ');
    particle.position.x = (float)atof(str.c_str());
    std::getline(sstream, str, ' ');
    particle.position.y = (float)atof(str.c_str());
    std::getline(sstream, str, ' ');
    particle.velocity.x = (float)atof(str.c_str());
    std::getline(sstream, str, '\n');
    particle.velocity.y = (float)atof(str.c_str());
    particle.id = (int)particles.size();
    particles.push_back(particle);
  }
  inFile.close();
  return true;
}

void saveToFile(std::string fileName, const std::vector<Particle>& particles) {
  std::ofstream file(fileName);
  if (!file) {
    std::cerr << "error writing file \"" << fileName << "\"" << std::endl;
    return;
  }
  file << std::setprecision(9);
  for (const auto& p : particles) {
    file << p.mass << " " << p.position.x << " " << p.position.y << " "
         << p.velocity.x << " " << p.velocity.y << std::endl;
  }
  file.close();
  if (!file)
    std::cerr << "error writing file \"" << fileName << "\"" << std::endl;
}

void dumpView(std::string fileName, float viewportRadius,
              const std::vector<Particle>& particles)
{
    Image image;
    const int imageSize = 512;
    image.setSize(imageSize, imageSize);
    image.clear();
    float invViewportSize = 0.5f / viewportRadius;
    for (auto & p : particles)
    {
        int x = (int)((p.position.x + viewportRadius) * invViewportSize * imageSize);
        int y = (int)((p.position.y + viewportRadius) * invViewportSize * imageSize);
        image.fillRectangle(x, y, 1);
    }

    // overlay visualization on to particle views
    QuadTree tree;
    buildQuadTree(particles, tree);
    tree.showStructure(image, viewportRadius);
    image.saveToFile(fileName);
}
