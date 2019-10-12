#ifndef COMMON_H_
#define COMMON_H_

#include <cstring>
#include <string>
#include <vector>
#include <cmath>

enum class FrameOutputStyle
{
    None, FinalFrameOnly, AllFrames
};

enum class SimulatorType
{
    MPI, MPILB
};

struct StartupOptions
{
    int numIterations = 1;
    int numParticles = 5;
    float viewportRadius = 10.0f;
    float spaceSize = 10.0f;
    FrameOutputStyle frameOutputStyle = FrameOutputStyle::FinalFrameOnly;
    std::string outputFile = "out.txt";
    std::string bitmapOutputDir;
    std::string inputFile;
    SimulatorType simulatorType = SimulatorType::MPI;
    bool checkCorrectness = false;
    std::string referenceAnswerDir = "";
};

std::string removeQuote(std::string input);

struct StepParameters
{
    float deltaTime = 0.2f;
    float cullRadius = 1.0f;
};

StepParameters getBenchmarkStepParams(float spaceSize);

StartupOptions parseOptions(int argc, char *argv[]);


class Vec2
{
public:
    float x, y;
    Vec2() = default;
    Vec2(float vx, float vy)
    {
        x = vx;
        y = vy;
    }
    static inline float dot(const Vec2 & v0, const Vec2 & v1)
    {
        return v0.x * v1.x + v0.y * v1.y;
    }
    inline float & operator [] (int i)
    {
        return ((float*)this)[i];
    }
    inline Vec2 operator * (float s) const
    {
        Vec2 rs;
        rs.x = x * s;
        rs.y = y * s;
        return rs;
    }
    inline Vec2 operator * (const Vec2 &vin) const
    {
        Vec2 rs;
        rs.x = x * vin.x;
        rs.y = y * vin.y;
        return rs;
    }
    inline Vec2 operator + (const Vec2 &vin) const
    {
        Vec2 rs;
        rs.x = x + vin.x;
        rs.y = y + vin.y;
        return rs;
    }
    inline Vec2 operator - (const Vec2 &vin) const
    {
        Vec2 rs;
        rs.x = x - vin.x;
        rs.y = y - vin.y;
        return rs;
    }
    inline Vec2 operator -() const
    {
        Vec2 rs;
        rs.x = -x;
        rs.y = -y;
        return rs;
    }
    inline Vec2 & operator += (const Vec2 & vin)
    {
        x += vin.x;
        y += vin.y;
        return *this;
    }
    inline Vec2 & operator -= (const Vec2 & vin)
    {
        x -= vin.x;
        y -= vin.y;
        return *this;
    }
    Vec2 & operator = (float v)
    {
        x = y = v;
        return *this;
    }
    inline Vec2 & operator *= (float s)
    {
        x *= s;
        y *= s;
        return *this;
    }
    inline Vec2 & operator *= (const Vec2 & vin)
    {
        x *= vin.x;
        y *= vin.y;
        return *this;
    }
    inline Vec2 normalize()
    {
        float len = sqrt(x*x + y*y);
        float invLen = 1.0f / len;
        Vec2 rs;
        rs.x = x * invLen;
        rs.y = y * invLen;
        return rs;
    }
    inline float length()
    {
        return sqrt(x*x + y*y);
    }
};

class Particle
{
public:
    int id;
    float mass;
    Vec2 position;
    Vec2 velocity;
};

class Pixel
{
public:
    unsigned char r, g, b, a;
};

class Image
{
public:
    int width = 0, height = 0;
    std::vector<Pixel> pixels;
    void setSize(int w, int h);
    void clear();
    void drawRectangle(Vec2 bmin, Vec2 bmax);
    void fillRectangle(int x, int y, int size);
    void saveToFile(std::string fileName);
};

// TODO: possibility to vectorize this, or something similar?
inline int clamp(int val, int lbound, int ubound)
{
    return val < lbound ? lbound : val > ubound ? ubound : val;
}

bool loadFromFile(std::string fileName, std::vector<Particle>& particles);
void saveToFile(std::string fileName, const std::vector<Particle>& particles);
void dumpView(std::string fileName, float viewportRadius, const std::vector<Particle>& particles);

inline Particle updateParticle(const Particle& pi, Vec2 force, float deltaTime)
{
  Particle result = pi;
  result.velocity += force * (deltaTime / pi.mass);
  result.position += result.velocity * deltaTime;
  return result;
}

// Do not modify this function.
inline Vec2 computeForce(const Particle & target, const Particle & attractor, float cullRadius)
{
  auto dir = (attractor.position - target.position);
  auto dist = dir.length();
  if (dist < 1e-3f)
    return Vec2(0.0f, 0.0f);
  dir *= (1.0f / dist);
  if (dist > cullRadius)
    return Vec2(0.0f, 0.0f);
  if (dist < 1e-1f)
    dist = 1e-1f;
  const float G = 0.01f;
  Vec2 force = dir * target.mass * attractor.mass * (G / (dist * dist));
  if (dist > cullRadius * 0.75f) {
    float decay = 1.0f - (dist - cullRadius * 0.75f) / (cullRadius * 0.25f);
    force *= decay;
  }
  return force;
}

#endif
