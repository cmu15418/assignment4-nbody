#include <sstream>
#include <iostream>
#include <vector>
#include "timing.h"
#include "common.h"
#include "quad-tree.h"

void simulateStep(const QuadTree& quadTree,
                  const std::vector<Particle>& particles,
                  std::vector<Particle>& newParticles,
                  StepParameters params) {
  for (int i = 0; i < (int) particles.size(); ++i) {
    const auto& pi = particles[i];
    std::vector<Particle> nearbyParticles;
    quadTree.getParticles(nearbyParticles, pi.position, params.cullRadius);
    Vec2 force = Vec2(0.0f, 0.0f);
    for (size_t j = 0; j < nearbyParticles.size(); ++j) {
      if (nearbyParticles[j].id == pi.id)
        continue;
      force += computeForce(pi, nearbyParticles[j], params.cullRadius);
    }
    newParticles[i] = updateParticle(pi, force, params.deltaTime);
  }
}

int main(int argc, char *argv[]) {
  StartupOptions options = parseOptions(argc, argv);

  std::vector<Particle> particles, newParticles;

  if (options.inputFile.empty()) {
    std::cerr << "Please specify input file with -in option\n";
    exit(1);
  }

  loadFromFile(options.inputFile, particles);

  StepParameters stepParams;
  stepParams = getBenchmarkStepParams(options.spaceSize);

  double totalTreeBuildingTime = 0;
  double totalSimulationTime = 0;
  newParticles.resize(particles.size());
  for (int i = 0; i < options.numIterations; i++) {
    QuadTree tree;
    Timer t;
    buildQuadTree(particles, tree);
    double treeBuildingTime = t.elapsed();

    t.reset();
    simulateStep(tree, particles, newParticles, stepParams);
    double simulateStepTime = t.elapsed();
    particles.swap(newParticles);

    totalTreeBuildingTime += treeBuildingTime;
    totalSimulationTime += simulateStepTime;

    printf("iteration %d, tree construction: %.6fms, simulation: %.6fms\n",
           i, treeBuildingTime, simulateStepTime);

    // generate simulation image
    if (options.frameOutputStyle == FrameOutputStyle::AllFrames) {
      std::stringstream sstream;
      sstream << options.bitmapOutputDir;
      if (!options.bitmapOutputDir.size() || (options.bitmapOutputDir.back() != '\\' &&
                                              options.bitmapOutputDir.back() != '/'))
        sstream << "/";
      sstream << i << ".bmp";
      dumpView(sstream.str(), options.viewportRadius, particles);
    }
  }

  printf("TOTAL TIME: %.6fms\ntotal tree construction time: %.6fms\ntotal simulation time: %.6fms\n",
         totalTreeBuildingTime + totalSimulationTime,
         totalTreeBuildingTime,
         totalSimulationTime);

  saveToFile(options.outputFile, particles);
}
