//
// Created by s.vezzuto on 6/14/23.
//

#ifndef S_VEZZUTO_PARALLEL_CALCULATOR_H
#define S_VEZZUTO_PARALLEL_CALCULATOR_H

#include "calculator.h"

using namespace std;


class ParallelCalculator: public Calculator {
private:
    mutex m;
    int realWorkers = 1, chromosomeDimension;
    std::vector<future<void>> workers;
    std::vector<pair<int, int>> chunks;
    std::vector<Chromosome> pathsWithLength;

    void initializeWorkers(int desiredWorkers) {
        realWorkers = desiredWorkers;
        if (realWorkers > chromosomeDimension) {
            realWorkers = chromosomeDimension;
        }

        workers.resize(realWorkers);
        chunks.resize(realWorkers);
    }

    void calculateChunksDimensionForInput(const int inputSize) {

        int workerCellsToCompute = std::floor(inputSize / realWorkers);
        int remainedElements = inputSize % realWorkers;
        int index = 0;

        //! setup chunk for workers
        std::for_each(chunks.begin(), chunks.end(), [&](std::pair<int, int> &chunk) {
            chunk.first = index;
            if (remainedElements) {
                chunk.second = chunk.first + workerCellsToCompute;
                remainedElements--;
            } else {
                chunk.second = chunk.first + workerCellsToCompute - 1;
            }
            index = chunk.second + 1;
        });
    }

public:
    void generateFirstChromosomes() {
        auto start = std::chrono::system_clock::now();
        initializeWorkers(realWorkers);
        calculateChunksDimensionForInput(chromosomeDimension);
        pathsWithLength.clear();
        pathsWithLength.resize(chromosomeDimension);
        pathsWithLength.reserve(chromosomeDimension * 3);

        auto chunksIt = chunks.begin();

        for_each(workers.begin(), workers.end(), [&](future<void> &worker) {
            worker = async(launch::async, [&, start = chunksIt->first, end = chunksIt->second] {
                for (int chromosomeIndex = start; chromosomeIndex <= end; chromosomeIndex++) {
                    pathsWithLength[chromosomeIndex] =  generateRandomPath();
                }
            });
            chunksIt++;
        });
        std::for_each(workers.begin(), workers.end(), [&](std::future<void> &worker) { worker.get(); });
        auto end = std::chrono::system_clock::now();
        timeToGenerateRandomPath += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    void evaluateAndSortChromosomes() {// Complexity O( chromosomeDimension * log²(chromosomeDimension))
        // Complexity without memoization O(chromosomeDimension * numberOfCities * log(chromosomeDimension))
        auto startEvaluation = std::chrono::system_clock::now();
        calculateChunksDimensionForInput(pathsWithLength.size());
        auto chunksIt = chunks.begin();


        for_each(workers.begin(), workers.end(), [&](future<void> &worker) {
            worker = async(launch::async, [&, start = chunksIt->first, end = chunksIt->second] {
                for (int chromosomeIndex = start; chromosomeIndex <= end; chromosomeIndex++) {
                    auto app = calculateFitnessForPath(pathsWithLength[chromosomeIndex].first);
                    pathsWithLength[chromosomeIndex].second = app;
                }
            });
            chunksIt++;
        });
        for_each(workers.begin(), workers.end(), [&](std::future<void> &worker) { worker.get(); });
        timeToEvaluate +=  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - startEvaluation).count();

        auto startSort = std::chrono::system_clock::now();
        sort(pathsWithLength.begin(), pathsWithLength.end(), minimalLengthComparatorPair);

        //We keep the chromosomeDimension of the desired size, removing the worst paths
        if (chromosomeDimension >= (int)pathsWithLength.size()) return;
        int app = pathsWithLength.size() - chromosomeDimension;
        for(int i=0 ; i < (app); ++i)
        {
            pathsWithLength.pop_back();
        }
        timeToSort += std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now() - startSort).count();
    }

    //This function will calculate the new population by applying the crossover
    void calculateChromosomesCrossover() {
        auto start = std::chrono::system_clock::now();
        calculateChunksDimensionForInput(pathsWithLength.size() - 1);
        auto chunksIt = chunks.begin();
        for_each(workers.begin(), workers.end(), [&](future<void> &worker) {

            worker = async(launch::async, [&, start = chunksIt->first, end = chunksIt->second] {
                vector<Chromosome> appPush;
                appPush.clear();
                appPush.reserve(pathsWithLength.size());

                for (int chromosomeIndex = start; chromosomeIndex <= end; chromosomeIndex+=2) {
                    pair<vector<int>,int > app = calculateCrossoverPath(pathsWithLength[chromosomeIndex].first, pathsWithLength[chromosomeIndex + 1].first);
                    appPush.push_back(app);
                }
                lock_guard<mutex> guard(m);
                pathsWithLength.insert(pathsWithLength.end(), appPush.begin(), appPush.end());
            });
            chunksIt++;

        });
        for_each(workers.begin(), workers.end(), [&](std::future<void> &worker) { worker.get(); });
        auto end = std::chrono::system_clock::now();
        timeToCrossover += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    //It consists of reversing a random subsequence of a percentageOfMutations% elements
    void calculateChromosomesMutations(int percentageOfMutations)
    {
        auto start = std::chrono::system_clock::now();
        int randomValueInt = rand() % chromosomeDimension;
        int chromosomeDimensionApp = (chromosomeDimension * percentageOfMutations) / 100;
        vector<Chromosome> appPush;
        appPush.clear();
        appPush.reserve(pathsWithLength.size());
        for (int i = 0; i <= chromosomeDimensionApp; i++) {
            int chromosomeIndexReal = (i + randomValueInt) % chromosomeDimension;
            vector<int> app = calculateMutatedPath(pathsWithLength[chromosomeIndexReal].first);
            appPush.push_back(make_pair(app,0));
        }
        pathsWithLength.insert(pathsWithLength.end(), appPush.begin(), appPush.end());

        auto end = std::chrono::system_clock::now();
        timeToMutate += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    /*
    //It consists of reversing a random subsequence of the first quarter of the parent population
    void calculateChromosomesMutations(int percentageOfMutations)
    {
        auto start = std::chrono::system_clock::now();
        int randomValueInt = rand() % chromosomeDimension;
        int chromosomeDimensionApp = (chromosomeDimension * percentageOfMutations) / 100;
        calculateChunksDimensionForInput(chromosomeDimensionApp);
        auto chunksIt = chunks.begin();
        for_each(workers.begin(), workers.end(), [&](future<void> &worker) {

            worker = async(launch::async, [&, start = chunksIt->first, end = chunksIt->second] {
                vector<Chromosome> appPush;
                appPush.clear();
                appPush.reserve(chromosomeDimensionApp * 2);
                for (int chromosomeIndex = start; chromosomeIndex <= end; chromosomeIndex++) {
                    int chromosomeIndexReal = (chromosomeIndex + randomValueInt) % chromosomeDimension;
                    vector<int> app = calculateMutatedPath(pathsWithLength[chromosomeIndexReal].first);
                    appPush.push_back(make_pair(app,0));
                }
                lock_guard<mutex> guard(m);
                pathsWithLength.insert(pathsWithLength.end(), appPush.begin(), appPush.end());
            });
            chunksIt++;

        });
        for_each(workers.begin(), workers.end(), [&](std::future<void> &worker) { worker.get(); });
        auto end = std::chrono::system_clock::now();
        timeToMutate += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    */

    int getBestPathLength()
    {
        return pathsWithLength[0].second;
    }

    string getCalculatorType()
    {
        return "Native Threads";
    }

    string getNumberOfThreads()
    {
        return  " with " + to_string(workers.size()) + " threads";
    }

    ParallelCalculator(int desiredNumberOfWorkers, int chromosomeDimension):
            realWorkers(desiredNumberOfWorkers), chromosomeDimension(chromosomeDimension) {
    }
};


#endif //S_VEZZUTO_PARALLEL_CALCULATOR_H
