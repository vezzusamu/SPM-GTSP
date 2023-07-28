//
// Created by s.vezzuto on 6/14/23.
//

#ifndef TRAVELLING_SALESMAN_PROBLEM_MASTER_FASTFLOW_CALCULATOR_H
#define TRAVELLING_SALESMAN_PROBLEM_MASTER_FASTFLOW_CALCULATOR_H

#include "calculator.h"
#include "ff/ff.hpp"
#include <ff/farm.hpp>
#include <ff/parallel_for.hpp>

using namespace std;
using namespace ff;

struct ChunksEmitter : ff::ff_node_t<std::pair<int, int>> {
private:
    std::vector<std::pair<int, int>> &chunks;
public:
    explicit ChunksEmitter(std::vector<std::pair<int, int>> *chunks)
            : chunks(*chunks) {}

    std::pair<int, int> *svc(std::pair<int, int> *currentGeneration) override {
        std::for_each(chunks.begin(), chunks.end(), [&](std::pair<int, int> chunk) {
            this->ff_send_out(new std::pair<int, int>(chunk.first, chunk.second));
        });
        return this->EOS;
    }
};

struct ChunksCollector : ff::ff_node_t<std::pair<int, int>> {
private:
    const int chunksNumber;
    int currentChunksNumber;
public:
    explicit ChunksCollector(int chunks_number) : chunksNumber(chunks_number), currentChunksNumber(chunks_number) {}
    std::pair<int, int> *svc(std::pair<int, int> *chunk) override {
        currentChunksNumber--;

        if (currentChunksNumber == 0) {
            currentChunksNumber = chunksNumber;
            this->ff_send_out(chunk);
        }
        return this->GO_ON;
    }
};

class FastFlowCalculator : public Calculator {
private:
    int chromosomeDimension, realWorkers;
    std::vector<std::pair<int, int>> chunks;
    std::vector<Chromosome> pathsWithLength;
    mutex m;

    void initializeWorkers(int desiredWorkers) {
        realWorkers = desiredWorkers;
        if (realWorkers > chromosomeDimension) {
            realWorkers = chromosomeDimension;
        }
        chunks.resize(realWorkers);
    }

    void calculateChunksDimensionForInput(const int inputSize) {

        int workerCellsToCompute = std::floor(inputSize / realWorkers);
        int remainedElements = inputSize % realWorkers;
        int index = 0;

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

    struct ChromosomeGenerationWorker : ff::ff_node_t<std::pair<int, int>> {
        private:
            vector<pair<vector<int>, int>>& pathsWithLength_;
            std::random_device rd;
            std::mt19937 gen{rd()};
            std::uniform_real_distribution<double> unif{0, 1};
            int numberOfCities;
            Chromosome generateRandomPath() // Complexity O(numberOfCities)
            {
                vector<int> appVector;
                for(int i=0; i < numberOfCities; ++i) appVector.push_back(i);
                std::shuffle(appVector.begin(), appVector.end(), gen);
                return make_pair(appVector, 0);
            }

        public:
            ChromosomeGenerationWorker(vector<pair<vector<int>, int>>* pathsWithLength,
                                       int numberOfCities) :
                    pathsWithLength_(*pathsWithLength), numberOfCities(numberOfCities){}


            std::pair<int, int> *svc(std::pair<int, int> *chunk) override {
                int start = chunk->first;
                int end = chunk->second;
                delete chunk;

                for (int chromosomeIndex = start; chromosomeIndex <= end; chromosomeIndex++) {
                    auto randomPath = generateRandomPath();
                    pathsWithLength_[chromosomeIndex] = std::make_pair(randomPath.first, randomPath.second);
                }

                this->ff_send_out(new std::pair<int, int>(start, end));
                return this->GO_ON;
            }
    };

    struct ChromosomeSortWorker : ff::ff_node_t<std::pair<int, int>> {
    private:
        vector<pair<vector<int>, int>>& pathsWithLength_;
        int** citiesDistances_;
        int numberOfCities;

        int calculateFitnessForPath(vector<int> v) //Complexity O(numberOfCities)
        {
            int ret = 0 ;
            for (int i=0; i < numberOfCities - 1 ; i++)
            {
                ret += citiesDistances_[v[i]][v[i + 1]];
            }
            //The last edge is between the last city and the first city
            ret += citiesDistances_[v[numberOfCities - 1]][v[0]];
            return ret ;
        }

    public:
        ChromosomeSortWorker(vector<pair<vector<int>, int>>* pathsWithLength,
                             int** citiesDistances, int numberOfCities) :
                pathsWithLength_(*pathsWithLength), citiesDistances_(citiesDistances), numberOfCities(numberOfCities){}


        std::pair<int, int> *svc(std::pair<int, int> *chunk) override {
            int start = chunk->first;
            int end = chunk->second;
            delete chunk;

            for (int chromosomeIndex = start; chromosomeIndex <= end; chromosomeIndex++) {
                auto app = calculateFitnessForPath(pathsWithLength_[chromosomeIndex].first);
                pathsWithLength_[chromosomeIndex].second = app;
            }

            this->ff_send_out(new std::pair<int, int>(start, end));
            return this->GO_ON;
        }
    };

    struct ChromosomeCrossoverWorker : ff::ff_node_t<std::pair<int, int>> {
    private:
        vector<pair<vector<int>, int>>& pathsWithLength_;
        int** citiesDistances_;
        int numberOfCities_;
        int chromosomeDimension_;
        mutex* m_;

        vector<Chromosome> calculateCrossoverPath(vector<int> &path1, vector<int> &path2) // Complexity O(numberOfCities)
        {
            vector<int> returnPath1;
            vector<int> returnPath2;
            returnPath1.resize(numberOfCities_);
            returnPath2.resize(numberOfCities_);

            int indexesOfPath1Cities[numberOfCities_];
            for(int i=0; i < numberOfCities_; i++)
            {
                indexesOfPath1Cities[path1[i]] = i;
                returnPath1[i] = path1[i];
            }

            int indexesOfPath2Cities[numberOfCities_];
            for(int i=0; i < numberOfCities_; i++)
            {
                indexesOfPath2Cities[path2[i]] = i;
                returnPath2[i] = path2[i];
            }

            int idx = rand() % numberOfCities_ - 1;

            for (int i = 0; i < idx; i++) {
                int pathApp1 = indexesOfPath1Cities[path2[i]];
                int pathApp2 = indexesOfPath2Cities[path1[i]];
                std::swap(returnPath1[pathApp1], returnPath1[i]);
                std::swap(returnPath2[pathApp2], returnPath2[i]);
            }

            return {make_pair(returnPath1, 0),make_pair(returnPath2, 0)};
        }

    public:
        ChromosomeCrossoverWorker(vector<pair<vector<int>, int>>* pathsWithLength, mutex *m,int numberOfCities,
                                  int chromosomeDimension) :
                pathsWithLength_(*pathsWithLength), m_(m), numberOfCities_(numberOfCities),
                chromosomeDimension_(chromosomeDimension){}


        std::pair<int, int> *svc(std::pair<int, int> *chunk) override {
            int start = chunk->first;
            int end = chunk->second;
            delete chunk;

            vector<Chromosome> appPush;
            appPush.clear();
            appPush.reserve(chromosomeDimension_);

            for (int chromosomeIndex = start; chromosomeIndex <= end; chromosomeIndex+=2) {
                vector<Chromosome> app = calculateCrossoverPath(pathsWithLength_[chromosomeIndex].first,
                                                                    pathsWithLength_[chromosomeIndex + 1].first);
                appPush.insert(appPush.end(), app.begin(), app.end());
            }

            lock_guard<mutex> guard(*m_);
            pathsWithLength_.insert(pathsWithLength_.end(), appPush.begin(), appPush.end());


            this->ff_send_out(new std::pair<int, int>(start, end));
            return this->GO_ON;
        }
    };

public:

    void generateFirstChromosomes() {
        auto start = std::chrono::system_clock::now();
        initializeWorkers(realWorkers);
        calculateChunksDimensionForInput(chromosomeDimension);
        pathsWithLength.clear();
        pathsWithLength.resize(chromosomeDimension);
        pathsWithLength.reserve(chromosomeDimension * 3);

        ChunksEmitter generateChromosomesEmitter(&chunks);
        ChunksCollector generateChromosomesCollector(chunks.size());
        std::vector<std::unique_ptr<ff::ff_node>> generationWorkers;
        for (int i = 0; i < realWorkers; i++) {
            generationWorkers.push_back(std::unique_ptr<ChromosomeGenerationWorker>(new ChromosomeGenerationWorker(&pathsWithLength, numberOfCities_)));
        }
        ff::ff_Farm<std::pair<int, int>>
                creationFarm(std::move(generationWorkers), generateChromosomesEmitter, generateChromosomesCollector);
        creationFarm.wrap_around();
        if (creationFarm.run_and_wait_end() < 0) {
            return;
        }
        auto end = std::chrono::system_clock::now();
        timeToGenerateRandomPath += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    void evaluateAndSortChromosomes() {
        auto start_evaluation = std::chrono::system_clock::now();

        calculateChunksDimensionForInput(pathsWithLength.size());

        ChunksEmitter generateChromosomesEmitter(&chunks);
        ChunksCollector generateChromosomesCollector(chunks.size());
        std::vector<std::unique_ptr<ff::ff_node>> generationWorkers;
        for (int i = 0; i < realWorkers; i++) {
            generationWorkers.push_back(std::unique_ptr<ChromosomeSortWorker>(new ChromosomeSortWorker(&pathsWithLength, citiesDistances_, numberOfCities_)));
        }
        ff::ff_Farm<std::pair<int, int>>
                creationFarm(std::move(generationWorkers), generateChromosomesEmitter, generateChromosomesCollector);
        creationFarm.wrap_around();
        if (creationFarm.run_and_wait_end() < 0) {
            return;
        }
        timeToEvaluate += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_evaluation).count();
        auto start_sort = std::chrono::system_clock::now();

        sort(pathsWithLength.begin(), pathsWithLength.end(), minimalLengthComparatorPair);

        //We keep the chromosomeDimension of the desired size, removing the worst paths
        if (chromosomeDimension >= (int)pathsWithLength.size()) return;
        int app = pathsWithLength.size() - chromosomeDimension;
        for(int i=0 ; i < (app); ++i)
        {
            pathsWithLength.pop_back();
        }
        timeToSort += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_sort).count();
    }

    void calculateChromosomesCrossover() {
        auto start = std::chrono::system_clock::now();


        int numberOfParentPaths = (int)pathsWithLength.size();
        calculateChunksDimensionForInput(numberOfParentPaths - 1);

        ChunksEmitter chromosomesCrossoverEmitter(&chunks);
        ChunksCollector chromosomesCrossoverCollector(chunks.size());
        std::vector<std::unique_ptr<ff::ff_node>> generationWorkers;

        for (int i = 0; i < realWorkers; i++) {
            generationWorkers.push_back(std::unique_ptr<ChromosomeCrossoverWorker>(
                    new ChromosomeCrossoverWorker(&pathsWithLength, &m,
                                                  numberOfCities_, chromosomeDimension))
            );
        }

        ff::ff_Farm<std::pair<int, int>>
                crossoverFarm(std::move(generationWorkers), chromosomesCrossoverEmitter, chromosomesCrossoverCollector);
        crossoverFarm.wrap_around();
        if (crossoverFarm.run_and_wait_end() < 0) {
            return;
        }

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

    int getBestPathLength() {
        return pathsWithLength[0].second;
    }

    string getCalculatorType() {
        return "FastFlow" ;
    }

    string getNumberOfThreads() {
        return " with " + std::to_string(realWorkers) + " workers";
    }

    FastFlowCalculator(int workersNumber, int chromosomeDimension)
            : realWorkers(workersNumber), chromosomeDimension(chromosomeDimension){
    }
};

#endif //TRAVELLING_SALESMAN_PROBLEM_MASTER_FASTFLOW_CALCULATOR_H
