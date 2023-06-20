#pragma once // Add this line to prevent multiple inclusion of the header file

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

class Calculator {

    protected:
        int** citiesDistances;
        int populationDimension;
        int numberOfCities;
        std::mt19937 generator;
        std::chrono::_V2::system_clock::time_point startingTime;
        int timeToSort = 0;
        int timeToCrossover = 0;
        int timeToMutate = 0;
        int timeToGenerateRandomPath = 0;

        pair<vector<int>,int> generateRandomPath() // Complexity O(numberOfCities)
        {
            vector<int> appVector;
            for(int i=0; i < numberOfCities; ++i) appVector.push_back(i);
            std::shuffle(appVector.begin(), appVector.end(), generator);
            return make_pair(appVector, 0);
        }


        static bool minimalLengthComparatorPair(const pair<vector<int>, int> &path1 , const pair<vector<int>, int> &path2)
        {
            return (path1.second < path2.second);
        }

        //PMX Partially mapped crossover
        pair<vector<int>,int> calculateCrossoverPath(vector<int> &path1, vector<int> &path2) // Complexity O(numberOfCities)
        {

            bool visitedCities[numberOfCities];
            int indexesOfPath1Cities[numberOfCities];
            for(int i=0; i < numberOfCities; ++i)
            {
                indexesOfPath1Cities[path1[i]] = i;
            }
            memset(visitedCities, false, sizeof visitedCities);

            int idx = rand() % numberOfCities; // random index from which we will begin the calculateCrossoverPath
            visitedCities[idx] = true;
            idx = indexesOfPath1Cities[path2[idx]];
            while(!visitedCities[idx]) // if visitedCities[idx]==true then we have reached a cycle
            {
                visitedCities[idx] = true;
                //We go to the node in path2 that has the same index as the node in path1, and we find its index in path1
                idx = indexesOfPath1Cities[path2[idx]];
            }
            vector<int> ret;
            ret.resize(numberOfCities);
            //The final path will choose between a node from path1 if it has been visited, otherwise it will contain a node from path2
            for(int i=0; i < numberOfCities; ++i)
            {
                if(visitedCities[i]) ret[i] = path1[i];
                else ret[i] = path2[i];
            }
            return make_pair(ret, 0);
        }

        //This function will choose two random indices, and reverse the elements between them
        
        vector<int> calculateMutatedPath(vector<int> &pathToMutate)
        {
            vector<int> mutatedPath = pathToMutate;

            int ind1 = rand() % numberOfCities;
            int ind2(ind1);
            while(ind2 == ind1)
            {
                ind2 = rand() % numberOfCities;
            }
            // ind1 must be less than ind2
            if(ind1>ind2) swap(ind1,ind2);
            //We reverse the elements between ind1 and ind2
            reverse(mutatedPath.begin() + ind1, mutatedPath.begin() + ind2 + 1);
            return mutatedPath;
        }

    public:
        void setCitiesDistances(int** citiesDistances) {
            this->citiesDistances = citiesDistances;
        }
        virtual void generateParentPaths()=0;
        virtual void sortParentByFitness()=0;
        virtual void calculatePopulationCrossover()=0;
        virtual void calculatePopulationMutations(int percentageOfMutations)=0;
        
        virtual int getBestPathLength()=0;
        void setRandomGenerator(std::mt19937 generator) {
            this->generator = generator;
        }
        void setNumberOfCities(int numberOfCities) {
            this->numberOfCities = numberOfCities;
        }
        void setStartingTime(std::chrono::_V2::system_clock::time_point startingTime) {
            this->startingTime = startingTime;
        }
        //The fitness of a path is the sum of the distances to cross all the cities and return to the starting city
        int calculateFitnessForPath(vector<int> v) //Complexity O(numberOfCities)
        {
            int ret = 0 ;
            for (int i=0; i < numberOfCities - 1 ; i++)
            {
                ret +=citiesDistances[v[i]][v[i + 1]];
            }
            //The last edge is between the last city and the first city
            ret += citiesDistances[v[numberOfCities - 1]][v[0]];
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
            cout << "Time spent to evaluate and sort the population: " << timeToSort << " ms" << endl;
            cout << "Time spent to calculate the crossover: " << timeToCrossover << " ms" << endl;
            cout << "Time spent to calculate the mutations: " << timeToMutate << " ms" << endl;
            cout << "Time spent to generate random paths: " << timeToGenerateRandomPath << " ms" << endl;
        }

        Calculator(){

        }
};