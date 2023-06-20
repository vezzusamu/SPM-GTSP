//
// Created by s.vezzuto on 6/14/23.
//

#ifndef S_VEZZUTO_SEQUENTIAL_CALCULATOR_H
#define S_VEZZUTO_SEQUENTIAL_CALCULATOR_H

#include "calculator.h"

using namespace std;


class SequentialCalculator: public Calculator {
private:
    std::vector<Chromosome> pathsWithLength;
    int chromosomeDimension;

public:
    void generateFirstChromosomes() {
        auto start = std::chrono::system_clock::now();

        pathsWithLength.clear();
        pathsWithLength.resize(chromosomeDimension);
        pathsWithLength.reserve(chromosomeDimension * 3);

        for (int i=0; i <= chromosomeDimension; i++) {
            pathsWithLength[i] = generateRandomPath();
        }

        auto end = std::chrono::system_clock::now();
        timeToGenerateRandomPath += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    void evaluateAndSortChromosomes() {// Complexity O( chromosomeDimension * log²(chromosomeDimension))
        // Complexity without memoization O(chromosomeDimension * numberOfCities * log(chromosomeDimension))
        auto start_evaluate = std::chrono::system_clock::now();
        for (int i = 0; i < pathsWithLength.size(); i++) {
            auto app = calculateFitnessForPath(pathsWithLength[i].first);
            pathsWithLength[i].second = app;
        }
        timeToEvaluate += std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - start_evaluate).count();
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


    //This function will calculate the new population by applying the crossover
    void calculateChromosomesCrossover() {
        auto start = std::chrono::system_clock::now();
        vector<Chromosome> appPush;
        appPush.clear();
        appPush.reserve(pathsWithLength.size());
        for (int i = 0; i < pathsWithLength.size() - 1; i+=2) {
            pair<vector<int>,int > app = calculateCrossoverPath(pathsWithLength[i].first,
                                                                pathsWithLength[i + 1].first);
            appPush.push_back(app);
        }
        pathsWithLength.insert(pathsWithLength.end(), appPush.begin(), appPush.end());

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

    int getBestPathLength()
    {
        return pathsWithLength[0].second;
    }

    string getCalculatorType()
    {
        return "Sequential";
    }

    SequentialCalculator(int chromosomeDimension)
            : chromosomeDimension(chromosomeDimension){
    }
};


#endif //S_VEZZUTO_SEQUENTIAL_CALCULATOR_H
