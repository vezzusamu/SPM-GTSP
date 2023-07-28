//
// Created by s.vezzuto on 6/14/23.
//

#ifndef S_VEZZUTO_CALCULATOR_H
#define S_VEZZUTO_CALCULATOR_H

#include <vector>
#include <iostream>
#include <map>
#include <algorithm>
#include <fstream>
#include <random>
#include <thread>
#include <future>
#include <cstring>
#include <mutex>
#include <chrono>

using namespace std;

typedef pair<vector<int>, int> Chromosome;

class Calculator {

protected:
    int** citiesDistances_;
    int numberOfCities_;
    std::chrono::_V2::system_clock::time_point startingTime;
    int timeToSort = 0;
    int timeToEvaluate = 0;
    int timeToCrossover = 0;
    int timeToMutate = 0;
    int timeToGenerateRandomPath = 0;


    //This function will choose two random indices, and reverse the elements between them
    vector<int> calculateMutatedPath(vector<int> &pathToMutate)
    {
        vector<int> mutatedPath = pathToMutate;

        int idx1 = rand() % numberOfCities_;
        int idx2(idx1);
        while(idx2 == idx1)
        {
            idx2 = rand() % numberOfCities_;
        }
        // ind1 must be less than ind2
        if(idx1>idx2) swap(idx1,idx2);
        //We reverse the elements between ind1 and ind2
        reverse(mutatedPath.begin() + idx1, mutatedPath.begin() + idx2 + 1);
        return mutatedPath;
    }

    Chromosome generateRandomPath() // Complexity O(numberOfCities)
    {
        random_device rd;
        std::mt19937 generator = mt19937(rd());
        vector<int> appVector;
        appVector.reserve(numberOfCities_);
        for(int i=0; i < numberOfCities_; ++i) appVector.push_back(i);
        std::shuffle(appVector.begin(), appVector.end(), generator);
        return make_pair(appVector, 0);
    }


    static bool minimalLengthComparatorPair(const pair<vector<int>, int> &path1 , const pair<vector<int>, int> &path2)
    {
        return (path1.second < path2.second);
    }


public:
    void setCitiesDistances(int** citiesDistances) {
        this->citiesDistances_ = citiesDistances;
    }
    virtual void generateFirstChromosomes()=0;
    virtual void evaluateAndSortChromosomes()=0;
    virtual void calculateChromosomesCrossover()=0;
    virtual void calculateChromosomesMutations(int percentageOfMutations)=0;

    virtual int getBestPathLength()=0;

    void setNumberOfCities(int numberOfCities) {
        this->numberOfCities_ = numberOfCities;
    }
    void setStartingTime(std::chrono::_V2::system_clock::time_point startingTime) {
        this->startingTime = startingTime;
    }

    //The fitness of a path is the sum of the distances to cross all the cities and return to the starting city
    int calculateFitnessForPath(vector<int> v) //Complexity O(numberOfCities)
    {
        int ret = 0 ;
        for (int i=0; i < numberOfCities_ - 1 ; i++)
        {
            ret +=citiesDistances_[v[i]][v[i + 1]];
        }
        //The last edge is between the last city and the first city
        ret += citiesDistances_[v[numberOfCities_ - 1]][v[0]];
        return ret ;
    }

    virtual string getCalculatorType()=0;

    int getTimeElapsed()
    {
        std::chrono::_V2::system_clock::time_point end = std::chrono::system_clock::now();
        return chrono::duration_cast<std::chrono::milliseconds>(end - startingTime).count();
    }

    void printTotalTimeSpentForCalculations()
    {
        cout << "Time spent to evaluate the chromosomes: " << timeToEvaluate << " ms" << endl;
        cout << "Time spent to sort the chromosomes: " << timeToSort << " ms" << endl;
        cout << "Time spent to calculate the crossover: " << timeToCrossover << " ms" << endl;
        cout << "Time spent to calculate the mutations: " << timeToMutate << " ms" << endl;
        cout << "Time spent to generate random paths: " << timeToGenerateRandomPath << " ms" << endl;
    }

    string getElapsedTime()
    {
        string ret = "";
        ret += to_string(timeToEvaluate);
        ret += ",";
        ret += to_string(timeToSort);
        ret += ",";
        ret += to_string(timeToCrossover);
        ret += ",";
        ret += to_string(timeToMutate);
        ret += ",";
        ret += to_string(timeToGenerateRandomPath);

        return ret;
    }

    virtual string getNumberOfThreads(){
        return "";
    }

    Calculator(){
    }
};

#endif //S_VEZZUTO_CALCULATOR_H
