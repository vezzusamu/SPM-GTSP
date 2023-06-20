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

//! if all the chunks have been computed then proceed to the next stage
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

    struct ChromosomeMutationsWorker : ff::ff_node_t<std::pair<int, int>> {
        private:
        vector<pair<vector<int>, int>>& pathsWithLength_;
        int** citiesDistances_;
        int numberOfCities_;
        int randomValueInt_;
        int chromosomeDimension_;
        mutex* m_;

        vector<int> calculateMutatedPath(vector<int> &pathToMutate)
        {
            vector<int> mutatedPath = pathToMutate;

            int ind1 = rand() % numberOfCities_;
            int ind2(ind1);
            while(ind2 == ind1)
            {
                ind2 = rand() % numberOfCities_;
            }
            // ind1 must be less than ind2
            if(ind1>ind2) swap(ind1,ind2);
            //We reverse the elements between ind1 and ind2
            reverse(mutatedPath.begin() + ind1, mutatedPath.begin() + ind2 + 1);
            return mutatedPath;
        }

        public:
        ChromosomeMutationsWorker(vector<pair<vector<int>, int>>* pathsWithLength, mutex *m,int numberOfCities,
                int randomValueInt, int chromosomeDimension) :
        pathsWithLength_(*pathsWithLength), m_(m), numberOfCities_(numberOfCities),
                randomValueInt_(randomValueInt), chromosomeDimension_(chromosomeDimension){}


        std::pair<int, int> *svc(std::pair<int, int> *chunk) override {
                int start = chunk->first;
                int end = chunk->second;
                delete chunk;

                vector<Chromosome> appPush;
                appPush.clear();
                appPush.reserve(chromosomeDimension_);

                for (int chromosomeIndex = start; chromosomeIndex <= end; chromosomeIndex++) {
                    int chromosomeIndexReal = (chromosomeIndex + randomValueInt_) % chromosomeDimension_;
                    vector<int> app = calculateMutatedPath(pathsWithLength_[chromosomeIndexReal].first);
                    appPush.push_back(make_pair(app,0));
                }

                lock_guard<mutex> guard(*m_);
                pathsWithLength_.insert(pathsWithLength_.end(), appPush.begin(), appPush.end());

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

        Chromosome calculateCrossoverPath(vector<int> &path1, vector<int> &path2) // Complexity O(numberOfCities)
        {

            bool visitedCities[numberOfCities_];
            int indexesOfPath1Cities[numberOfCities_];
            for(int i=0; i < numberOfCities_; ++i)
            {
                indexesOfPath1Cities[path1[i]] = i;
            }
            memset(visitedCities, false, sizeof visitedCities);

            int idx = rand() % numberOfCities_; // random index from which we will begin the calculateCrossoverPath
            visitedCities[idx] = true;
            idx = indexesOfPath1Cities[path2[idx]];
            while(!visitedCities[idx]) // if visitedCities[idx]==true then we have reached a cycle
            {
                visitedCities[idx] = true;
                //We go to the node in path2 that has the same index as the node in path1, and we find its index in path1
                idx = indexesOfPath1Cities[path2[idx]];
            }
            vector<int> ret;
            ret.resize(numberOfCities_);
            //The final path will choose between a node from path1 if it has been visited, otherwise it will contain a node from path2
            for(int i=0; i < numberOfCities_; ++i)
            {
                if(visitedCities[i]) ret[i] = path1[i];
                else ret[i] = path2[i];
            }
            return make_pair(ret, 0);
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
                    pair<vector<int>,int > app = calculateCrossoverPath(pathsWithLength_[chromosomeIndex].first,
                                                                        pathsWithLength_[chromosomeIndex + 1].first);
                    appPush.push_back(app);
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

    void work(){
        int numbers []= {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

        // Create the farm
        ff_farm farm;
        ff::ParallelFor pf(10);
        int i = 0;

        auto function = [&](const int num) -> void { numbers[++i] = num *2; };
        pf.parallel_for(0, 9, 1, 0, function, 10);
        for (int i = 0; i < 10; ++i) {
            std::cout << numbers[i] << std::endl;
        }

        /*
        std::vector<ff_node*> workers;

        // Add workers to the farm
        for (int i = 0; i < 30; ++i) {
            workers.push_back(new SumWorker(numbers));
        }

        // Set the emitter and workers for the farm
        farm.add_emitter(new NumberEmitter(numbers));
        farm.add_workers(workers);

        // Run the farm and wait for the result
        if (farm.run_and_wait_end() < 0) {
            std::cerr << "Error executing the farm\n";
            return 1;
        }

        // Get the final result from the farm
        int totalSum = 0;
        ff_node* lastWorker = farm.getlb();
        int* partialSum;
        while ((partialSum = (int*)lastWorker->recv()) != nullptr) {
            totalSum += *partialSum;
            delete partialSum;
        }



        std::cout << "Total sum: " << totalSum << std::endl;
         */
        // Cleanup
    }

    void evaluateAndSortChromosomes() {
        auto start_evaluation = std::chrono::system_clock::now();

        calculateChunksDimensionForInput(pathsWithLength.size());

        //! chromosomes creation farm
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

        //We keep the chromosomeDimension of the desidered size, removing the worst paths
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

        //! chromosomes creation farm
        ChunksEmitter generateChromosomesEmitter(&chunks);
        ChunksCollector generateChromosomesCollector(chunks.size());
        std::vector<std::unique_ptr<ff::ff_node>> generationWorkers;

        for (int i = 0; i < realWorkers; i++) {
            generationWorkers.push_back(std::unique_ptr<ChromosomeCrossoverWorker>(
                    new ChromosomeCrossoverWorker(&pathsWithLength, &m,
                                                  numberOfCities_, chromosomeDimension))
            );
        }

        ff::ff_Farm<std::pair<int, int>>
                creationFarm(std::move(generationWorkers), generateChromosomesEmitter, generateChromosomesCollector);
        creationFarm.wrap_around();
        if (creationFarm.run_and_wait_end() < 0) {
            return;
        }

        auto end = std::chrono::system_clock::now();
        timeToCrossover += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    /*
    void calculateChromosomesMutations(int percentageOfMutations) {
        auto start = std::chrono::system_clock::now();
        int randomValueInt = rand() % chromosomeDimension;
        int chromosomeDimensionApp = (chromosomeDimension * percentageOfMutations) / 100;
        calculateChunksDimensionForInput(chromosomeDimensionApp);

        //! chromosomes creation farm
        ChunksEmitter generateChromosomesEmitter(&chunks);
        ChunksCollector generateChromosomesCollector(chunks.size());
        std::vector<std::unique_ptr<ff::ff_node>> generationWorkers;

        for (int i = 0; i < realWorkers; i++) {
            generationWorkers.push_back(std::unique_ptr<ChromosomeMutationsWorker>(
                    new ChromosomeMutationsWorker(&pathsWithLength, &m,
                                                  numberOfCities_, randomValueInt, chromosomeDimension))
            );
        }
        ff::ff_Farm<std::pair<int, int>>
                creationFarm(std::move(generationWorkers), generateChromosomesEmitter, generateChromosomesCollector);
        creationFarm.wrap_around();
        if (creationFarm.run_and_wait_end() < 0) {
            return;
        }

        auto end = std::chrono::system_clock::now();
        timeToMutate += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    */

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
