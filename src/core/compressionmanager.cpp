#include "compressionmanager.h"
#include <chrono>

void CompressionManager::execute() {
	auto startTD = std::chrono::high_resolution_clock::now();
	_quadStack->topDownPhase();
	auto stopTD = std::chrono::high_resolution_clock::now();
	auto durationTD = std::chrono::duration_cast<std::chrono::milliseconds>(stopTD - startTD).count();

	auto startBU = std::chrono::high_resolution_clock::now();
	_quadStack->bottomUpPhase();
	auto stopBU = std::chrono::high_resolution_clock::now();
	auto durationBU = std::chrono::duration_cast<std::chrono::milliseconds>(stopBU - startBU).count();

	auto startRH = std::chrono::high_resolution_clock::now();
	_quadStack->rearrangeHeightField();
	auto stopRH = std::chrono::high_resolution_clock::now();
	auto durationRH = std::chrono::duration_cast<std::chrono::milliseconds>(stopRH - startRH).count();

	auto startCH = std::chrono::high_resolution_clock::now();
	_quadStack->compressHeightField(_sbr->getHeightResolution());
	auto stopCH = std::chrono::high_resolution_clock::now();
	auto durationCH = std::chrono::duration_cast<std::chrono::milliseconds>(stopRH - startRH).count();
	auto durationTotal = std::chrono::duration_cast<std::chrono::milliseconds>(stopRH - startTD).count();


	//std::cout << "Total: " << durationTotal << " ms" << std::endl;
	//std::cout << "Top down: " << float(durationTD) / durationTotal * 100.0 << "%" << std::endl;
	//std::cout << "Bottom up: " << float(durationBU) / durationTotal * 100.0 << "%" << std::endl;
	//std::cout << "Rearrangement: " << float(durationRH) / durationTotal * 100.0 << "%" << std::endl;
	//std::cout << "Compression: " << float(durationCH) / durationTotal * 100.0 << "%" << std::endl;

	//getchar();
}